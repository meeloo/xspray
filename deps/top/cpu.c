/*
 * Copyright (c) 2002-2004, 2008 Apple Computer, Inc.  All rights reserved.
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

#include <stdlib.h>
#include <sys/time.h>
#include "libtop.h"
#include "cpu.h"
#include "generic.h"
#include "preferences.h"
#include "log.h"

extern const libtop_tsamp_t *tsamp;

static bool cpu_insert_cell(struct statistic *s, const void *sample) {
    const libtop_psamp_t *psamp = sample;
    struct timeval elapsed, used;
    char buf[10];
    unsigned long long elapsed_us = 0, used_us = 0;
    int whole = 0, part = 0;

    if(0 == psamp->p_seq) {
	whole = 0;
	part = 0;

	if(-1 == snprintf(buf, sizeof(buf), "%d.%1d", whole, part))
	    return true;

	return generic_insert_cell(s, buf);
    }


    switch(top_prefs_get_mode()) {
    case STATMODE_ACCUM:
	timersub(&tsamp->time, &tsamp->b_time, &elapsed);
	timersub(&psamp->total_time, &psamp->b_total_time, &used);
	break;

	
    case STATMODE_EVENT:
    case STATMODE_DELTA:
    case STATMODE_NON_EVENT:
	timersub(&tsamp->time, &tsamp->p_time, &elapsed);
	timersub(&psamp->total_time, &psamp->p_total_time, &used);
	break;

    default:
	fprintf(stderr, "unhandled STATMOMDE in %s\n", __func__);
	abort();
    }

    elapsed_us = (unsigned long long)elapsed.tv_sec * 1000000ULL
	+ (unsigned long long)elapsed.tv_usec;

    used_us = (unsigned long long)used.tv_sec * 1000000ULL
	+ (unsigned long long)used.tv_usec;
      
    /* Avoid a divide by 0 exception. */
    if(elapsed_us > 0) {
	whole = (used_us * 100ULL) / elapsed_us;
	part = (((used_us * 100ULL) - (whole * elapsed_us)) * 10ULL) / elapsed_us;
    }

    //top_log("command %s whole %d part %d\n", psamp->command, whole, part);
   
    if(-1 == snprintf(buf, sizeof(buf), "%d.%1d", whole, part))
	return true;

    return generic_insert_cell(s, buf);
}

static struct statistic_callbacks callbacks = {
    .draw = generic_draw,
    .resize_cells = generic_resize_cells,
    .move_cells = generic_move_cells,
    .get_request_size = generic_get_request_size,
    .get_minimum_size = generic_get_minimum_size,
    .insert_cell = cpu_insert_cell,
    .reset_insertion = generic_reset_insertion
};

struct statistic *top_cpu_create(WINDOW *parent, const char *name) {
    return create_statistic(STATISTIC_CPU, parent, NULL, &callbacks, name);
}
