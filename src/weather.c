/*
 * weather.c - optional weather line via wttr.in.
 *
 * Only compiled in when built with `make WITH_WEATHER=1` (see
 * Makefile), which links libcurl. Without that flag this file
 * still compiles but every function is a harmless no-op/status
 * message, so the base klok binary has zero extra dependencies.
 *
 * wttr.in serves a plain one-line summary (e.g. "Rotterdam: partly
 * cloudy +18C") when it sees a curl-like User-Agent, and an HTML
 * page otherwise -- so we deliberately send "curl/klok" as our
 * User-Agent below. Units: by default we don't force metric or
 * imperial and let wttr.in pick based on the requester's IP
 * geolocation (their usual default is Celsius, except for US
 * requests); -U/'U' can force metric ("&m") or imperial ("&u").
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "klok.h"
#include "weather.h"

static bool force_refresh = false;

void
weather_toggle(void)
{
    klok.weather.enabled = !klok.weather.enabled;
    if (klok.weather.enabled)
        force_refresh = true;
    else
        klok.weather.have_data = false;
}

void
weather_set_location(const char *location)
{
    if (!location)
        location = "";
    strncpy(klok.weather.location, location, sizeof(klok.weather.location) - 1);
    klok.weather.location[sizeof(klok.weather.location) - 1] = '\0';
    klok.weather.enabled = true;
    force_refresh = true;
}

void
weather_cycle_unit(void)
{
    klok.weather.unit = (weather_unit_t)((klok.weather.unit + 1) % WEATHER_UNIT_COUNT);
    force_refresh = true;
}

#ifdef WITH_WEATHER

#include <curl/curl.h>

struct fetch_buf {
    char data[512];
    size_t len;
};

static size_t
write_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    struct fetch_buf *buf = (struct fetch_buf *)userdata;
    size_t n = size * nmemb;
    if (n > sizeof(buf->data) - buf->len - 1)
        n = sizeof(buf->data) - buf->len - 1;
    if (n > 0) {
        memcpy(buf->data + buf->len, ptr, n);
        buf->len += n;
        buf->data[buf->len] = '\0';
    }
    return size * nmemb; /* tell curl we "handled" all of it */
}

void
weather_init(void)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void
weather_shutdown(void)
{
    curl_global_cleanup();
}

static void
strip_trailing_newline(char *s)
{
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[n - 1] = '\0';
        n--;
    }
}

void
weather_tick(void)
{
    if (!klok.weather.enabled)
        return;

    time_t now = time(NULL);
    if (!force_refresh && klok.weather.have_data &&
        (now - klok.weather.last_fetch) < klok.weather.refresh_sec)
        return;

    CURL *curl = curl_easy_init();
    if (!curl) {
        klok.weather.fetch_failed = true;
        return;
    }

    const char *unit_param = "";
    if (klok.weather.unit == WEATHER_UNIT_METRIC)
        unit_param = "&m";
    else if (klok.weather.unit == WEATHER_UNIT_IMPERIAL)
        unit_param = "&u";

    char url[256];
    if (klok.weather.location[0]) {
        snprintf(url, sizeof(url), "https://wttr.in/%s?format=3%s",
                  klok.weather.location, unit_param);
    } else {
        snprintf(url, sizeof(url), "https://wttr.in/?format=3%s", unit_param);
    }

    struct fetch_buf buf = {0};

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/klok");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    klok.weather.last_fetch = now;
    force_refresh = false;

    if (res != CURLE_OK || http_code != 200 || buf.len == 0) {
        klok.weather.fetch_failed = true;
        return;
    }

    strip_trailing_newline(buf.data);
    strncpy(klok.weather.line, buf.data, sizeof(klok.weather.line) - 1);
    klok.weather.line[sizeof(klok.weather.line) - 1] = '\0';
    klok.weather.have_data = true;
    klok.weather.fetch_failed = false;
}

#else /* !WITH_WEATHER */

void weather_init(void) {}
void weather_shutdown(void) {}

void
weather_tick(void)
{
    if (klok.weather.enabled && !klok.weather.have_data) {
        strncpy(klok.weather.line, "weather: built without WITH_WEATHER",
                 sizeof(klok.weather.line) - 1);
        klok.weather.line[sizeof(klok.weather.line) - 1] = '\0';
        klok.weather.have_data = true;
        force_refresh = false;
    }
}

#endif /* WITH_WEATHER */
