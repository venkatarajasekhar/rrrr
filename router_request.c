#include "config.h"
#include "router_request.h"
#include "util.h"

#include <stdio.h>

/* router_request_to_epoch returns the time-date
 * used in the request in seconds since epoch.
 */

time_t router_request_to_epoch (router_request_t *req, tdata_t *tdata, struct tm *tm_out) {
    time_t seconds;
    calendar_t day_mask = req->day_mask;
    uint8_t cal_day = 0;

    while (day_mask >>= 1) cal_day++;

    seconds = tdata->calendar_start_time + (cal_day * SEC_IN_ONE_DAY) + RTIME_TO_SEC(req->time - RTIME_ONE_DAY) - ((tdata->dst_active & 1 << cal_day) ? SEC_IN_ONE_HOUR : 0);

    rrrr_localtime_r (&seconds, tm_out);

    return seconds;
}


/* router_request_to_date returns the date used
 * in the request in seconds since epoch.
 */

time_t router_request_to_date (router_request_t *req, tdata_t *tdata, struct tm *tm_out) {
    time_t seconds;
    calendar_t day_mask = req->day_mask;
    uint8_t cal_day = 0;

    while (day_mask >>= 1) cal_day++;

    seconds = tdata->calendar_start_time + (cal_day * SEC_IN_ONE_DAY) - ((tdata->dst_active & 1 << cal_day) ? SEC_IN_ONE_HOUR : 0);

    rrrr_localtime_r (&seconds, tm_out);

    return seconds;
}

/* router_request_initialize sets all the defaults for the request.
 * it will not set required arguments such as arrival and departure
 * stops, not it will set the time.
 */

void router_request_initialize(router_request_t *req) {
    req->walk_speed = RRRR_DEFAULT_WALK_SPEED;
    req->walk_slack = RRRR_DEFAULT_WALK_SLACK;
    req->walk_max_distance = RRRR_DEFAULT_WALK_MAX_DISTANCE;
    req->from = req->to = req->via = NONE;
    req->time = UNREACHED;
    req->time_cutoff = UNREACHED;
    req->arrive_by = true;
    req->time_rounded = false;
    req->calendar_wrapped = false;
    req->max_transfers = RRRR_DEFAULT_MAX_ROUNDS - 1;
    req->mode = m_all;
    req->trip_attributes = ta_none;
    req->optimise = o_all;
    req->n_banned_routes = 0;
    req->n_banned_stops = 0;
    req->n_banned_trips = 0;
    req->n_banned_stops_hard = 0;
    memset32(req->banned_route, NONE, RRRR_MAX_BANNED_ROUTES);
    memset32(req->banned_stop, NONE, RRRR_MAX_BANNED_STOPS);
    memset32(req->banned_stop_hard, NONE, RRRR_MAX_BANNED_STOPS);
    memset32(req->banned_trip_route, NONE, RRRR_MAX_BANNED_TRIPS);
    memset32(req->banned_trip_offset, NONE, RRRR_MAX_BANNED_TRIPS);
    req->onboard_trip_route = NONE;
    req->onboard_trip_offset = NONE;
    req->intermediatestops = false;

    #ifdef RRRR_FEATURE_AGENCY_FILTER
    req->agency = AGENCY_UNFILTERED;
    #endif
}

/* Initializes the router request then fills in its time and datemask fields from the given epoch time. */
/* TODO: if we set the date mask in the router itself we wouldn't need the tdata here. */
void router_request_from_epoch(router_request_t *req, tdata_t *tdata, time_t epochtime) {
    #if 0
    char etime[32];
    strftime(etime, 32, "%Y-%m-%d %H:%M:%S\0", localtime(&epochtime));
    printf ("epoch time: %s [%ld]\n", etime, epochtime);
    router_request_initialize (req);
    #endif
    struct tm origin_tm;
    uint32_t cal_day;

    req->time = epoch_to_rtime (epochtime, &origin_tm);
    req->time_rounded = (origin_tm.tm_sec % 4);
    /* TODO not DST-proof, use noons */
    cal_day = (mktime(&origin_tm) - tdata->calendar_start_time) / SEC_IN_ONE_DAY;
    if (cal_day > 31 ) {
        /* date not within validity period of the timetable file, wrap to validity range
         * 28 is a multiple of 7, so we always wrap up to the same day of the week
         */
        cal_day %= 28;
        printf ("calendar day out of range. wrapping to %d, which is on the same day of the week.\n", cal_day);
        req->calendar_wrapped = true;
    }
    req->day_mask = 1 << cal_day;
}

/* router_request_randomize creates a completely filled in, working request.
 */

void router_request_randomize (router_request_t *req, tdata_t *tdata) {
    req->walk_speed = RRRR_DEFAULT_WALK_SPEED;
    req->walk_slack = RRRR_DEFAULT_WALK_SLACK;
    req->walk_max_distance = RRRR_DEFAULT_WALK_MAX_DISTANCE;
    req->time = RTIME_ONE_DAY + SEC_TO_RTIME(3600 * 9 + rrrrandom(3600 * 12));
    req->via = NONE;
    req->time_cutoff = UNREACHED;
    /* 0 or 1 */
    req->arrive_by = rrrrandom(2);
    req->max_transfers = RRRR_DEFAULT_MAX_ROUNDS - 1;
    req->day_mask = 1 << rrrrandom(32);
    req->mode = m_all;
    req->trip_attributes = ta_none;
    req->optimise = o_all;
    req->n_banned_routes = 0;
    req->n_banned_stops = 0;
    req->n_banned_trips = 0;
    req->n_banned_stops_hard = 0;
    memset32(req->banned_route, NONE, RRRR_MAX_BANNED_ROUTES);
    memset32(req->banned_stop, NONE, RRRR_MAX_BANNED_STOPS);
    memset32(req->banned_stop_hard, NONE, RRRR_MAX_BANNED_STOPS);
    memset32(req->banned_trip_route, NONE, RRRR_MAX_BANNED_TRIPS);
    memset32(req->banned_trip_offset, NONE, RRRR_MAX_BANNED_TRIPS);
    req->intermediatestops = false;
    req->from = rrrrandom(tdata->n_stops);
    req->to = rrrrandom(tdata->n_stops);

    #ifdef RRRR_FEATURE_LATLON
    req->from_latlon = tdata->stop_coords[rrrrandom(tdata->n_stops)];
    req->to_latlon = tdata->stop_coords[rrrrandom(tdata->n_stops)];
    req->from = NONE;
    req->to = NONE;
    #endif

    #ifdef RRRR_FEATURE_AGENCY_FILTER
    req->agency = AGENCY_UNFILTERED;
    #endif
}

/* router_request_next updates the current request structure with
 * the next request using the rtime_t resolution (4s)
 */

void router_request_next(router_request_t *req, rtime_t inc) {
    req->time += inc;

    if (req->time >= 21600) {
        req->day_mask++;
        req->time -= 21600;
    }

    req->time_cutoff = UNREACHED;
    req->time_rounded = false;
    req->max_transfers = RRRR_DEFAULT_MAX_ROUNDS - 1;
}

/* Reverse the direction of the search leaving most request parameters unchanged but applying time
 * and transfer cutoffs based on an existing result for the same request.
 * Returns a boolean value indicating whether the request was successfully reversed.
 */
bool router_request_reverse(router_t *router, router_request_t *req) {
    uint32_t round = NONE;
    uint32_t best_stop_index = NONE;
    uint32_t max_transfers = req->max_transfers;
    /* Variable Array Length implementation for states[round][stop_index]
     * router_state_t (*states)[router->tdata->n_stops] = (router_state_t(*)[]) (router->states);
     */

    /* range-check to keep search within states array */
    if (max_transfers >= RRRR_DEFAULT_MAX_ROUNDS)
        max_transfers = RRRR_DEFAULT_MAX_ROUNDS - 1;

    #ifdef RRRR_FEATURE_LATLON
    if ((!req->arrive_by ? req->to == NONE : req->from == NONE)) {
        HashGridResult *hg_result;
        uint32_t stop_index;
        double distance;
        rtime_t best_time = (req->arrive_by ? 0 : UNREACHED);

        if (req->arrive_by) {
            hg_result = &req->from_hg_result;
        } else {
            hg_result = &req->to_hg_result;
        }

        HashGridResult_reset(hg_result);

        I printf ("Reversal - Hashgrid results:\n");

        stop_index = HashGridResult_next_filtered(hg_result, &distance);

        while (stop_index != HASHGRID_NONE) {
            rtime_t extra_walktime = 0;

            if (router->best_time[stop_index] != UNREACHED) {
                extra_walktime = SEC_TO_RTIME((uint32_t)((distance * RRRR_WALK_COMP) / req->walk_speed));

                if (req->arrive_by) {
                    router->best_time[stop_index] -= extra_walktime;
                    if (router->best_time[stop_index] > best_time) {
                        best_stop_index = stop_index;
                        best_time = router->best_time[stop_index];
                    }
                } else {
                    router->best_time[stop_index] += extra_walktime;
                    if (router->best_time[stop_index] < best_time) {
                        best_stop_index = stop_index;
                        best_time = router->best_time[stop_index];
                    }
                }

            }

            I printf ("%d %s %s (%.0fm) %d %d\n", stop_index, tdata_stop_id_for_index(router->tdata, stop_index), tdata_stop_name_for_index(router->tdata, stop_index), distance, router->best_time[stop_index], extra_walktime);

            /* get the next potential start stop */
            stop_index = HashGridResult_next_filtered(hg_result, &distance);
        }

        if (req->arrive_by) {
            req->from = best_stop_index;
        } else {
            req->to = best_stop_index;
        }

        /* TODO: Ideally we should implement a o_transfers option here to find the stop that requires the
         * least transfers and is the best with respect to arrival time. This might be a different stop
         * than the stop that is the best with most transfers.
         */

    } else
    #endif
    {
        best_stop_index = (req->arrive_by ? req->from : req->to);
    }

    {
        /* find the solution with the most transfers and the earliest arrival */
        uint32_t r;
        for (r = 0; r <= max_transfers; ++r) {
            if (router->states[r * router->tdata->n_stops + best_stop_index].walk_time != UNREACHED) {
                round = r;
                /* Instead of the earliest arrival (most transfers)
                * use the solution with the least transfers.
                */
                if (req->optimise == o_transfers) break;
            }
        }
    }

    /* In the case that no solution was found, the request will remain unchanged. */
    if (round == NONE) return false;

    req->time_cutoff = req->time;
    req->time = router->states[round * router->tdata->n_stops + best_stop_index].walk_time;
    #if 0
    printf ("State present at round %d \n", round);
    router_state_dump (&(states[round][stop]));
    #endif
    req->max_transfers = round;
    req->arrive_by = !(req->arrive_by);
    #if 0
    router_request_dump(router, req);
    /* TODO: range-check the resulting request here? */
    #endif
    return true;

    /* Eigenlijk zou in de counter clockwise stap een walkleg niet naar de target moeten gaan,
     * maar naar de de fictieve arrival / departure halte. Zou mooi zijn om een punt te introduceren
     * die dat faciliteert, dan zou je op dat punt een apply_hashgrid kunnen doen, ipv apply_transfers.
     */
}

/* router_request_dump prints the current request structure to the screen */

void router_request_dump(router_request_t *req, tdata_t *tdata) {
    char *from_stop_id = tdata_stop_name_for_index(tdata, req->from);
    char *to_stop_id   = tdata_stop_name_for_index(tdata, req->to);
    char time[32], time_cutoff[32], date[11];
    struct tm ltm;

    router_request_to_date (req, tdata, &ltm);
    strftime(date, 11, "%Y-%m-%d", &ltm);

    btimetext(req->time, time);
    btimetext(req->time_cutoff, time_cutoff);
    printf("-- Router Request --\n"
           "from:  %s [%d]\n"
           "to:    %s [%d]\n"
           "date:  %s\n"
           "time:  %s [%d]\n"
           "speed: %f m/sec\n"
           "arrive-by: %s\n"
           "max xfers: %d\n"
           "max time:  %s\n"
           "mode: ",
           from_stop_id, req->from, to_stop_id, req->to, date, time,
           req->time, req->walk_speed,
           (req->arrive_by ? "true" : "false"), req->max_transfers, time_cutoff);

    if (req->mode == m_all) {
        printf("transit\n");
    } else {
         if ((req->mode & m_tram)      == m_tram)      printf("tram,");
         if ((req->mode & m_subway)    == m_subway)    printf("subway,");
         if ((req->mode & m_rail)      == m_rail)      printf("rail,");
         if ((req->mode & m_bus)       == m_bus)       printf("bus,");
         if ((req->mode & m_ferry)     == m_ferry)     printf("ferry,");
         if ((req->mode & m_cablecar)  == m_cablecar)  printf("cablecar,");
         if ((req->mode & m_gondola)   == m_gondola)   printf("gondola,");
         if ((req->mode & m_funicular) == m_funicular) printf("funicular,");
         printf("\b\n");
    }
}

time_t req_to_date (router_request_t *req, tdata_t *tdata, struct tm *tm_out) {
    time_t seconds;
    uint32_t day_mask = req->day_mask;
    uint8_t cal_day = 0;

    while (day_mask >>= 1) cal_day++;

    seconds = tdata->calendar_start_time + (cal_day * SEC_IN_ONE_DAY);
    rrrr_localtime_r(&seconds, tm_out);

    return seconds;
}

time_t req_to_epoch (router_request_t *req, tdata_t *tdata, struct tm *tm_out) {
    time_t seconds;
    uint32_t day_mask = req->day_mask;
    uint8_t cal_day = 0;

    while (day_mask >>= 1) cal_day++;

    seconds = tdata->calendar_start_time + (cal_day * SEC_IN_ONE_DAY) + RTIME_TO_SEC(req->time - RTIME_ONE_DAY);
    rrrr_localtime_r(&seconds, tm_out);

    return seconds;
}

/* Check the given request against the characteristics of the router that will be used.
 * Indexes larger than array lengths for the given router, signed values less than zero, etc.
 * can and will cause segfaults and present security risks.
 *
 * We could also infer departure stop etc. from start trip here, "missing start point" and reversal problems.
 */
bool range_check(router_request_t *req, tdata_t *tdata) {
    if (req->time < 0)               return false;
    if (req->walk_speed < 0.1)       return false;
    if (req->from >= tdata->n_stops) return false;
    if (req->to   >= tdata->n_stops) return false;
    return true;
}

