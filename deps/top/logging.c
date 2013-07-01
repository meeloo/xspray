/*
 * Copyright (c) 2008, 2009 Apple Computer, Inc.  All rights reserved.
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "statistic.h"
#include "generic.h"
#include "preferences.h"
#include "globalstats.h"
#include "top.h"
#include "sig.h"

struct log_get_total_data {
    int total;
};

static bool log_get_total_iter(struct statistic *s, void *ptr) {
    struct log_get_total_data *data = ptr;
    struct generic_cells *cells;

    cells = s->cells;

    if(NULL == cells)
	return false;

    data->total = cells->length;

    return false;
}

/* Get the total number of processes this run. */
static int log_get_total(void *tinst) {
    struct statistics_controller *c = tinst;
    struct log_get_total_data data;
    
    data.total = 0;

    c->iterate(c, log_get_total_iter, &data);

    return data.total;
}

struct log_print_header_data {
    int x;
    int xoffset;
    bool have_limit;
    int ncols;
    int column;
};

static bool log_print_header_iter(struct statistic *s, void *ptr) {
    struct log_print_header_data *data = ptr;
    int max_width, headerlen; 
    struct generic_cells *cells;
    int i, afterx;
    
    cells = s->cells;
    
    if(NULL == cells)
	return false;

    max_width = cells->max_width;

    headerlen = (int)strlen(s->header);
    
    if(headerlen > max_width)
	max_width = headerlen;
    
    afterx = max_width + data->x;

    for(i = 0; i < data->xoffset; ++i)
	putchar(' ');

    printf("%s", s->header);
 
    for(i = headerlen; i < max_width; ++i)
	putchar(' ');
    
    data->x = afterx;
    data->xoffset = /*padding*/ 1;

    data->column++;

    if(data->have_limit && data->column >= data->ncols)
	return false; /*stop iterating*/
        
    return true;
}

static void log_print_header(void *tinst) {
    struct statistics_controller *c = tinst;
    struct log_print_header_data data;

    data.x = 0;
    data.xoffset = 0;
    data.have_limit = top_prefs_get_ncols(&data.ncols);
    data.column = 0;

    c->iterate(c, log_print_header_iter, &data);
}

struct log_print_cell_data {
    int x;
    int xoffset;
    bool have_limit;
    int ncols; /* The limit of the columns. */
    int column; /* The current number of columns. */
    int row;
};

static bool log_print_cell(struct statistic *s, void *ptr) {
    struct log_print_cell_data *data = ptr;
    struct generic_cells *cells;
    size_t i;
    int pad;
    int afterx;
    int max_width;
    int headerlen;
 
    cells = s->cells;
    
    if(NULL == cells)
	return false;
    
    max_width = cells->max_width;
    
    headerlen = (int)strlen(s->header);

    if(headerlen > max_width)
	max_width = headerlen;
    
    afterx = data->x + max_width;

    assert(data->row < cells->length);

    /* Add padding to the left. */
    for(pad = 0; pad < data->xoffset; ++pad) {
	putchar(' ');
    }

    printf("%s", cells->array[data->row].string);
    
    for(i = cells->array[data->row].length; i < (size_t)max_width; ++i) {
	putchar(' ');
    }

    data->x = afterx;
    data->xoffset = 1;

    data->column++;

    /* See if we have displayed all of the columns requested. */
    if(data->have_limit && data->column >= data->ncols)
	return false;

    return true;
}

static bool log_print_globalstats(char *s, void *ptr) {
    printf("%s\n", s);
    return true;
}

static void log_print_all(void *tinst) {
    struct statistics_controller *c = tinst;
    struct log_print_cell_data data;
    int y, ylimit;
    
    top_globalstats_iterate(c->globalstats, log_print_globalstats, NULL);
    putchar('\n');

    log_print_header(tinst);
    putchar('\n');
    
    data.x = 0;
    data.xoffset = /*padding*/ 0;
    data.have_limit = top_prefs_get_ncols(&data.ncols);
    data.column = 0;
    data.row = 0;
       
    ylimit = log_get_total(tinst);

    for(y = 0; y < ylimit; ++y) {
	data.x = 0;
	data.xoffset = 0;
	data.column = 0;
	c->iterate(c, log_print_cell, &data);
	data.row++;
	putchar('\n');
    }
}

void top_logging_loop_body(void *tinst) {
    top_insert(tinst);
    log_print_all(tinst);
    fflush(stdout);
    sleep(top_prefs_get_sleep());
    
    if(top_signal_is_exit_set())
	exit(EXIT_SUCCESS);
}

void top_logging_loop(void *tinst) {
    int samples;
    bool forever = false;

    if(0 == top_prefs_get_samples())
	forever = true;
    
    if(forever) {
	while(1)
	    top_logging_loop_body(tinst);
    } else {
	for(samples = top_prefs_get_samples(); samples > 0; --samples)
	    top_logging_loop_body(tinst);
	
	/* We displayed the requested number of samples. */
	exit(EXIT_SUCCESS);
    }
}
