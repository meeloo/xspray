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

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "layout.h"
#include "top.h"
#include "log.h"

enum { COLUMN_PADDING = 1 };

struct enlarge_data {
    int add;
    int maxy;
    int count;
};

static bool enlarge_iterator(struct statistic *s, void *ptr) {
    struct enlarge_data *data = ptr;
    int size;

    if(s->request_size.width > s->minimum_size.width) {
        size = s->request_size.width;
    } else {
        size = s->minimum_size.width;
    }

    if(data->count > 0)
	size += COLUMN_PADDING;

    s->actual_size.width = size;

    s->actual_size.width += data->add;
    s->actual_size.height = data->maxy;

    data->count += 1;

    /* Return true for more iterations (if possible). */
    return true;
}

/* This assumes that toadd is >= 1.0, so the caller must verify that. */
static void enlarge(struct statistics_controller *c, 
					int maxy, int toadd) {
    struct enlarge_data data;

    data.add = toadd;
    data.maxy = maxy;
    data.count = 0;

    c->iterate(c, enlarge_iterator, &data);
}


struct do_layout_data {
    int x, y;
    bool error;
};

static bool do_layout_iterator(struct statistic *s, void *ptr) {
    struct do_layout_data *data = ptr;
    struct statistic_size firstsize = {.width = 1, .height = 1};
	
    if(s->callbacks.resize_cells(s, &firstsize)) {
	data->error = true;
	return false;
    }

    if(s->callbacks.move_cells(s, data->x, data->y)) {
	top_log("error: moving cells for %s!\n", s->header);
	top_log("error info: data->x %d data->y %d\n",
		data->x, data->y);
	
	data->error = true;
	/*stop iterating*/
	return false;
     }

    if(s->callbacks.resize_cells(s, &s->actual_size)) {
	top_log("error: resizing cells for %s!\n", s->header);
	top_log("error info: data->x %d s->actual_size.width %d "
		"s->actual_size.height %d\n", data->x, s->actual_size.width,
		s->actual_size.height);

	data->error = true;
	/*stop iterating*/
	return false;
    }
    
    data->x += s->actual_size.width;

    /*continue iterating*/
    return true;
}

static bool do_layout(struct statistics_controller *c, int y) {
    struct do_layout_data data;

    data.x = 0;
    data.y = y;
    data.error = false;

    c->iterate(c, do_layout_iterator, &data);

    return data.error;
}

struct get_size_data {
    struct statistic_size *reqsize;
    struct statistic_size *minsize;
    int count;
};

/* Return true if this iterator wants more data. */
static bool get_size_iterator(struct statistic *s, void *ptr) {
    struct get_size_data *data = ptr;
    int minwidth, reqwidth;

    s->callbacks.get_request_size(s);
    s->callbacks.get_minimum_size(s);
    
    minwidth = s->minimum_size.width;
    reqwidth = s->request_size.width;

    /* Skip padding the first column's left side. */
    if(data->count > 0) {
	minwidth += COLUMN_PADDING;
	reqwidth += COLUMN_PADDING;
    }

    data->minsize->width += minwidth;

    if(s->minimum_size.height > data->minsize->height)
	data->minsize->height = s->minimum_size.height;
  
    if(reqwidth > minwidth) {
        data->reqsize->width += reqwidth;
    } else {
        data->reqsize->width += minwidth;
    }

    if(s->request_size.height > data->reqsize->height)
	data->reqsize->height = s->request_size.height;

    data->count += 1;

    return true;
}

static void get_size(struct statistics_controller *c, 
		     struct statistic_size *reqsize,
		     struct statistic_size *minsize) {
    struct get_size_data data;

    reqsize->width = 0;
    reqsize->height = 0;
    minsize->width = 0;
    minsize->height = 0;

    data.reqsize = reqsize;
    data.minsize = minsize;
    data.count = 0;

    c->iterate(c, get_size_iterator, &data);
}

struct minsize_fit_data {
    int maxy;
    int count;
};

static bool minsize_fit_iterator(struct statistic *s, void *ptr) {
    struct minsize_fit_data *data = ptr;

    s->actual_size.width = s->minimum_size.width;

    /* Don't pad the first column's left side. */
    if(data->count > 0)
	s->actual_size.width += COLUMN_PADDING;

    s->actual_size.height = data->maxy;

    data->count += 1;

    /*more iterations*/
    return true;
}

static void minsize_fit(struct statistics_controller *c, int maxy) {
    struct minsize_fit_data data;
    
    data.maxy = maxy;
    data.count = 0;

    c->iterate(c, minsize_fit_iterator, &data);
}

/*
 * The layout algorithm:
 * We first attempt to fit as many statistics as possible using the 
 * minimum size of each.  If we are at a point where the width of the
 * terminal is such that the total statistics is equal to the total
 * possible statistics, then we attempt to use the request size.
 * If the request size exceeds the terminal size, then we fall back
 * to the minimum size.  If the request size fits, then we try to 
 * enlarge proportionately every column, as long as the enlargement
 * is > 1 for each column.
 */

/* Return true if an error occurred. */
/* The caller will probably want to schedule another layout after a failure. */
bool layout_statistics(struct statistics_controller *c, int maxx, int maxy,
		       int y) {
    struct statistic_size reqsize, minsize;
    int total_stats = 0;
    int inserts = 0, removes = 0;

    if(maxy <= 0)
	return false;

    while(1) {
	get_size(c, &reqsize, &minsize);
	total_stats = c->get_total_active(c);
	
	if(minsize.width == maxx) {
	    minsize_fit(c, maxy);
	    break;
	} else if(minsize.width > maxx) {
	    /* The minsize is greater than the total window width. */

	    if(total_stats > 1) {
		/* Mark that we removed to prevent an endless loop. */
		++removes;
		//werase(c->parent);
		c->remove_tail(c);
		//werase(c->parent);
		continue;
	    }

	    minsize_fit(c, maxy);
	    break;
	} else {
	    if(0 == removes && total_stats < c->get_total_possible(c)) {
		++inserts;
		c->insert_tail(c);
		continue;
	    }    

	    minsize_fit(c, maxy);
	    break;
	}	    
    }
    
    //werase(c->parent);

    total_stats = c->get_total_active(c);
    if(total_stats >= c->get_total_possible(c)) {
	/*
	 * We have all of the possible statistics. 
	 * Now we attempt to enlarge to the request size or beyond.
	 */
	get_size(c, &reqsize, &minsize);
	
	if(reqsize.width == maxx) {
	    /* The request size fit exactly. */
	    enlarge(c, maxy, /*add*/ 0);
	} else if(reqsize.width > maxx) {
	    /* Fallback to the minsize. */
	    minsize_fit(c, maxy);
	} else {
	    /* reqsize.width < maxx */
	    /* 
	     * We can probably enlarge some if not all of the stats.
	     */
	    int xdelta = maxx - reqsize.width;
	    int enlargement = xdelta / c->get_total_possible(c);
	    
	    if(enlargement > 0) {
		enlarge(c, maxy, enlargement);
	    } else {
		enlarge(c, maxy, 0);
	    }
	}
    }

    top_log("c->get_total_active(c) is %d\n", c->get_total_active(c));

    if(do_layout(c, y))
	return true;

    return false;
}
