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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <curses.h>
#include "libtop.h"
#include "generic.h"
#include "log.h"
#include "top.h"

enum { HEADER_SIZE = 1 };

int generic_draw_header(struct statistic *s, int x, int y, int anchor) {
    if(s->header) {
	generic_draw_extended(s, x, y, anchor, s->header, strlen(s->header));
	y += HEADER_SIZE; 
    }

    return y;
}

void generic_draw_aligned(struct statistic *s, int x) {
    int y = 0;
    struct generic_cells *cells;
    int peaklength = 0;
    int maxy;
    size_t i;

    cells = s->cells;

    if(NULL == cells)
	return;

    /* This is needed for panels. */
    werase(s->window);

    y = generic_draw_header(s, x, y, GENERIC_DRAW_LEFT);
   
    maxy = s->actual_size.height;
   
    peaklength = s->actual_size.width;

    for(i = 0; i < cells->length && y < maxy; ++i, ++y) {
	int len = cells->array[i].length;
     	int localx;
    
	localx = peaklength - len;
		
	if(len <= 0)
	    continue;

	if(ERR == mvwaddnstr(s->window, y, localx,
			     cells->array[i].string, len)) {
	    int erry, errx;
	    getmaxyx(s->window, erry, errx);

	    top_log("mvwaddnstr error in %s\n", __func__);
	    top_log("error info: s->header %s y %d x %d n %d string: %s\n"
		    "s->window width %d height %d\n",
		    s->header, y, localx + x, len, 
		    cells->array[i].string,
		    errx, erry);
	}
    }
}

void generic_draw(struct statistic *s, int x) {
    int y = 0;
    size_t i;
    struct generic_cells *cells;
    int maxy;

    cells = s->cells;
    
    if(NULL == cells)
	return;

    /* This is needed for panels. */
    werase(s->window);

    y = generic_draw_header(s, x, y, GENERIC_DRAW_LEFT);

    maxy = s->actual_size.height;

    for(i = 0; i < cells->length && y < maxy; ++i, ++y) {
	generic_draw_extended(s, x, y, GENERIC_DRAW_LEFT, 
			      cells->array[i].string,
			      cells->array[i].length);
    }
}

/* 
 * This is meant to be called by a statistic draw function.  
 * It's not directly for use in a callback.
 */
void generic_draw_extended(struct statistic *s, int xoffset, int yoffset, int anchor, const char *string, int slen) {
    int yheight, xwidth, xpos;
    int n;

    xwidth = s->actual_size.width;
    yheight = s->actual_size.height;

    switch(anchor) {
    case GENERIC_DRAW_LEFT:
	n = xwidth - xoffset;
	if(n > slen)
	    n = slen;		
	
	mvwaddnstr(s->window, yoffset, xoffset, string, n);
	break;

    case GENERIC_DRAW_CENTERED:
	/* Find the middle, and then subtract half of the string. */
	xpos = ((xwidth - xoffset) / 2) - (slen / 2);

	if(xpos < 0) {
      	    /*
	     * The string resulted in an offset less than the requested
	     * offset.  We may need to truncate it later.
	     */
	    xpos = 0;
	}

	n = slen;

	if((slen + xpos) >= xwidth) {
	    /* The n chars is too large for this stat width. */
	    n = xwidth - xpos - xoffset;
	}
	
	if(n < 0) {
	    xpos = 0;
	    n = 0;
	}
	
	mvwaddnstr(s->window, yoffset, xpos + xoffset, string, n);
	break;

    case GENERIC_DRAW_RIGHT: 
	xpos = xwidth - slen - xoffset;
	
	if(xpos < xoffset) {
	    /* The string is too big to fit. */
	    xpos = xoffset;
	}
	
	n = xwidth - xpos;
	if(n > slen)
	    n = slen;
	
	mvwaddnstr(s->window, yoffset, xpos + xoffset, string, n);
	break;
    }

    wsyncup(s->window);
}

void generic_draw_centered(struct statistic *s, int x) {
    struct generic_cells *cells;
    size_t i;
    int y = 0;

    cells = s->cells;
   
    if(NULL == cells)
        return;

    y = generic_draw_header(s, x, y, GENERIC_DRAW_CENTERED);

    for(i = 0; i < cells->length; ++i) {
        generic_draw_extended(s, x, y, GENERIC_DRAW_CENTERED, 
                              cells->array[i].string, 
			      cells->array[i].length);
        ++y;
    }
}

void generic_draw_right(struct statistic *s, int x) {
    struct generic_cells *cells;
    size_t i;
    int y = 0;

    cells = s->cells;
   
    if(NULL == cells)
        return;

    y = generic_draw_header(s, x, y, GENERIC_DRAW_RIGHT);
    
    for(i = 0; i < cells->length; ++i) {
        generic_draw_extended(s, x, y, GENERIC_DRAW_RIGHT, 
                              cells->array[i].string,
			      cells->array[i].length);
        ++y;
    }
}

bool generic_resize_cells(struct statistic *s, struct statistic_size *size) {
    if(ERR == wresize(s->window, size->height, size->width))
        return true;
    
    return false;
}

bool generic_move_cells(struct statistic *s, int x, int y) {
    if(ERR == move_panel(s->panel, y, x))
        return true;
    
    return false;
}

void generic_get_request_size(struct statistic *s) {
    struct generic_cells *cells;
  
    cells = s->cells;

    if(NULL == cells)
	return;

    s->request_size.width = cells->max_width;
    s->request_size.height = cells->length;
}

static struct generic_cells *alloc_generic_cells(void) {
    size_t i, length = 10;
    struct generic_cells *cells;

    cells = malloc(sizeof *cells);
    if(NULL == cells)
	return NULL;

    cells->array = malloc(sizeof(*(cells->array)) * length);
    if(NULL == cells->array) {
	free(cells);
	return NULL;
    }

    for(i = 0; i < length; ++i) {
	cells->array[i].string = NULL;
	cells->array[i].length = 0;
	cells->array[i].allocated_length = 0;
    }

    cells->max_width = 0;
    cells->length = 0;
    cells->length_allocated = length;
    
    return cells;
}

static void free_generic_cells(struct generic_cells *cells) {
    size_t i;

    for(i = 0; i < cells->length_allocated; ++i) {
	if(cells->array[i].string) {
	    free(cells->array[i].string);
	}
    }

    free(cells->array);
    free(cells);
}

static void generic_cell_destructor(struct statistic *s, void *ptr) {
    free_generic_cells(s->cells);
}

/* Return true if an error occurred. */
bool generic_insert_cell(struct statistic *s, const char *sample) {
    struct generic_cells *cells = s->cells;
    size_t offset;
    int sample_length = (int)strlen(sample);
    int sample_z_length = sample_length + 1;

#if 0
    top_log("%s %s\n", __func__, sample);
#endif

    if(NULL == cells) {
	cells = s->cells = alloc_generic_cells();

	if(NULL == cells)
	    return true;

	if(create_statistic_destructor(s, generic_cell_destructor, NULL))
	    return true;
    }

    
    /* Update the max_width if the sample_length is greater. */
    if(sample_length > cells->max_width) {
	cells->max_width = sample_length;
	
	if(cells->max_width > s->actual_size.width) {
	    /* This requests a top_layout at a later time. */
	    top_relayout(s->controller, s->type, cells->max_width);
	}
    }

    offset = cells->length;

    cells->length += 1;

    if(cells->length >= cells->length_allocated) {
	/* Resize the array to store the length and more. */

	size_t newlength = cells->length_allocated * 2;
	size_t i;
	cells->array = realloc(cells->array, 
			       sizeof(*(cells->array)) * newlength);

	if(NULL == cells->array)
	    return true;

	for(i = cells->length_allocated; i < newlength; ++i) {
	    cells->array[i].string = NULL;
	    cells->array[i].length = 0;
	    cells->array[i].allocated_length = 0;
	} 

	cells->length_allocated = newlength;
    }

    if(0 == sample_length) {
	cells->array[offset].length = 0;
	return false;
    }

    if(cells->array[offset].string
       && cells->array[offset].allocated_length >= sample_z_length) {
	/* We have an existing buffer that should fit. */
	memcpy(cells->array[offset].string, sample, sample_z_length);
	cells->array[offset].length = sample_length;
    } else {
	/* There wasn't enough space or the string was NULL. */
	free(cells->array[offset].string);
	
	cells->array[offset].string = malloc(cells->max_width + 1);

	if(NULL == cells->array[offset].string) {
	    cells->array[offset].length = 0;
	    return true;
	}
	
	cells->array[offset].length = sample_length;
	cells->array[offset].allocated_length = cells->max_width + 1;
	memcpy(cells->array[offset].string, sample, sample_z_length);
    }

    return false;
}

void generic_reset_insertion(struct statistic *s) {
    struct generic_cells *cells;

    cells = s->cells;
    if(NULL == cells)
	return;

    cells->length = 0;
}

void generic_get_minimum_size(struct statistic *s) {
    struct generic_cells *cells;

    cells = s->cells;
    
    if(NULL == cells)
	return;

    
    s->minimum_size.width = cells->max_width; 
    s->minimum_size.height = cells->length;

    if(s->minimum_size.width < 4)
	s->minimum_size.width = 4;
}
