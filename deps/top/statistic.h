/*
 * Copyright (c) 2008 Apple Computer, Inc.  All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 * 
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

#ifndef STATISTIC_H
#define STATISTIC_H

#include <stdio.h>
#include <inttypes.h>
#include <curses.h>
#include <panel.h>
#include <stdbool.h>

/* 
 * Extend this if we add another type.  
 * These types are used for sorting, and priority of display. 
 */
enum {
    /*Begin same as old top*/
    STATISTIC_PID, STATISTIC_COMMAND, STATISTIC_CPU, STATISTIC_TIME,
    STATISTIC_THREADS, STATISTIC_PORTS, STATISTIC_MREGION, 
    STATISTIC_RPRVT, STATISTIC_RSHRD, STATISTIC_RSIZE, STATISTIC_VSIZE,
    /*End same as old top*/
    STATISTIC_VPRVT, 
    STATISTIC_PGRP, STATISTIC_PPID, STATISTIC_PSTATE,
    STATISTIC_UID, STATISTIC_WORKQUEUE, STATISTIC_FAULTS, STATISTIC_COW_FAULTS,
    STATISTIC_MESSAGES_SENT, STATISTIC_MESSAGES_RECEIVED, STATISTIC_SYSBSD,
    STATISTIC_SYSMACH, STATISTIC_CSW, STATISTIC_PAGEINS, STATISTIC_KPRVT, STATISTIC_KSHRD,
    STATISTIC_USER
};

enum {
    STATISTIC_TOTAL = STATISTIC_USER + 1
};

struct statistic;
struct statistics_controller;

struct statistic_name_map {
    int e;
    struct statistic *(*creator)(WINDOW *parent, const char *name);
    const char *name;
};

extern struct statistic_name_map statistic_name_map[];

struct statistic_state {
    int type;
    char name[50];
    struct statistic *(*create_statistic)(WINDOW *parent, const char *name);
    struct statistic *instance;
};

struct statistics_controller {
    void *globalstats;
    WINDOW *parent;
    struct statistic_state state[STATISTIC_TOTAL];
    int total_possible_statistics;
    int total_active_statistics;

    void (*reset_insertion)(struct statistics_controller *c);
    void (*insert_sample)(struct statistics_controller *c, const void *sample);
    void (*remove_tail)(struct statistics_controller *c);
    void (*insert_tail)(struct statistics_controller *c);
    int (*get_total_possible)(struct statistics_controller *c);
    int (*get_total_active)(struct statistics_controller *c);
    void (*iterate)(struct statistics_controller *c, 
		    bool (*func)(struct statistic *, void *),
		    void *ptr);
};

struct statistic_size {
    int width, height;
};

struct statistic_destructor {
    /* This provides a destructor that may be called to cleanup a statistic. */
    void (*destructor) (struct statistic *s, void *ptr);
    /* This is passed to the callback stored above. */
    void *ptr;
    struct statistic_destructor *next;
};

struct statistic_callbacks {
    void (*draw) (struct statistic *s, int x);
    bool (*resize_cells) (struct statistic *s, struct statistic_size *size);
    bool (*move_cells) (struct statistic *s, int x, int y);
    void (*get_request_size) (struct statistic *s);
    void (*get_minimum_size) (struct statistic *s);
    bool (*insert_cell) (struct statistic *s, const void *sample);
    void (*reset_insertion) (struct statistic *s);
};

struct statistic {
    WINDOW *parent;
    WINDOW *window;
    PANEL *panel;
    int type; /* The STATISTIC enum type from above. */
    void *cells; /* This usually stores a struct generic_cell. */
    void *ptr; /* This is for private data created at the time of
		* create_statistic.
		*/
    char *header;

    bool visible;

    struct statistics_controller *controller;

    /* Used for profiling: */
    uint64_t time_consumed;
    uint64_t runs;
    
    struct statistic_callbacks callbacks;
    struct statistic_destructor *destructors;
    struct statistic_size request_size; /* Used with get_request_size */
    struct statistic_size actual_size;
    struct statistic_size minimum_size;
    struct statistic *previous, *next;
};


/* This should be called before any of the functions below: */
struct statistics_controller *create_statistics_controller(WINDOW *parent);

/* 
 * This takes a STATISTIC_ enum type from above, a WINDOW parent for a derwin,
 * and a pointer of some sort (if desired for extra data).
 *
 * This returns NULL when an error occurs.
 */
struct statistic *create_statistic(int type, WINDOW *parent, void *ptr,
				   struct statistic_callbacks *callbacks,
				   const char *name);
				   
/*
 * This destroys the statistic and calls any destructors necessary.
 */
void destroy_statistic(struct statistic *s);

/* 
 * This creates a callback that is invoked when destroy_statistic is called. 
 *
 * This returns true when an error occurs.
 */
bool create_statistic_destructor(struct statistic *s, 
				 void (*destructor) (struct statistic *, void *),
				 void *ptr);

#endif /*STATISTIC_H*/
