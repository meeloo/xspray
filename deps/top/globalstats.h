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

#ifndef GLOBALSTATS_H
#define GLOBALSTATS_H

#include <curses.h>
#include <stdbool.h>

void *top_globalstats_create(WINDOW *parent);
void top_globalstats_draw(void *ptr);
bool top_globalstats_update(void *ptr, const void *sample);
bool top_globalstats_resize(void *ptr, int width, int height, 
			    int *consumed_height);
void top_globalstats_iterate(void *ptr, bool (*iter)(char *, void *), 
			     void *dataptr);

/* This resets the maximum width of the windows, typically after a relayout. */
void top_globalstats_reset(void *ptr);

#endif
