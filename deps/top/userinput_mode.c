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
#include "top.h"

static void mode_completion(void *tinst, struct user_input_state *s) {
    if(!strlen(s->buf)) {
	/* Use the current mode. */
	user_input_set_state(NULL);
	return;
    }

    if(top_prefs_set_mode(s->buf)) {
	char errbuf[60];
	if(-1 == snprintf(errbuf, sizeof(errbuf), "invalid mode: %s\n", 
			  s->buf)) {
	    user_input_set_error_state("mode error buffer overflow");
	    return;
	}
	user_input_set_error_state(errbuf);
	return;
    }

    /*Success*/

    /*
     * This has an order dependency, and assumes that the 
     * relayout will be lazy. 
     */
    top_relayout_force();
    top_insert(tinst);
    user_input_set_state(NULL);
}

static void mode_draw(void *tinst, struct user_input_state *s, WINDOW *win,
		       int row, int column) {
    char display[60];
    
    if(-1 == snprintf(display, sizeof(display), "mode [%s]: %s\n",
		      top_prefs_get_mode_string(), s->buf))
	return;

    mvwaddstr(win, row, column, display);
}

struct user_input_state top_user_input_mode_state = {
    .offset = 0,
    .completion = mode_completion,
    .draw = mode_draw
};
