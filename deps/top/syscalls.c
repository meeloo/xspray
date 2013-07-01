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

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "libtop.h"
#include "user.h"
#include "generic.h"
#include "uinteger.h"

static bool sysmach_insert_cell(struct statistic *s, const void *sample) {
    const libtop_psamp_t *psamp = sample;
    char buf[GENERIC_INT_SIZE];
    
    if(top_uinteger_format_result(buf, sizeof(buf),
				  psamp->syscalls_mach.now,
				  psamp->syscalls_mach.previous,
				  psamp->syscalls_mach.began)) {
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
    .insert_cell = sysmach_insert_cell,
    .reset_insertion = generic_reset_insertion
};

struct statistic *top_sysmach_create(WINDOW *parent, const char *name) {
    return create_statistic(STATISTIC_SYSMACH, parent, NULL, &callbacks, 
			    name);
}


static bool sysbsd_insert_cell(struct statistic *s, const void *sample) {
    const libtop_psamp_t *psamp = sample;
    char buf[GENERIC_INT_SIZE];
    
    if(top_uinteger_format_result(buf, sizeof(buf),
				psamp->syscalls_bsd.now,
				psamp->syscalls_bsd.previous,
				psamp->syscalls_bsd.began)) {
	return true;
    }
    
    return generic_insert_cell(s, buf);
}

static struct statistic_callbacks sysbsd_callbacks = {
    .draw = generic_draw,
    .resize_cells = generic_resize_cells,
    .move_cells = generic_move_cells,
    .get_request_size = generic_get_request_size,
    .get_minimum_size = generic_get_minimum_size,
    .insert_cell = sysbsd_insert_cell,
    .reset_insertion = generic_reset_insertion
};

struct statistic *top_sysbsd_create(WINDOW *parent, const char *name) {
    return create_statistic(STATISTIC_SYSBSD, parent, NULL, &sysbsd_callbacks, 
			    name);
}

