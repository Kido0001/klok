#ifndef KLOK_WEATHER_H
#define KLOK_WEATHER_H

/* One-time setup (curl_global_init when built WITH_WEATHER). */
void weather_init(void);
void weather_shutdown(void);

/* Toggles weather on/off (key 'w'). Turning it on forces an
 * immediate fetch on the next tick. */
void weather_toggle(void);

/* Sets the location (key 'W' prompts for this) and forces a
 * re-fetch on the next tick. Empty string = auto-detect by IP. */
void weather_set_location(const char *location);

/* Cycles auto -> metric -> imperial -> auto (key 'U') and forces a
 * re-fetch on the next tick. */
void weather_cycle_unit(void);

/* Fetches fresh weather data if klok.weather.enabled and
 * refresh_sec has elapsed since the last fetch (or a fetch was
 * just forced). This makes a blocking network call on the calling
 * thread when it does fetch, so klok will visibly pause for the
 * duration of the request fine for a refresh every few minutes,
 * worth backgrounding with a thread if you want it snappier. */
void weather_tick(void);

#endif /* KLOK_WEATHER_H */
