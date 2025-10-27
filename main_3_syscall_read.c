#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// for IO system calls and options
#include <fcntl.h>
#include <unistd.h>

// for boolean
#include <stdbool.h>

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
    int fd = open("../1brc-java/measurements.txt", O_RDONLY);
    if (fd < 0)
    {
        perror("open failed");
        return 1;
    }

    char buffer[32768];
    char myBuffer[32768];

    int myBufferStart = 0;
    int myBufferBound = 0;

    char* city = NULL;
    char* temp = NULL;

    int bytesRead;
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
    {
        for (int i = 0; i < bytesRead; i++)
        {
            if (buffer[i] == ';')
            {
                myBuffer[myBufferStart++] = '\0';
                city = &myBuffer[myBufferBound];
                myBufferBound = myBufferStart;
            }
            else if (buffer[i] == '\n')
            {
                myBuffer[myBufferStart] = '\0';
                temp = &myBuffer[myBufferBound];
                addStation(&ws, city, atof(temp));
                // after every new line, I can start from 0 in myBuffer
                myBufferStart = 0;
                myBufferBound = 0;
            }
            else
            {
                myBuffer[myBufferStart++] = buffer[i];
            }
        }
    }

    close(fd);

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


// time elapsed for 413 records: 1367.346s with 4KB,
// 1000s with 8KB
// 500s with 16KB and O3 flag
// 494s with 32KB and O3 flag