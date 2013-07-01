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
#include "pid.h"
#include "libtop.h"
#include "generic.h"

static void get_pid_suffix(const libtop_psamp_t *psamp, char *suffix,
			   size_t length) {
    bool proc_is_foreign = false;
    bool proc_is_64 = false;
    bool host_is_64 = false;

#if defined(__LP64__)
    host_is_64 = true;
#endif
    
    switch (psamp->cputype) {
    case CPU_TYPE_X86_64:
	proc_is_64 = true;
	// FALLTHROUGH
    case CPU_TYPE_X86:
#if !defined(__i386__) && !defined(__x86_64__)
	proc_is_foreign = true;
#endif
	break;
    case CPU_TYPE_POWERPC64:
	proc_is_64 = true;
	// FALLTHROUGH
    case CPU_TYPE_POWERPC:
#if !defined(__ppc__) && !defined(__ppc64__)
	proc_is_foreign = true;
#endif
	break;
    case CPU_TYPE_ARM:
#if !defined(__arm__)
	proc_is_foreign = true;
#endif
	break;
    default:
	proc_is_foreign = true;
	break;
    }
    
    if(proc_is_foreign) {
	strncpy(suffix, "*", length);
    } else if(host_is_64 && !proc_is_64) {
	strncpy(suffix, "-", length);
    } else {
	strncpy(suffix, " ", length);
    }	
}


static bool pid_insert_cell(struct statistic *s, const void *sample) {
    const libtop_psamp_t *psamp = sample;
    char buf[GENERIC_INT_SIZE + 2];
    char suffix[2];
    unsigned int p;

    get_pid_suffix(psamp, suffix, sizeof(suffix));
    p = (unsigned int)psamp->pid;

    if(-1 == snprintf(buf, sizeof(buf), "%u%s", p, suffix))
	return true;
 
    return generic_insert_cell(s, buf);
}

static struct statistic_callbacks callbacks = {
    .draw = generic_draw,
    .resize_cells = generic_resize_cells, 
    .move_cells = generic_move_cells,
    .get_request_size = generic_get_request_size,
    .get_minimum_size = generic_get_minimum_size,
    .insert_cell = pid_insert_cell, 
    .reset_insertion = generic_reset_insertion
};

struct statistic *top_pid_create(WINDOW *parent, const char *name) {
    return create_statistic(STATISTIC_PID, parent, NULL, &callbacks, name);
}
