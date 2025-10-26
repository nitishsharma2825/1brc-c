typedef struct Station {
    char* name;
    double minTemp;
    double maxTemp;
    double totalTemp;
    int numRecords;
} Station;

typedef struct WeatherStation {
    // array of stations, like Vector<Stations> which is resizable
    // Can I do struct of arrays here? like 2 arrays 1 for names and other for stations which would be cache friendly
    Station* stations;
    int count;
    int capacity;
} WeatherStation;

void initWeatherStation(WeatherStation* ws, int capacity);
void freeWeatherStation(WeatherStation* ws);
void addStation(WeatherStation* ws, char* name, double temp);

// typedef struct WeatherStation {
//     // array of pointers
//     Station** stations;
//     int count;
//     int capacity
// } WeatherStation;

// OR Complete Struct of arrays

// typedef struct {
//     char** names;        // array of char*
//     double* minTemp;
//     double* maxTemp;
//     double* totalTemp;
//     int* numRecords;
//     int count;
//     int capacity;
// } WeatherStation;

// Memory Layout:
// names:        [ptr1, ptr2, ptr3, ...]
// minTemp:      [12.3, 14.2, 8.4, ...]
// maxTemp:      [15.0, 20.2, 12.0, ...]
// totalTemp:    [...]
// numRecords:   [...]

// OR Hybrid memory Layout 1

// typedef struct {
//     char* name;
//     int index; use this for fast lookup using hashmap for stations
// } StationIndex;


// OR Hybrid memory Layout 2

// typedef struct {
//     char** name;  help in cache friedly search of station
//     Station* stations;
//     int count;
//     int capacity;
// } WeatherStation;


// Struct prefixing: C style inheritance