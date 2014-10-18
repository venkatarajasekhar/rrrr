#include "util.h"
#include "rrrr_types.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

/* buffer should always be at least 13 characters long,
 * including terminating null
 */
char *btimetext(rtime_t rt, char *buf) {
    char *day;
    uint32_t t, s, m, h;

    if (rt == UNREACHED) {
        strcpy(buf, "   --   ");
        return buf;
    }

    if (rt >= RTIME_THREE_DAYS) {
        day = " +2D";
        rt -= RTIME_THREE_DAYS;
    } else if (rt >= RTIME_TWO_DAYS) {
        day = " +1D";
        rt -= RTIME_TWO_DAYS;
    } else if (rt >= RTIME_ONE_DAY) {
        day = "";
        rt -= RTIME_ONE_DAY;
    } else {
        day = " -1D";
    }

    t = RTIME_TO_SEC(rt);
    s = t % 60;
    m = t / 60;
    h = m / 60;
    m = m % 60;
    sprintf(buf, "%02d:%02d:%02d%s", h, m, s, day);

    return buf;
}

static char buf[13];

char *timetext(rtime_t t) {
    return btimetext(t, buf);
}

void memset32(uint32_t *s, uint32_t u, size_t n) {
    uint32_t i;
    for (i = 0; i < n; i++) {
        s[i] = u;
    }
}

void memset_rtime(rtime_t *s, rtime_t u, size_t n) {
    size_t i;
    for (i = 0; i < n; i++) {
        s[i] = u;
    }
}

uint32_t rrrrandom(uint32_t limit) {
    return (uint32_t) (limit * (rand() / (RAND_MAX + 1.0)));
}

/* assumes little endian http://stackoverflow.com/a/3974138/778449
 * size in bytes
 */
void printBits(size_t const size, void const * const ptr) {
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;
    for (i = size - 1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = b[i] & (1 << j);
            byte >>= j;
            printf("%u", byte);
        }
    }
    puts("");
}

/* Converts the given epoch time to in internal RRRR router time.
 * If epochtime is within the first day of 1970 it is interpreted as seconds
 * since midnight on the current day. If epochtime is 0, the current time and
 * date are used.
 * The intermediate struct tm will be copied to the location pointed to
 * by *stm, unless stm is null. The date should be range checked in the router,
 * where we can see the validity of the tdata file.
 */
rtime_t epoch_to_rtime (time_t epochtime, struct tm *tm_out) {
    struct tm ltm;
    uint32_t seconds;
    rtime_t rtime;

    if (epochtime < SEC_IN_ONE_DAY) {
        /* local date and time */
        time_t now;
        time(&now);
        rrrr_localtime_r (&now, &ltm);
        if (epochtime > 0) {
            /* use current date but specified time */
            struct tm etm;
            /* UTC so timezone doesn't affect interpretation of time */
            rrrr_gmtime_r (&epochtime, &etm);
            ltm.tm_hour = etm.tm_hour;
            ltm.tm_min  = etm.tm_min;
            ltm.tm_sec  = etm.tm_sec;
        }
    } else {
        /* use specified time and date */
        rrrr_localtime_r (&epochtime, &ltm);
    }

    if (tm_out != NULL) {
        *tm_out = ltm;
    }

    seconds = (((ltm.tm_hour * 60) + ltm.tm_min) * 60) + ltm.tm_sec;
    rtime = SEC_TO_RTIME(seconds);
    /* shift rtime to day 1. day 0 is yesterday. */
    rtime += RTIME_ONE_DAY;
    #if 0
    printf ("epoch time is %ld \n", epochtime);

    /* ctime and asctime include newlines */
    printf ("epoch time is %s", ctime(&epochtime));
    printf ("ltm is %s", asctime(&ltm));

    printf ("seconds is %d \n", seconds);
    printf ("rtime is %d \n", rtime);
    #endif
    return rtime;
}

#ifdef _XOPEN_SOURCE
time_t strtoepoch (char *time) {
    struct tm ltm;
    memset (&ltm, 0, sizeof(struct tm));
    strptime (time, "%Y-%m-%dT%H:%M:%S", &ltm);
    ltm.tm_isdst = -1;
    return mktime(&ltm);
}
#else
time_t strtoepoch (char *time) {
    char *endptr;
    struct tm ltm;
    memset (&ltm, 0, sizeof(struct tm));
    ltm.tm_year = strtol(time, &endptr, 10) - 1900;
    ltm.tm_mon  = strtol(&endptr[1], &endptr, 10) - 1;
    ltm.tm_mday = strtol(&endptr[1], &endptr, 10);
    ltm.tm_hour = strtol(&endptr[1], &endptr, 10);
    ltm.tm_min  = strtol(&endptr[1], &endptr, 10);
    ltm.tm_sec  = strtol(&endptr[1], &endptr, 10);
    ltm.tm_isdst = -1;
    return mktime(&ltm);
}
#endif
