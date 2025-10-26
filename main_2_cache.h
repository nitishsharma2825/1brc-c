typedef struct TemperatureRecord {
    double minTemp;
    double maxTemp;
    double totalTemp;
    int numRecords;
} TemperatureRecord;

typedef struct WeatherStation {
    char** name;
    TemperatureRecord* records;
    int count;
    int capacity;
} WeatherStation;

typedef struct {
    char* name;
    TemperatureRecord* record;
} NamedRecord;

void initWeatherStation(WeatherStation* ws, int capacity);
void freeWeatherStation(WeatherStation* ws);
void addStation(WeatherStation* ws, char* name, double temp);