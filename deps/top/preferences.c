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
#include <signal.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>
#include <libproc.h>
#include "preferences.h"
#include "statistic.h"
#include "top.h"

static struct {
    int mode;
    int sort_by;
    int secondary_sort;
    bool sort_ascending;
    bool secondary_sort_ascending;
    int sleep_seconds;
    bool frameworks;
    int frameworks_interval;
    char *user;
    uid_t uid;
    int samples;
    int nprocs;
    bool have_pid;
    pid_t pid;
    bool logging_mode;
    bool have_ncols;
    int ncols;
    bool show_swap;
    bool mmr; /* memory map reporting */
    bool delta_forced_mmr; /* delta mode forced mmr */

    /* These are initialized in top_prefs_init() below. */
    struct {
	int total;
	int array[STATISTIC_TOTAL];
    } display_stats;

    const char *signal_string;
    int signal_number;   
} prefs = {
    .mode = STATMODE_NON_EVENT, 
    .sort_by = STATISTIC_PID,
    .secondary_sort = STATISTIC_PID,
    .sort_ascending = false,
    .secondary_sort_ascending = false,
    .sleep_seconds = 1,
    .frameworks = true,
    .frameworks_interval = 10,
    .user = NULL,
    .uid = 0,
    .samples = -1,
    .nprocs = -1,
    .have_pid = false,
    .pid = 0,
    .logging_mode = false,
    .have_ncols = false,
    .ncols = -1,
    .show_swap = false,
    .mmr = true,
    .delta_forced_mmr = false
};

static struct {
    const char *string;
    int e;
} stat_map[] = {
    {"pid", STATISTIC_PID},
    {"command", STATISTIC_COMMAND},
    {"cpu", STATISTIC_CPU},
    {"csw", STATISTIC_CSW},
    {"time", STATISTIC_TIME},
    /*alias*/
    {"threads", STATISTIC_THREADS},
    {"th", STATISTIC_THREADS},
    /*alias*/
    {"ports", STATISTIC_PORTS},
    {"prt", STATISTIC_PORTS},
    /*alias*/
    {"mregion", STATISTIC_MREGION},
    /*alias*/
    {"mreg", STATISTIC_MREGION},
    /*alias*/
    {"mregs", STATISTIC_MREGION},
    {"reg", STATISTIC_MREGION},
    {"rprvt", STATISTIC_RPRVT},
    {"rshrd", STATISTIC_RSHRD},
    {"rsize", STATISTIC_RSIZE},
    {"vsize", STATISTIC_VSIZE},
    {"vprvt", STATISTIC_VPRVT},
    {"pgrp", STATISTIC_PGRP},
    {"ppid", STATISTIC_PPID},
    {"state", STATISTIC_PSTATE},
    {"pstate", STATISTIC_PSTATE},
    {"uid", STATISTIC_UID},
    {"wq", STATISTIC_WORKQUEUE},
    /*alias*/
    {"#wq", STATISTIC_WORKQUEUE},
    /*alias*/
    {"workqueue", STATISTIC_WORKQUEUE},
    {"faults", STATISTIC_FAULTS},
    /*alias*/
    {"fault", STATISTIC_FAULTS},
    {"cow", STATISTIC_COW_FAULTS},
    /*alias*/
    {"cow_faults", STATISTIC_COW_FAULTS},
    {"user", STATISTIC_USER},
    {"username", STATISTIC_USER},
    {"msgsent", STATISTIC_MESSAGES_SENT},
    {"msgrecv", STATISTIC_MESSAGES_RECEIVED},
    {"sysbsd", STATISTIC_SYSBSD},
    {"sysmach", STATISTIC_SYSMACH},
    {"pageins", STATISTIC_PAGEINS},
    {"kprvt", STATISTIC_KPRVT},
    {"kshrd", STATISTIC_KSHRD},
    {NULL, 0}
};

static struct {
    const char *string;
    int value;
} signal_map[] = {
    {"HUP", SIGHUP},
    {"INT", SIGINT},
    {"QUIT", SIGQUIT},
    {"ILL", SIGILL},
    {"TRAP", SIGTRAP},
    {"ABRT", SIGABRT},
    {"IOT", SIGIOT},
    {"EMT", SIGEMT},
    {"FPE", SIGFPE},
    {"KILL", SIGKILL},
    {"BUS", SIGBUS},
    {"SEGV", SIGSEGV},
    {"SYS", SIGSYS},
    {"PIPE", SIGPIPE},
    {"ALRM", SIGALRM},
    {"TERM", SIGTERM},
    {"URG", SIGURG},
    {"STOP", SIGSTOP},
    {"TSTP", SIGTSTP},
    {"CONT", SIGCONT},
    {"CHLD", SIGCHLD},
    {"TTIN", SIGTTIN},
    {"TTOU", SIGTTOU},
    {"IO", SIGIO},
    {"XCPU", SIGXCPU},
    {"XFSZ", SIGXFSZ},
    {"VTALRM", SIGVTALRM},
    {"PROF", SIGPROF},
    {"WINCH", SIGWINCH},
    {"INFO", SIGINFO},
    {"USR1", SIGUSR1},
    {"USR2", SIGUSR2},
    {NULL, -1}
};

void top_prefs_init(void) {
    int i;

    prefs.display_stats.total = 0;

#define SPREF(e) do { \
	assert(prefs.display_stats.total < STATISTIC_TOTAL); \
	prefs.display_stats.array[prefs.display_stats.total] = e; \
	prefs.display_stats.total++;				  \
    } while(0)
    
    SPREF(STATISTIC_PID);
    SPREF(STATISTIC_COMMAND);
    SPREF(STATISTIC_CPU);
    SPREF(STATISTIC_TIME);
    SPREF(STATISTIC_THREADS);
#ifdef PROC_PIDWORKQUEUEINFO
    SPREF(STATISTIC_WORKQUEUE);
#endif
    SPREF(STATISTIC_PORTS);
    SPREF(STATISTIC_MREGION);
    SPREF(STATISTIC_RPRVT);
    SPREF(STATISTIC_RSHRD);
    SPREF(STATISTIC_RSIZE);
    SPREF(STATISTIC_VPRVT);
    SPREF(STATISTIC_VSIZE);
    SPREF(STATISTIC_PGRP);
    SPREF(STATISTIC_PPID);
    SPREF(STATISTIC_PSTATE);
    SPREF(STATISTIC_UID);
    SPREF(STATISTIC_FAULTS);
    SPREF(STATISTIC_COW_FAULTS);
    SPREF(STATISTIC_MESSAGES_SENT);
    SPREF(STATISTIC_MESSAGES_RECEIVED);
    SPREF(STATISTIC_SYSBSD);
    SPREF(STATISTIC_SYSMACH);
    SPREF(STATISTIC_CSW);
    SPREF(STATISTIC_PAGEINS);
    SPREF(STATISTIC_KPRVT);
    SPREF(STATISTIC_KSHRD);
    SPREF(STATISTIC_USER);

#undef SPREF

    for(i = 0; signal_map[i].string; ++i) {
	if(SIGTERM == signal_map[i].value) {
	    prefs.signal_string = signal_map[i].string;
	    prefs.signal_number = SIGTERM;
	    break;
	}
    }
}

/* MODE */
static struct {
    const char *string;
    int e; /*enum*/
} mode_map[] = {
    {"a", STATMODE_ACCUM},
    {"d", STATMODE_DELTA},
    {"e", STATMODE_EVENT},
    {"n", STATMODE_NON_EVENT},
    {NULL, 0}
};

/* Return true if an error occurred. */
bool top_prefs_set_mode(const char *mode) {
    int i;
    
    for(i = 0; mode_map[i].string; ++i) {
	if(!strcmp(mode, mode_map[i].string)) {
	    prefs.mode = mode_map[i].e;
	    
	    if(STATMODE_DELTA == prefs.mode) {
		if(top_prefs_get_mmr()) {
		    /*
		     * By default we turn off memory map reporting (mmr)
		     * in delta mode.  It uses too much CPU time in
		     * some cases.  Users can re-enable the mmr option
		     * interactively or after specifying the mode at the
		     * command line with -r after -c d.
		     *
		     * The effected stats should display N/A.
		     */
		    top_prefs_set_mmr(false);
		    prefs.delta_forced_mmr = true;
		}
	    } else {
		if(prefs.delta_forced_mmr) {
		    /* Delta mode forced the mmr off. */
		    top_prefs_set_mmr(true);
		    prefs.delta_forced_mmr = false;
		}
	    }
	    
	    return false;
	}
    }
    
    return true; /*error*/
}

int top_prefs_get_mode(void) {
    return prefs.mode;
}

const char *top_prefs_get_mode_string(void) {
    int i;

    for(i = 0; mode_map[i].string; ++i) {
	if(prefs.mode == mode_map[i].e)
	    return mode_map[i].string;
    }

    return NULL;
}


/* SLEEP */
void top_prefs_set_sleep(int seconds) {
    prefs.sleep_seconds = seconds;
}

int top_prefs_get_sleep(void) {
    return prefs.sleep_seconds;
}

/* Specify NULL for ascending if the sort shouldn't allow a + or - prefix. */
/* This returns true when it has found the matching enum value. */
/* NOTE: The caller should initialize ascending to avoid an invalid value. */
static bool find_sort(const char *sortkey, int *e, bool *ascending) {
    int i;

    if(ascending) {
	if('+' == sortkey[0]) {
	    *ascending = true;
	    ++sortkey;
	} else if('-' == sortkey[0]) {
	    *ascending = false;
	    ++sortkey;
	} 
    }

    for(i = 0; stat_map[i].string; ++i) {
	if(!strcasecmp(sortkey, stat_map[i].string)) {
	    *e = stat_map[i].e;
	    return true;
	}
    }

    return false;
}

/*Return true if an error occurred.*/
bool top_prefs_set_sort(const char *sortkey) {
    bool ascending;
    int e;
       
    ascending = top_prefs_get_ascending();

    if(find_sort(sortkey, &e, &ascending)) {
	top_prefs_set_ascending(ascending);
	prefs.sort_by = e;
	return false;
    }

    /* Not found -- error */
    return true;
}

int top_prefs_get_sort(void) {
    return prefs.sort_by;
}

/* Return true if an error occurred. */
bool top_prefs_set_secondary_sort(const char *sortkey) {
    bool ascending;
    int e;

    ascending = top_prefs_get_secondary_ascending();

    if(find_sort(sortkey, &e, &ascending)) {
	top_prefs_set_secondary_ascending(ascending);
	prefs.secondary_sort = e;
	return false;
    }

    return true;
}


int top_prefs_get_secondary_sort(void) {
    return prefs.secondary_sort;
}

static const char *enum_value_to_sort_string(int e) {
    int i;

    for(i = 0; stat_map[i].string; ++i) {
	if(e == stat_map[i].e) {
	    return stat_map[i].string;
	}
    }
  
    fprintf(stderr, "Invalid enum value: %d in %s!\n", e, __func__);
    abort(); 
}

const char *top_prefs_get_sort_string(void) {
    return enum_value_to_sort_string(prefs.sort_by);
}

const char *top_prefs_get_secondary_sort_string(void) {
    return enum_value_to_sort_string(prefs.secondary_sort);
}

/* ASCENDING/DESCENDING */

void top_prefs_set_ascending(bool flag) {
    prefs.sort_ascending = flag;
}

bool top_prefs_get_ascending(void) {
    return prefs.sort_ascending;
}

void top_prefs_set_secondary_ascending(bool flag) {
    prefs.secondary_sort_ascending = flag;
}

bool top_prefs_get_secondary_ascending(void) {
    return prefs.secondary_sort_ascending;
}

void top_prefs_set_frameworks(bool flag) {
    prefs.frameworks = flag;
}

bool top_prefs_get_frameworks(void) {
    return prefs.frameworks;
}

void top_prefs_set_frameworks_interval(int interval) {
    prefs.frameworks_interval = interval;
}

int top_prefs_get_frameworks_interval(void) {
    return prefs.frameworks_interval;
}

void top_prefs_set_user(const char *user) {
    free(prefs.user);
    prefs.user = NULL;

    if(strlen(user)) {
	prefs.user = strdup(user);
    } 
}

char *top_prefs_get_user(void) {
    return prefs.user;
}

void top_prefs_set_user_uid(uid_t uid) {
    prefs.uid = uid;
}

uid_t top_prefs_get_user_uid(void) {
    return prefs.uid;
}

/* Return true if found. */
static bool find_stat_enum(const char *key, int *e) {
    int i;

    for(i = 0; stat_map[i].string; ++i) {
	if(!strcasecmp(key, stat_map[i].string)) {
	    *e = stat_map[i].e;
	    return true;
	}
    }

    return false;
}

/* Take a comma separated list of names. */
/* Return true if an error occurred. */
bool top_prefs_set_stats(const char *names) {
    char key[20];
    int key_offset = 0;
    const char *np;
    int stat_enum_array[STATISTIC_TOTAL];
    int stat_enum_array_offset = 0;
    int e, i;

    for(np = names; *np; ++np) {
	if(isspace(*np))
	    continue;

	/* Check for a comma separating the keys. */
	if(',' == *np) { 
	    key[key_offset++] = '\0';

	    if(!find_stat_enum(key, &e)) {
		fprintf(stderr, "invalid stat: %s\n", key);
		return true;
	    } 

	    if(stat_enum_array_offset >= STATISTIC_TOTAL) {
		fprintf(stderr, "too many stats specified.\n");
		return true;
	    }	   
 
	    stat_enum_array[stat_enum_array_offset++] = e;

	    key_offset = 0;
	} else {
	    key[key_offset++] = *np;
	    
	    /* Check if we would exceed the length of the buffer. */
	    if(key_offset >= (sizeof(key) - 1)) {
		fprintf(stderr, "invalid input: longer than any valid stat.\n");
		return true;
	    }
	}
    }

    /* See if we had a trailing key without a comma. */
    if(key_offset > 0) {
	key[key_offset++] = '\0';
	
	if(!find_stat_enum(key, &e)) {
	    fprintf(stderr, "invalid stat: %s\n", key);
	    return true;
	}

	if(stat_enum_array_offset >= STATISTIC_TOTAL) {
	    fprintf(stderr, "too many stats specified.\n");
	    return true;
	}

	stat_enum_array[stat_enum_array_offset++] = e;
	
	key_offset = 0;
    }

    /* See if we had no keys at all. */
    if(stat_enum_array_offset <= 0) {
	fprintf(stderr, "invalid input: %s\n", names);
 	return true;
    }

    /* Now set the stats. */
    for(i = 0; i < stat_enum_array_offset; ++i) {
	prefs.display_stats.array[i] = stat_enum_array[i];
    }

    prefs.display_stats.total = stat_enum_array_offset;

    return false;
}

/* Return true if able to get the stats. */
bool top_prefs_get_stats(int *total, int **array) {
    *total = prefs.display_stats.total;
    *array = prefs.display_stats.array;

    return true;
}

int top_prefs_get_samples(void) {
    return prefs.samples;
}

void top_prefs_set_samples(int s) {
    prefs.samples = s;
}

int top_prefs_get_nprocs(void) {
    return prefs.nprocs;
}

void top_prefs_set_nprocs(int n) {
    prefs.nprocs = n;
}

void top_prefs_set_pid(pid_t pid) {
    prefs.have_pid = true;
    prefs.pid = pid;
}

bool top_prefs_get_pid(pid_t *pidptr) {
    if(prefs.have_pid) {
	*pidptr = prefs.pid;
	return true;
    }

    return false;
}

/* Return true if the signal string is invalid. */
bool top_prefs_set_signal_string(char *s) {
    int i;

    for(i = 0; signal_map[i].string; ++i) {
	if(!strcasecmp(signal_map[i].string, s)) {
	    prefs.signal_string = signal_map[i].string;
	    prefs.signal_number = signal_map[i].value;
	    return false;
	}
    }

    return true;
}

int top_prefs_get_signal(const char **sptr) {
    *sptr = prefs.signal_string;

    return prefs.signal_number;
}

void top_prefs_set_logging_mode(bool mode) {
    prefs.logging_mode = mode;
}

bool top_prefs_get_logging_mode(void) {
    return prefs.logging_mode;
}

void top_prefs_set_ncols(int limit) {
    prefs.have_ncols = true;
    prefs.ncols = limit;
}

bool top_prefs_get_ncols(int *limit) {
    if(prefs.have_ncols) {
	*limit = prefs.ncols;
	return true;
    }

    return false;
}

void top_prefs_set_swap(bool show) {
    prefs.show_swap = show;
}

bool top_prefs_get_swap(void) {
    return prefs.show_swap;
}

void top_prefs_set_mmr(bool mmr) {
    prefs.mmr = mmr;
}

bool top_prefs_get_mmr(void) {
    return prefs.mmr;
}
