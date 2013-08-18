/* router.h */

#ifndef _ROUTER_H
#define _ROUTER_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "tdata.h"
#include "bitset.h"
#include "util.h"

/* When associated with a stop index, a router_state_t describes a leg of an itinerary. */
typedef struct router_state router_state_t;
/* We could potentially remove the back_time from router_state, but this requires implementing some 
   lookup functions and storing the back_trip_stop rather than the back_stop (global stop index): 
   a trip can pass through a stop more than once.
   TODO rename members to ride_from, walk_from, route, trip, ride_time, walk_time ? */
struct router_state {
    uint32_t back_stop;  // The index of the previous stop in the itinerary
    uint32_t back_route; // The index of the route used to travel from back_stop to here, or WALK
    uint32_t back_trip;  // The index of the trip used to travel from back_stop to here, or WALK
    rtime_t  time;       // The time when this stop was reached
    rtime_t  board_time; // The time at which the trip within back_route left back_stop
    /* Second phase footpath/transfer results */
    uint32_t walk_from;  // The stop from which this stop was reached by walking (2nd phase)
    rtime_t  walk_time;  // The time when this stop was reached by walking (2nd phase)
};

// Scratch space for use by the routing algorithm.
// Making this opaque requires more dynamic allocation.
typedef struct router router_t;
struct router {
    tdata_t tdata;          // The transit / timetable data tables
    rtime_t *best_time;     // The best known time at each stop 
    router_state_t *states; // One router_state_t per stop, per round
    BitSet *updated_stops;  // Used to track which stops improved during each round
    BitSet *updated_routes; // Used to track which routes might have changed during each round

    // We should move more routing state in here, like round and sub-scratch pointers.
};

typedef struct router_request router_request_t;
struct router_request {
    uint32_t from;       // start stop index from the user's perspective, independent of arrive_by
    uint32_t to;         // destination stop index from the user's perspective, independent of arrive_by
    time_t time;         // the departure or arrival time at which to search (in seconds since midnight, should be changed to epoch time)
    double walk_speed;   // speed at which the user walks, in meters per second
    bool arrive_by;      // whether the given time is an arrival time rather than a departure time
    rtime_t time_cutoff; // the latest (or earliest in arrive_by) time to reach the destination (in internal rtime_t 4 second intervals)
    uint32_t max_transfers;  // the largest number of transfers to allow in the result
};

void router_setup(router_t*, tdata_t*);

bool router_request_from_qstring(router_request_t*);

void router_request_dump(router_t*, router_request_t*);

void router_request_initialize(router_request_t*);

void router_request_randomize(router_request_t*);

bool router_request_reverse(router_t*, router_request_t*);

void router_teardown(router_t*);

bool router_route(router_t*, router_request_t*);

uint32_t router_result_dump(router_t*, router_request_t*, char *buf, uint32_t buflen); // return num of chars written

#endif // _ROUTER_H

