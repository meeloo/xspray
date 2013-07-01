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
#include <string.h>
#include <mach/mach_time.h>

#include "statistic.h"

#include "preferences.h"

#include "pid.h"
#include "pgrp.h"
#include "ppid.h"
#include "command.h"
#include "cpu.h"
#include "timestat.h"
#include "threads.h"
#include "ports.h"
#include "memstats.h"
#include "pstate.h"
#include "user.h"
#include "workqueue.h"
#include "uid.h"
#include "faults.h"
#include "messages.h"
#include "syscalls.h"
#include "csw.h"
#include "log.h"

struct statistic_name_map statistic_name_map[] = {
    /*The order of this must match the enum in statistic.h. */
    {STATISTIC_PID, top_pid_create, "PID"},
    {STATISTIC_COMMAND, top_command_create, "COMMAND"},
    {STATISTIC_CPU, top_cpu_create, "%CPU"},
    {STATISTIC_TIME, top_time_create, "TIME"},
    {STATISTIC_THREADS, top_threadcount_create, "#TH"},
    {STATISTIC_PORTS, top_ports_create, "#PORTS"},
    {STATISTIC_MREGION, top_mregion_create, "#MREGS"},
    {STATISTIC_RPRVT, top_rprvt_create, "RPRVT"},
    {STATISTIC_RSHRD, top_rshrd_create, "RSHRD"}, 
    {STATISTIC_RSIZE, top_rsize_create, "RSIZE"},
    {STATISTIC_VSIZE, top_vsize_create, "VSIZE"},
    {STATISTIC_VPRVT, top_vprvt_create, "VPRVT"},
    {STATISTIC_PGRP, top_pgrp_create, "PGRP"},
    {STATISTIC_PPID, top_ppid_create, "PPID"},
    {STATISTIC_PSTATE, top_pstate_create, "STATE"},
    {STATISTIC_UID, top_uid_create, "UID"},
    {STATISTIC_WORKQUEUE, top_workqueue_create, "#WQ"},
    {STATISTIC_FAULTS, top_faults_create, "FAULTS"},
    {STATISTIC_COW_FAULTS, top_cow_faults_create, "COW"},
    {STATISTIC_MESSAGES_SENT, top_messages_sent_create, "MSGSENT"},
    {STATISTIC_MESSAGES_RECEIVED, top_messages_received_create, "MSGRECV"},
    {STATISTIC_SYSBSD, top_sysbsd_create, "SYSBSD"},
    {STATISTIC_SYSMACH, top_sysmach_create, "SYSMACH"},
    {STATISTIC_CSW, top_csw_create, "CSW"},
    {STATISTIC_PAGEINS, top_pageins_create, "PAGEINS"},
    {STATISTIC_KPRVT, top_kprvt_create, "KPRVT"},
    {STATISTIC_KSHRD, top_kshrd_create, "KSHRD"},
    {STATISTIC_USER, top_user_create, "USER"},
    {0, NULL, NULL}
};

static void reset_insertion(struct statistics_controller *c) {
    struct statistic *s;
    int i;

    for(i = 0; i < c->get_total_possible(c); ++i) {
	s = c->state[i].instance;
	
	if(s) {
	    s->callbacks.reset_insertion(s);
	}
    }
}

static void insert_sample(struct statistics_controller *c, const void *sample) {
    struct statistic *s;
    int i;
   
    for(i = 0; i < c->get_total_possible(c); ++i) {
	s = c->state[i].instance;

	if(s) {
	    if(s->callbacks.insert_cell(s, sample))
		fprintf(stderr, "insert cell failed!\n");
	}
    }
}

static void remove_tail(struct statistics_controller *c) {
    int i;

    for(i = c->get_total_possible(c) - 1; i >= 0; --i) {
	if(c->state[i].instance && c->state[i].instance->visible) {
	    c->state[i].instance->visible = false;
	    hide_panel(c->state[i].instance->panel);
	    c->total_active_statistics--;
	    return;
	}	 
    }
}

/* This is used with the controller to insert a statistic. */
static void insert_tail(struct statistics_controller *c) {
    int i;

    for(i = 0; i < c->get_total_possible(c); ++i) {
	if(c->state[i].instance && !c->state[i].instance->visible) {
	    c->state[i].instance->visible = true;
	    show_panel(c->state[i].instance->panel);
	    top_panel(c->state[i].instance->panel);
	    c->total_active_statistics++;
	    return;
	}
    }
}

static int get_total_possible(struct statistics_controller *c) {
    return c->total_possible_statistics;
}

static int get_total_active(struct statistics_controller *c) {
    return c->total_active_statistics;
}

static void iterate(struct statistics_controller *c, 
		   bool (*func)(struct statistic *, void *),
		   void *ptr) {
    struct statistic *s;
    int i;
    bool log = top_prefs_get_logging_mode();

    for(i = 0; i <  c->get_total_possible(c); ++i) {
	s = c->state[i].instance;

	if(s && (s->visible || log)) {
	    if(func(s, ptr))
		continue;
	    
	    break;
	}
    }
}

struct statistics_controller *create_statistics_controller(WINDOW *parent) {
    struct statistics_controller *c;
    int i;
    int total = 0;
    int *array;
    
    c = malloc(sizeof *c);
    if(NULL == c)
	return NULL;
   
    c->parent = parent;

    for(i = 0; i < STATISTIC_TOTAL; ++i) {
	c->state[i].type = i;
	c->state[i].instance = NULL;
	c->state[i].create_statistic = NULL;
    }

    top_prefs_get_stats(&total, &array);

    for(i = 0; i < total; ++i) {
	c->state[i].type = array[i];
	c->state[i].create_statistic = statistic_name_map[array[i]].creator;
	strcpy(c->state[i].name, statistic_name_map[array[i]].name);
    }

    c->total_possible_statistics = total;
    c->total_active_statistics = 0;

    c->reset_insertion = reset_insertion;
    c->insert_sample = insert_sample;
    c->remove_tail = remove_tail;
    c->insert_tail = insert_tail;
    c->get_total_possible = get_total_possible;
    c->get_total_active = get_total_active;
    c->iterate = iterate;

    for(i = 0; i < c->get_total_possible(c); ++i) {
	c->state[i].instance = 
	    c->state[i].create_statistic(c->parent, c->state[i].name);

	if(NULL == c->state[i].instance) {
	    free(c);
	    return NULL;
	}
	
	c->state[i].instance->controller = c;
    }

    return c;
}


/* Return NULL if an error occurred. */
struct statistic *create_statistic(int type, WINDOW *parent, void *ptr, 
				   struct statistic_callbacks *callbacks,
				   const char *name) {
    struct statistic *s;
    bool log = top_prefs_get_logging_mode();    

    s = malloc(sizeof *s);

    if(NULL == s) {
	perror("malloc in create_statistic");
	return NULL;
    }

    if(!log) {
	s->parent = parent;
	s->window = newwin(1, 1, 0, 0);

	if(NULL == s->window) {
	    free(s);
	    return NULL;
	}

	s->panel = new_panel(s->window);
	
	if(NULL == s->panel) {
	    delwin(s->window);
	    free(s);
	    return NULL;
	}
    } else {
	s->parent = NULL;
	s->window = NULL;
	s->panel = NULL;
    }

    s->type = type;
    s->cells = NULL;
    s->ptr = ptr;
    s->header = strdup(name);

    if(NULL == s->header) {
	if(!log) {
	    del_panel(s->panel);
	    delwin(s->window);
	}

	free(s);
	return NULL;
    }

    s->visible = false;

    if(!log)
	hide_panel(s->panel);

    s->time_consumed = 0;
    s->runs = 0;

    s->callbacks = *callbacks;
    s->destructors = NULL;
    s->request_size.width = 0;
    s->request_size.height = 0;
    s->actual_size.width = 0;
    s->actual_size.height = 0;

    s->minimum_size.width = 1;
    s->minimum_size.height = 1;

    s->previous = NULL;
    s->next = NULL;

    return s;
}

void destroy_statistic(struct statistic *s) {
    struct statistic_destructor *dtor, *dtornext;


    for(dtor = s->destructors; dtor; ) {
	dtornext = dtor->next;
	dtor->destructor(s, dtor->ptr);
	free(dtor);
	dtor = dtornext;
    }

    werase(s->window);
    del_panel(s->panel);
    delwin(s->window);
       
    if(s->header)
	free(s->header);

    free(s);    
}

/* Return true if an error occurred. */
bool create_statistic_destructor(struct statistic *s,
				 void (*destructor)(struct statistic *, void *),
				 void *ptr) {
    struct statistic_destructor *dtor;

    dtor = malloc(sizeof *dtor);
    if(NULL == dtor) {
	perror("malloc in create_statistic_destructor");
	return true;
    }

    dtor->destructor = destructor;
    dtor->ptr = ptr;
    dtor->next = s->destructors;
    
    s->destructors = dtor;

    return false;
}
