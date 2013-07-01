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
#include "command.h"
#include "generic.h"

static bool command_insert_cell(struct statistic *s, const void *sample) {
    const libtop_psamp_t *psamp = sample;
 
    assert(NULL != psamp->command);

    return generic_insert_cell(s, psamp->command);
}

static void command_get_minimum_size(struct statistic *s) {
    s->minimum_size.width = 12;
    s->minimum_size.height = 1;
}

static struct statistic_callbacks callbacks = {
    .draw = generic_draw,
    .resize_cells = generic_resize_cells,
    .move_cells = generic_move_cells,
    .get_request_size = generic_get_request_size,
    .get_minimum_size = command_get_minimum_size,
    .insert_cell = command_insert_cell,
    .reset_insertion = generic_reset_insertion
};

struct statistic *top_command_create(WINDOW *parent, const char *name) {
    return create_statistic(STATISTIC_COMMAND, parent, NULL, &callbacks, 
			 name);
}
