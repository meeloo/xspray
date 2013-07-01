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

#include <inttypes.h>
#include "libtop.h"
#include "threads.h"
#include "generic.h"
#include "preferences.h"
#include "uinteger.h"

/* Return true if an error occurred. */
static bool threadcount_insert_cell(struct statistic *s, const void *sample) {
    const libtop_psamp_t *psamp = sample;
    char thbuf[GENERIC_INT_SIZE];
    char runbuf[GENERIC_INT_SIZE];
    char buf[GENERIC_INT_SIZE * 2 + 1];
    struct top_uinteger thtotal, thrun;

    thtotal = top_uinteger_calc_result(psamp->th, psamp->p_th, 0ULL);
    thrun = top_uinteger_calc_result(psamp->running_th, psamp->p_running_th,
				     0ULL);

    if(top_sprint_uinteger(thbuf, sizeof(thbuf), thtotal))
	return true;

    if(top_sprint_uinteger(runbuf, sizeof(runbuf), thrun))
       return true;
       
    if(-1 == (thrun.value > 0 ? snprintf(buf, sizeof(buf), "%s/%s", thbuf, runbuf) : snprintf(buf, sizeof(buf), "%s", thbuf)))
	return true;
        
    return generic_insert_cell(s, buf);
}

static struct statistic_callbacks callbacks = {
    .draw = generic_draw,
    .resize_cells = generic_resize_cells,
    .move_cells = generic_move_cells,
    .get_request_size = generic_get_request_size,
    .get_minimum_size = generic_get_minimum_size,
    .insert_cell = threadcount_insert_cell,
    .reset_insertion = generic_reset_insertion
};

struct statistic *top_threadcount_create(WINDOW *parent, const char *name) {
    return create_statistic(STATISTIC_THREADS, parent, NULL, &callbacks,
			    name);
}
