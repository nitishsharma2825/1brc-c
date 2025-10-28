#include <stdio.h> // printf, perror
#include <string.h>
#include <stdlib.h>
#include <time.h>

// for IO system calls and file options
#include <fcntl.h> // open(), O_RDONLY
#include <unistd.h> // close(), read(), write()

// for boolean
#include <stdbool.h>

#include <sys/types.h> // size_t
#include <sys/stat.h> // for fstat, struct stat
#include <sys/mman.h> // for mmap, unmap, PROT_*, MAP_* macros

#include "main_2_cache.h"

static int cmpStationName(const void* a, const void* b) {
    const NamedRecord* s1 = (const NamedRecord*)a;
    const NamedRecord* s2 = (const NamedRecord*)b;
    return strcmp(s1->name, s2->name); // lexographic order
}

static int findStation(WeatherStation* ws, char* name) {
    for (int i = 0; i < ws->count; i++) {
        if (strcmp(ws->name[i], name) == 0) {
            return i;
        }
    }

    return -1;
}

void initWeatherStation(WeatherStation* ws, int capacity) {
    ws->name = (char**)calloc(capacity, sizeof(char*));
    ws->records = (TemperatureRecord*)calloc(capacity, sizeof(TemperatureRecord));
    ws->capacity = capacity;
    ws->count = 0;
}

void freeWeatherStation(WeatherStation* ws) {
    for (int i = 0; i < ws->count; i++) {
        free(ws->name[i]);
    }
    free(ws->name);
    free(ws->records);
}

void addStation(WeatherStation* ws, char* name, double temp) {
    int existingStationIndex = findStation(ws, name);
    if (existingStationIndex == -1) {
        // check if size is good? else reallocate stations
        if (ws->count + 1 > ws->capacity) {
            // increase capacity by 2
            ws->capacity = ws->capacity * 2;
            ws->records = (TemperatureRecord*)realloc(ws->records, ws->capacity * sizeof(TemperatureRecord));
            ws->name = (char**)realloc(ws->name, ws->capacity * sizeof(char*));
        }

        // append new station
        TemperatureRecord* existingRecord = &ws->records[ws->count];

        // allocate for string and null termination, can use strdup directly too
        ws->name[ws->count] = (char*)calloc(1, strlen(name) + 1);
        strcpy(ws->name[ws->count], name);

        existingRecord->maxTemp = temp;
        existingRecord->minTemp = temp;
        existingRecord->totalTemp = temp;
        existingRecord->numRecords = 1;
        ws->count++;
    } else {
        TemperatureRecord* existingRecord = &ws->records[existingStationIndex];
        existingRecord->minTemp = existingRecord->minTemp < temp ? existingRecord->minTemp : temp;
        existingRecord->maxTemp = existingRecord->maxTemp > temp ? existingRecord->maxTemp : temp;
        existingRecord->totalTemp += temp;
        existingRecord->numRecords++;
    }
}

int main(void) {

    clock_t start = clock();

    WeatherStation ws;
    initWeatherStation(&ws, 16);

    // low level system calls vs fopen, fread and not buffered too
    // other options include: O_DIRECT, O_SYNC, O_CREAT
    int fd = open("../1brc-java/measurements.txt", O_RDONLY);
    if (fd < 0)
    {
        perror("open failed");
        return 1;
    }

    struct stat st;
    if(fstat(fd, &st) == -1) {
        perror("fstat error");
        return 1;
    }

    char* data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    char buffer[512];
    char* city;
    char* temp;

    int bufferStart = 0;
    int boundary = 0;

    for (off_t i = 0; i < st.st_size; i++)
    {
        if (data[i] == ';')
        {
            buffer[bufferStart++] = '\0';
            city = &buffer[boundary];
            boundary = bufferStart;
        }
        else if (data[i] == '\n')
        {
            buffer[bufferStart] = '\0';
            temp = &buffer[boundary];
            addStation(&ws, city, atof(temp));
            bufferStart = 0;
            boundary = 0;
        }
        else
        {
            buffer[bufferStart++] = data[i];
        }
    }

    close(fd);
    munmap(data, st.st_size);

    NamedRecord* sortArray = (NamedRecord*)calloc(ws.count, sizeof(NamedRecord));
    for (int i = 0; i < ws.count; i++) {
        sortArray[i].name = ws.name[i];
        sortArray[i].record = &ws.records[i];
    }

    qsort(sortArray, ws.count, sizeof(NamedRecord), cmpStationName);

    for (int i = 0; i < ws.count; i++) {
        NamedRecord* st = &sortArray[i];
        double mean = st->record->totalTemp / st->record->numRecords;
        printf("%s=%.1f/%.1f/%.1f\n", st->name, st->record->minTemp, mean, st->record->maxTemp);
    }

    clock_t end = clock();

    printf("time elapsed for %d records: %.3fs\n", ws.count, (double)(end - start) / CLOCKS_PER_SEC);
    
    freeWeatherStation(&ws);
    free(sortArray);
    return 0;
}


// time elapsed for 413 records: 396.841s


// mmap creates a new mapping in virtual address space of the process, this avoids syscalls for IO and process can read from its own memory like array
// MAP_SHARED: share this mapping i.e updates are visible to other processes mapping the same region and in case of file backed mapping are carried through to the underlying file.
// MAP_PRIVATE: private copy on write mapping for this process. Updates not visible to other processes mapping the same file and not carried to the underlying file.
// MAP_ANONYMOUS: not backed by file. fd = -1, just get some memory