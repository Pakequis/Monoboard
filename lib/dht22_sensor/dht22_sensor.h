#ifndef DHT22_SENSOR_H
#define DHT22_SENSOR_H

// Initializes the DHT22 sensor. Call once from setup(), before the first
// readDht22() call.
void initDht22();

// Reads temperature (Celsius) and humidity (%). Returns true on success;
// on failure (sensor absent/not wired, or a bad reading), returns false
// and leaves tempCOut/humidityOut untouched.
bool readDht22(float* tempCOut, float* humidityOut);

#endif // DHT22_SENSOR_H
