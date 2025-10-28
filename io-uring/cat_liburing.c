#include <stdio.h> // high level buffered IO: printf, perror, fopen, fclose, fread, fwrite
#include <fcntl.h> // file control options for manipulating file descriptors, open(), fcntl() and flags like O_RDONLY
#include <sys/stat.h> // fstat
#include <sys/ioctl.h> // ioctl() system call for device specific control operations that don't fit normal read/write
#include <stdlib.h> // memory management: malloc, free, exit
#include <string.h>

#include <liburing.h>

#define QUEUE_DEPTH 1
#define BLOCK_SZ    1024

off_t get_file_size(int fd)
{
    struct stat st;
    if (fstat(fd, &st) < 0)
    {
        perror("fstat");
        return -1;
    }

    if (S_ISBLK(st.st_mode))
    {
        unsigned long long bytes;
        if (ioctl(fd, BLKGETSIZE64, &bytes) != 0)
        {
            perror("ioctl");
            return -1;
        }
        return bytes;
    }
    else if (S_ISREG(st.st_mode))
    {
        return st.st_size;
    }

    return -1;
}

void output_to_console(char* buf, int len)
{
    while(len--)
    {
        fputc(*buf++, stdout);
    }
}

struct file_info {
    off_t file_size;
    struct iovec iovecs[];
};

int get_completion_and_print(struct io_uring *ring)
{
    struct io_uring_cqe *cqe;

    // wait for a completion event
    int ret = io_uring_wait_cqe(ring, &cqe);
    if (ret < 0)
    {
        perror("io_uring_wait_cqe");
        return 1;
    }
    if (cqe->res < 0)
    {
        fprintf(stderr, "Async read operation failed.\n");
        return 1;
    }

    // get user data we submitted, this will contain the result of the operation
    struct file_info *fi = io_uring_cqe_get_data(cqe);
    int blocks = (int) fi->file_size / BLOCK_SZ;
    if (fi->file_size % BLOCK_SZ) blocks++;
    for (int i = 0; i < blocks; i++)
    {
        output_to_console(fi->iovecs[i].iov_base, fi->iovecs[i].iov_len);
    }

    io_uring_cqe_seen(ring, cqe);
    return 0;
}

int submit_read_request(char* file_path, struct io_uring *ring)
{
    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0)
    {
        perror("open");
        return 1;
    }

    off_t file_sz = get_file_size(file_fd);
    off_t bytes_remaining = file_sz;
    int blocks = (int) file_sz / BLOCK_SZ;
    if (file_sz % BLOCK_SZ) blocks++;
    int current_block = 0;
    off_t offset = 0;

    struct file_info *fi = malloc(sizeof(*fi) + (sizeof(struct iovec) * blocks));
    fi->file_size = file_sz;

    while (bytes_remaining)
    {
        off_t bytes_to_read = bytes_remaining;
        if (bytes_to_read > BLOCK_SZ) bytes_to_read = BLOCK_SZ;
        offset += bytes_to_read;

        void *buf;
        // heap memory like malloc but aligned as per user, used with direct IO eg: 4KB page size alignment
        if (posix_memalign(&buf, BLOCK_SZ, BLOCK_SZ))
        {
            perror("posix_memalign");
            return 1;
        }
        fi->iovecs[current_block].iov_base = buf;
        fi->iovecs[current_block].iov_len = bytes_to_read;
        current_block++;
        bytes_remaining-=bytes_to_read;
    }

    // Get an SQE
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    // setup a readv operation
    io_uring_prep_readv(sqe, file_fd, fi->iovecs, blocks, 0);
    // Set user data
    io_uring_sqe_set_data(sqe, fi);
    // submit the request
    io_uring_submit(ring);

    return 0;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <filename1> [<filename2> ...]\n", argv[0]);
        return 1;
    }

    // Initialize io_uring
    struct io_uring ring;
    io_uring_queue_init(QUEUE_DEPTH, &ring, 0);

    // for each file passed as argument, call read and print_file() functions
    for (int i = 1; i < argc; i++)
    {
        int ret = submit_read_request(argv[i], &ring);
        if(ret)
        {
            fprintf(stderr, "Error reading file: %s\n", argv[i]);
            return 1;
        }
        get_completion_and_print(&ring);
    }
    fputc('\n', stdout);

    // cleanup
    io_uring_queue_exit(&ring);

    return 0;
}