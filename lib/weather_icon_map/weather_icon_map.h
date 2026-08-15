#ifndef WEATHER_ICON_MAP_H
#define WEATHER_ICON_MAP_H

// Maps an Open-Meteo WMO weather code to a Weather Icons glyph character
// (lib/weather_icons_font). Unknown codes fall back to ':' (blank).
// isDay picks between the day/night variant for the two codes that depict
// a sun disc (clear / partly cloudy) -- every other glyph (cloud, rain,
// snow, thunderstorm) is already day/night neutral and ignores it.
char weatherCodeToIconChar(int wmoCode, int isDay);

#endif // WEATHER_ICON_MAP_H
