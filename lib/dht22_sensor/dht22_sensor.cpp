#include "dht22_sensor.h"
#include "config.h"
#include <DHT.h>

static DHT dht(PIN_DHT22_DATA, DHT22);

void initDht22()
{
  dht.begin();
}

bool readDht22(float* tempCOut, float* humidityOut)
{
  float humidity = dht.readHumidity();
  float tempC = dht.readTemperature();

  if (isnan(humidity) || isnan(tempC))
  {
    return false;
  }

  *tempCOut = tempC;
  *humidityOut = humidity;
  return true;
}
