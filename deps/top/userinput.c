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

#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <curses.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include "preferences.h"
#include "userinput.h"
#include "userinput_mode.h"
#include "userinput_sleep.h"
#include "userinput_order.h"
#include "userinput_secondary_order.h"
#include "userinput_user.h"
#include "userinput_signal.h"
#include "userinput_help.h"
#include "top.h"

static struct user_input_state *current_state = NULL;

static void reset_state(struct user_input_state *s) {
    s->buf[0] = '\0';
    s->offset = 0;
}

static void completion(void *tinst, struct user_input_state *s) {
    reset_state(s);
    current_state = NULL;
}

/* This displays an error for 1 draw interval. */
static void error_draw(void *tinst, struct user_input_state *s, WINDOW *win,
		       int row, int column) {
    char display[60];

    if(s->misc > 1) {
	s->completion(tinst, s);
	return;
    }

    if(-1 == snprintf(display, sizeof(display), "error: %s", s->buf))
	return;

    mvwaddstr(win, row, column, display);

    s->misc++;
}

static struct user_input_state error_state = {
    .offset = 0,
    .completion = completion,
    .draw = error_draw
};

void user_input_set_error_state(const char *err) {
    current_state = &error_state;
    reset_state(current_state);
    current_state->misc = 0;
    strncpy(current_state->buf, err, sizeof(current_state->buf));
    current_state->buf[sizeof(current_state->buf) - 1] = '\0';
}

/* This displays status output for 1 draw interval. */
static void status_draw(void *tinst, struct user_input_state *s, WINDOW *win,
			int row, int column) {
    if(s->misc > 1) {
	s->completion(tinst, s);
	return;
    }
    
    mvwaddstr(win, row, column, s->buf);
    
    s->misc++;
}


static struct user_input_state status_state = {
    .offset = 0,
    .completion = completion,
    .draw = status_draw
};

void user_input_set_status_state(const char *status) {
    current_state = &status_state;
    reset_state(current_state);
    current_state->misc = 0;
    strncpy(current_state->buf, status, sizeof(current_state->buf));
    current_state->buf[sizeof(current_state->buf) - 1] = '\0';
}


void user_input_set_state(struct user_input_state *state) {
    current_state = state;
}

/* Process stdin data and return true if any was processed. */
bool user_input_process(void *tinst) {

    if(&error_state == current_state) {
	/* Sleep a bit in the error state, before switching. */	
	usleep(500000);
	return false;
    }

    if(&status_state == current_state) {
	/* Return until the status state has completed its display. */
	return false;
    }

    int c = getch();

    if(ERR == c) {
	int flags, modflags, readerrno = 0;
	char tmp;
	ssize_t r;

	/* Get stdin's flags. */
	flags = fcntl(STDIN_FILENO, F_GETFL, 0);

	/* Make stdin non-blocking. */
	modflags = flags | O_NONBLOCK;

	if(-1 == fcntl(STDIN_FILENO, F_SETFL, modflags)) {
	    perror("fcntl");
	    exit(EXIT_FAILURE);
	}

	r = read(STDIN_FILENO, &tmp, sizeof(tmp));

	readerrno = errno;

	if(0 == r) {
	    /* An EOF occurred, so top should exit. */
	    exit(EXIT_FAILURE);
	}

	/* Restore stdin's old flags. */
	if(-1 == fcntl(STDIN_FILENO, F_SETFL, flags)) {
	    perror("fcntl");
	    exit(EXIT_FAILURE);
	}
	
	if(-1 == r && EAGAIN == readerrno) {
	    /*
	     * The read above returned an error, but it was due to the 
	     * O_NONBLOCK with no available data.
	     */
	    return false;
	}

	/* Set the character we read. */
	c = tmp;

	/* Fall through and handle the character. */
    }

    if(current_state) {
	if(/*del*/ 127 == c) {
	    if(current_state->offset > 0) {
		current_state->offset--;
		current_state->buf[current_state->offset] = '\0';
	    }
	} else if('\r' == c) {
	    current_state->buf[current_state->offset] = '\0';
	    current_state->completion(tinst, current_state);

	    /* 
	     * The user changing an interactive option triggers the display
	     * of new data.
	     */
	    top_insert(tinst);	
	    top_draw(tinst);
	} else {
 	    if(current_state->offset < (sizeof(current_state->buf) - 2)) {
		current_state->buf[current_state->offset++] = c;
		current_state->buf[current_state->offset] = '\0';
	    }
	}	

	return true;
    }

    switch(c) {
    case '?':
	user_input_set_state(&top_user_input_help_state);
	reset_state(current_state);
	break;

    case 'c':
	user_input_set_state(&top_user_input_mode_state);
	reset_state(current_state);
	break;

    case 'o':
	user_input_set_state(&top_user_input_order_state);
	reset_state(current_state);
	break;

    case 'O':
	user_input_set_state(&top_user_input_secondary_order_state);
	reset_state(current_state);
	break;
	
    case 'r':
	top_prefs_set_mmr(!top_prefs_get_mmr());
	
	user_input_set_status_state(top_prefs_get_mmr() ? 
				    "Report process memory object maps."
				    : 
				    "Do not report process memory object maps.");
	break;
	
    case 's':
	user_input_set_state(&top_user_input_sleep_state);
	reset_state(current_state);
	break;

    case 'S':
	user_input_set_state(&top_user_input_signal_state);
	reset_state(current_state);
	break;

    case 'U':
	user_input_set_state(&top_user_input_user_state);
	reset_state(current_state);
	break;

    case 'q':
    case 'Q':
	exit(EXIT_SUCCESS);
	break;

    default:
	if(current_state) {
	    reset_state(current_state);
	    return true;
	}
    }

    if(/*^L*/ '\x0c' == c || /*space*/ ' ' == c || '\r' == c)  {
	/* Trigger a new insert regardless of the interval. */
	top_insert(tinst);	
	top_draw(tinst);
    }
    
    return true;
}

static int row, column;
void user_input_set_position(int r, int c) {
    row = r;
    column = c;
}

void user_input_draw(void *tinst, WINDOW *win) {
    if(current_state && current_state->draw) {
	current_state->draw(tinst, current_state, win, row, column);
    }
}
