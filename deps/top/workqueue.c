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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "libtop.h"
#include "workqueue.h"
#include "generic.h"
#include "uinteger.h"

static bool workqueue_insert_cell(struct statistic *s, const void *sample) {
    const libtop_psamp_t *psamp = sample;
    char runbuf[GENERIC_INT_SIZE];
    char totalbuf[GENERIC_INT_SIZE];
    char buf[GENERIC_INT_SIZE * 2 + 1];
    struct top_uinteger run, total;

    run = top_uinteger_calc_result(psamp->wq_run_threads, 
				   psamp->p_wq_run_threads, 0ULL);

    total = top_uinteger_calc_result(psamp->wq_nthreads, 
				     psamp->p_wq_nthreads, 0ULL);

    if(top_sprint_uinteger(runbuf, sizeof(runbuf), run))
		return true;

    if(top_sprint_uinteger(totalbuf, sizeof(totalbuf), total))
        return true;

    if(0 != run.value) {
        if(-1 == snprintf(buf, sizeof(buf), "%s/%s", totalbuf, runbuf))
            return true;
    } else {
        if(-1 == snprintf(buf, sizeof(buf), "%s", totalbuf))
            return true;
    }

    return generic_insert_cell(s, buf);
}

static struct statistic_callbacks callbacks = {
    .draw = generic_draw,
    .resize_cells = generic_resize_cells,
    .move_cells = generic_move_cells,
    .get_request_size = generic_get_request_size,
    .get_minimum_size = generic_get_minimum_size,
    .insert_cell = workqueue_insert_cell,
    .reset_insertion = generic_reset_insertion
};

struct statistic *top_workqueue_create(WINDOW *parent, const char *name) {
    return create_statistic(STATISTIC_WORKQUEUE, parent, NULL, &callbacks, name);
}
