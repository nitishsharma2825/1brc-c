#include <stdio.h> // high level buffered IO: printf, perror, fopen, fclose, fread, fwrite
#include <fcntl.h> // file control options for manipulating file descriptors, open(), fcntl() and flags like O_RDONLY
#include <unistd.h> // close
#include <sys/uio.h> // readv, writev, provides scatter/gather IO system calls i.e reading/writing from multiple buffers in 1 syscall instead of read/write which does only for 1 buffer
#include <sys/stat.h> // fstat
#include <linux/fs.h> // linux file system specific constants and ioctl commands
#include <sys/ioctl.h> // ioctl() system call for device specific control operations that don't fit normal read/write
#include <stdlib.h> // memory management: malloc, free, exit

#define BLOCK_SZ    4096

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

int read_and_print_file(char* file_name)
{
    int file_fd = open(file_name, O_RDONLY);
    if (file_fd < 0)
    {
        perror("open");
        return 1;
    }

    struct iovec *iovecs;

    off_t file_sz = get_file_size(file_fd);
    off_t bytes_remaining = file_sz;
    int blocks = (int) file_sz / BLOCK_SZ;
    if (file_sz % BLOCK_SZ) blocks++;

    // prepare the iovecs arrays
    iovecs = malloc(sizeof(struct iovec) * blocks);
    int current_block = 0;
    while (bytes_remaining)
    {
        off_t bytes_to_read = bytes_remaining;
        if (bytes_to_read > BLOCK_SZ) bytes_to_read = BLOCK_SZ;

        void *buf;
        // heap memory like malloc but aligned as per user, used with direct IO eg: 4KB page size alignment
        if (posix_memalign(&buf, BLOCK_SZ, BLOCK_SZ))
        {
            perror("posix_memalign");
            return 1;
        }
        iovecs[current_block].iov_base = buf;
        iovecs[current_block].iov_len = bytes_to_read;
        current_block++;
        bytes_remaining-=bytes_to_read;
    }

    // read blocks no of buffers in 1 syscall
    int ret = readv(file_fd, iovecs, blocks);
    if (ret < 0)
    {
        perror("readv");
        return 1;
    }

    for (int i = 0; i < blocks; i++)
    {
        output_to_console(iovecs[i].iov_base, iovecs[i].iov_len);
    }
    fputc('\n', stdout);

    free(iovecs);
    close(file_fd);
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <filename1> [<filename2> ...]\n", argv[0]);
        return 1;
    }

    // for each file passed as argument, call read and print_file() functions
    for (int i = 1; i < argc; i++)
    {
        if (read_and_print_file(argv[i]))
        {
            fprintf(stderr, "Error reading file\n");
            return 1;
        }
    }

    return 0;
}