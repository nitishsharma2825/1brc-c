#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "main_1.h"

static int cmpStationName(const void* a, const void* b) {
    const Station* s1 = (const Station*)a;
    const Station* s2 = (const Station*)b;
    return strcmp(s1->name, s2->name); // lexographic order
}

static Station* findStation(WeatherStation* ws, char* name) {
    for (int i = 0; i < ws->count; i++) {
        // compares string with null termination \0
        if (strcmp(ws->stations[i].name, name) == 0) {
            return &ws->stations[i];
        }
    }
    return NULL;
}

void initWeatherStation(WeatherStation* ws, int capacity) {
    ws->stations = (Station*)calloc(capacity, sizeof(Station));
    ws->capacity = capacity;
    ws->count = 0;
}

void freeWeatherStation(WeatherStation* ws) {
    // strdup allocates a new string on heap, so need to free it
    for (int i = 0; i < ws->count; i++) {
        free(ws->stations[i].name);
    }
    free(ws->stations);
}

void addStation(WeatherStation* ws, char* name, double temp) {
    Station* existingStation = findStation(ws, name);
    if (existingStation == NULL) {
        // check if size is good? else reallocate stations
        if (ws->count + 1 > ws->capacity) {
            // increase capacity by 2
            ws->capacity = ws->capacity * 2;
            Station* newStations = (Station*)realloc(ws->stations, ws->capacity * sizeof(Station));
            if (newStations == NULL) {
                perror("realloc failed");
                exit(1);
            }
            ws->stations = newStations;
        }

        // append new station
        existingStation = &ws->stations[ws->count];
        existingStation->name = strdup(name);
        existingStation->maxTemp = temp;
        existingStation->minTemp = temp;
        existingStation->totalTemp = temp;
        existingStation->numRecords = 1;
        ws->count++;
    } else {
        existingStation->minTemp = existingStation->minTemp < temp ? existingStation->minTemp : temp;
        existingStation->maxTemp = existingStation->maxTemp > temp ? existingStation->maxTemp : temp;
        existingStation->totalTemp += temp;
        existingStation->numRecords++;
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

    // sort by name, in place
    qsort(ws.stations, ws.count, sizeof(Station), cmpStationName);

    for (int i = 0; i < ws.count; i++) {
        Station* st  = &ws.stations[i];
        double mean = st->totalTemp / st->numRecords;
        printf("%s=%.1f/%.1f/%.1f\n", st->name, st->minTemp, mean, st->maxTemp);
    }

    clock_t end = clock();

    printf("time elapsed for %d records: %.3fs\n", ws.count, (double)(end - start) / CLOCKS_PER_SEC);
    
    freeWeatherStation(&ws);
    return 0;
}


// Notes
// Some useful functions: strtok: split strings, strncmp: compare 2 strings till length
// memcpy, memset
// strlen, snprintf
// Allocate on heap vs Allocate on stack?
// WeatherStation* weatherStation = (WeatherStation*)calloc(1, sizeof(WeatherStation)); good for dynamic lifetime, passing around

// this allocates weather station on heap so will be present unless freed, other way to allocate it on stack
// Is heap allocation of weatherStation needed? If I allocate weatherStation on stack by declaring it on stack since anyway its used in main's lifecycle only


// time taken: 950s