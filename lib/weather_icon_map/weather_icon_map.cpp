#include "weather_icon_map.h"

char weatherCodeToIconChar(int wmoCode, int isDay)
{
  switch (wmoCode)
  {
    case 0:
      return isDay ? '1' : '8'; // clear sky -> sun / night-clear
    case 1:
    case 2:
      return isDay ? '9' : ';'; // mainly clear / partly cloudy -> sun+cloud / night-cloud
    case 3:
    case 45:
    case 48:
      return '2'; // overcast / fog -> cloud
    case 51:
    case 53:
    case 55:
    case 56:
    case 57:
    case 61:
      return '3'; // drizzle / light rain -> rain
    case 63:
    case 65:
    case 66:
    case 67:
    case 80:
    case 81:
    case 82:
      return '4'; // moderate/heavy rain / rain showers -> hard rain
    case 71:
    case 73:
    case 75:
    case 77:
    case 85:
    case 86:
      return '5'; // snow -> snow
    case 95:
      return '6'; // thunderstorm -> thunder+rain
    case 96:
    case 99:
      return '7'; // thunderstorm with hail -> thunder+hard rain
    default:
      return ':'; // unrecognized code -> blank/all-off
  }
}
