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

#ifndef GENERIC_H
#define GENERIC_H
#include "statistic.h"

/* A value large enough for a 64-bit number string rep (greater actually). */
enum { GENERIC_INT_SIZE = 30 };

struct generic_cell {
    char *string;
    size_t allocated_length;
    size_t length;
};

struct generic_cells {
    struct generic_cell *array;
    int max_width; /* The maximum width for a generic_cell. */
    size_t length;
    size_t length_allocated;
};

enum {
    GENERIC_DRAW_LEFT, /* Draw with an achor to the left. */
    GENERIC_DRAW_CENTERED, /* Draw text as centered as possible. */
    GENERIC_DRAW_RIGHT /* Draw text anchored to the right. */
};

void generic_draw_aligned(struct statistic *s, int x);
void generic_draw(struct statistic *s, int x);
void generic_draw_extended(struct statistic *s, int x, int y, int anchor, 
			   const char *string, int slen);
void generic_draw_centered(struct statistic *s, int x);
void generic_draw_right(struct statistic *s, int x);
bool generic_resize_cells(struct statistic *s, struct statistic_size *size);
bool generic_move_cells(struct statistic *s, int x, int y);
void generic_get_request_size(struct statistic *s);
bool generic_insert_cell(struct statistic *s, const char *sample);
void generic_reset_insertion(struct statistic *s);
void generic_get_minimum_size(struct statistic *s);

#endif /*GENERIC_H*/
