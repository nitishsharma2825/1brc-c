#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

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

    // allocate on stack since its lifecycle is tied to main
    // if dynamically allocated, need to free by hand
    WeatherStation ws;
    initWeatherStation(&ws, 16);

    // using high level library calls like fgets/fread, extra libc overhead and 2 userspace memory buffers
    FILE* file = fopen("../1brc-java/measurements.txt", "r");

    char buffer[1024]; // holds 1 line but what if 1 line does not fit in 8KB?

    // fgets treats file as byte stream but reads it as text, no conversion needed
    // it overwrites as many bytes it needs for the line, plus a null terminator [\n\0 at end], rest of the buffers stays as it is
    while (fgets(buffer, sizeof(buffer), file)) {
        // process each line
        char* station = strtok(buffer, ";"); // replaces by \0
        char* temp_str = strtok(NULL, "\n"); // works on the same char*, remember next starting point
        double temp = atof(temp_str);

        addStation(&ws, station, temp);
    }

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


// time take: 700s
// time taken with -O3 flag: 576s