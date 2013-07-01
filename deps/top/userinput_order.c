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
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include "preferences.h"
#include "userinput_order.h"

static void order_completion(void *tinst, struct user_input_state *s) {
    if(!strlen(s->buf)) {
	/* Use the current order. */
	user_input_set_state(NULL);
	return;
    }

    if(top_prefs_set_sort(s->buf)) {
	char errbuf[60];
	if(-1 == snprintf(errbuf, sizeof(errbuf), "invalid order: %s\n", 
			  s->buf)) {
	    user_input_set_error_state("order buffer overflow");
	    return;
	}
	user_input_set_error_state(errbuf);
	return;
    }

    user_input_set_state(NULL);
}

static void order_draw(void *tinst, struct user_input_state *s, WINDOW *win,
		       int row, int column) {
    char display[60];
    
    if(-1 == snprintf(display, sizeof(display), "primary key [%c%s]: %s\n",
		      top_prefs_get_ascending() ? '+' : '-',
		      top_prefs_get_sort_string(),
		      s->buf))
	return;

    mvwaddstr(win, row, column, display);
}

struct user_input_state top_user_input_order_state = {
    .offset = 0,
    .completion = order_completion,
    .draw = order_draw
};
