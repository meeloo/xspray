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
#include <assert.h>
#include <ctype.h>
#include <pwd.h>
#include "options.h"
#include "preferences.h"

#include "log.h"

enum {
    OPT_COUNTING = 1,
    OPT_FRAMEWORK_OFF,
    OPT_FRAMEWORK_ON,
    OPT_HELP,
    OPT_ORDER,
    OPT_SECONDARY_ORDER,
    OPT_SLEEP,
    OPT_INTERVAL,
    OPT_SAMPLES,
    OPT_NCOLS,
    OPT_NPROCS,
    OPT_STATS,
    OPT_PID,
    OPT_USER,
    OPT_DEBUGLOG,
    OPT_U_SORT,
    OPT_SWAP,
    OPT_MMR_OFF,
    OPT_MMR_ON,
    /*compat/deprecated options*/
    OPT_ACCUM,
    OPT_DELTA,
    OPT_ABSOLUTE
};

enum {
    TOP_OPTION_REQUIRED = 1, /* A -flag value pair is specified as required. */
    TOP_OPTION_SET /* The option is a boolean to enable/disable something. */
};


struct top_option {
    const char *option_string;
    int option_flag;
    int option_value; 
};

/* Return true if found, and false if not. */
/* The caller of this is expected to have initialized the *offset value to a valid argv array offset. */
bool top_option_get(int argc, char *argv[], struct top_option *opts, int *offset, int *optvalue, char **argresult) {
    int opti;
    
    assert(*offset < argc);

    /* Set the argument result (AKA the option value) to NULL. */
    *argresult = NULL;
    *optvalue = -1;
    
    for(opti = 0; opts[opti].option_string; ++opti) {
	if(TOP_OPTION_REQUIRED == opts[opti].option_flag) {
	    /* First check for an exact match. */
	    /* Otherwise check for a single char option with a value like -n4. */
	    if(!strcmp(argv[*offset], opts[opti].option_string)) {
		if((*offset + 1) >= argc) {
		    /* The option was something like -stats without a value. */
		    return false;
		}
		*argresult = argv[*offset + 1];
		*optvalue = opts[opti].option_value;
		*offset += 2;
		return true;
	    } else if(2 == strlen(opts[opti].option_string) && strlen(argv[*offset]) >= 2
		      && opts[opti].option_string[0] == argv[*offset][0]
		      && opts[opti].option_string[1] == argv[*offset][1]) {
		/* We found a pattern like -n123 and the argresult should be 123. */
		*argresult = argv[*offset] + 2;
		*optvalue = opts[opti].option_value;
		*offset += 1;
		return true;
	    }
	} else {
	    /* TOP_OPTION_SET */
	    if(!strcmp(argv[*offset], opts[opti].option_string)) {
		*optvalue = opts[opti].option_value;
		*offset += 1;
		return true;
	    }
	}
    }
    
    return false;
}

/* Note: it's important that options with the same prefix have the long option in this struct first. */
static struct top_option opts[] = {
    {"-stats", TOP_OPTION_REQUIRED, OPT_STATS},
    {"-ncols", TOP_OPTION_REQUIRED, OPT_NCOLS},
    {"-pid", TOP_OPTION_REQUIRED, OPT_PID},
    {"-user", TOP_OPTION_REQUIRED, OPT_USER},
    {"-c", TOP_OPTION_REQUIRED, OPT_COUNTING},
    {"-F", TOP_OPTION_SET, OPT_FRAMEWORK_OFF},
    {"-f", TOP_OPTION_SET, OPT_FRAMEWORK_ON},
    {"-h", TOP_OPTION_SET, OPT_HELP},
    {"-o", TOP_OPTION_REQUIRED, OPT_ORDER},
    {"-O", TOP_OPTION_REQUIRED, OPT_SECONDARY_ORDER},
    {"-s", TOP_OPTION_REQUIRED, OPT_SLEEP},
    {"-i", TOP_OPTION_REQUIRED, OPT_INTERVAL},
    {"-l", TOP_OPTION_REQUIRED, OPT_SAMPLES},
    {"-n", TOP_OPTION_REQUIRED, OPT_NPROCS},
    {"-U", TOP_OPTION_REQUIRED, OPT_USER},
    {"-u", TOP_OPTION_SET, OPT_U_SORT},
    {"-S", TOP_OPTION_SET, OPT_SWAP},
    {"-R", TOP_OPTION_SET, OPT_MMR_OFF},
    {"-r", TOP_OPTION_SET, OPT_MMR_ON},
    /*compat/deprecated options*/
    {"-a", TOP_OPTION_SET, OPT_ACCUM},
    {"-d", TOP_OPTION_SET, OPT_DELTA},
    {"-e", TOP_OPTION_SET, OPT_ABSOLUTE},
    {NULL, 0, 0}
};

void top_options_init(void) {
    /* do nothing */
}

void top_options_usage(FILE *fp, char *argv0) {
    fprintf(fp, 
	    "%s usage: %s\n"
	    "\t\t[-a | -d | -e | -c <mode>]\n"
	    "\t\t[-F | -f]\n"
	    "\t\t[-h]\n"
	    "\t\t[-i <interval>]\n"
	    "\t\t[-l <samples>]\n"
	    "\t\t[-ncols <columns>]\n"
	    "\t\t[-o <key>] [-O <secondaryKey>]\n"
	    "\t\t[-R | -r]\n"
	    "\t\t[-S]\n"
	    "\t\t[-s <delay>]\n"
	    "\t\t[-n <nprocs>]\n"
	    "\t\t[-stats <key(s)>]\n"
	    "\t\t[-pid <processid>]\n"
	    "\t\t[-user <username>]\n"
	    "\t\t[-U <username>]\n"
	    "\t\t[-u]\n",
	    argv0, argv0);
    
    fprintf(fp, "\n");
}

static bool string_is_integer(const char *s) {
    const char *b = s;
    bool indicator = false;

    /*skip whitespace*/
    for(; *s && isspace((int)*s); ++s)
	/*empty body*/;

    if('-' == *s || '+' == *s) {
	++s;
	indicator = true;
    }

    for(; *s && isdigit((int)*s); ++s)
	/*empty body*/;

    /* 
     * At this point we could have trailing whitespace, but
     * in the use case that is not a real problem.  If this is
     * reused we might want the whitespace handled here.
     */

    if('\0' == *s && b != s) {
	/* Check if it was just - or + */
	if(indicator) {
	    if((s - b) > 1) {
		return true;
	    } else {
		return false;
	    }
	} else {
	    return true;
	}
    }
    
    return false;
}

/* Return true if an error occurred. */
bool top_options_parse(int argc, char *argv[]) {
    int offset = 1;

    while(offset < argc) {
	char *optarg;
	int optvalue;

	if(false == top_option_get(argc, argv, opts, &offset, &optvalue, &optarg)) {
	    fprintf(stderr, "invalid option or syntax: %s\n", argv[offset]);
	    return true;
	}
	
	switch(optvalue) {
	case OPT_COUNTING:
	    if(top_prefs_set_mode(optarg)) {
		fprintf(stderr, "invalid argument for -c: %s\n", optarg);
		return true;
	    }
	    break;

	case OPT_FRAMEWORK_OFF:
	    top_prefs_set_frameworks(false);
	    break;

	case OPT_FRAMEWORK_ON:
	    top_prefs_set_frameworks(true);
	    break;
	    
	case OPT_HELP:
	    top_options_usage(stdout, argv[0]);
	    exit(EXIT_SUCCESS);
	    break;	    

	case OPT_INTERVAL: {
	    int n;

	    if(!string_is_integer(optarg)) {
		fprintf(stderr,
			"invalid interval number (not an integer): %s\n",
			optarg);
		return true;
	    }

	    n = atoi(optarg);

	    if(n < 1) {
		fprintf(stderr, "invalid argument for -i: %s\n", optarg);
		return true;
	    }
	    
	    top_prefs_set_frameworks_interval(n);
	}
	    break;

	case OPT_SAMPLES: {
	    int s;

	    if(!string_is_integer(optarg)) {
		fprintf(stderr, 
			"invalid number of samples (not an integer): %s\n",
			optarg);
		return true;
	    }

	    s = atoi(optarg);

	    if(s < 0) {
		fprintf(stderr, "invalid number of samples: %d\n", s);
		return true;
	    }

	    top_prefs_set_samples(s);
	}
	    break;

	case OPT_NCOLS: {
	    int n;

	    if(!string_is_integer(optarg)) {
		fprintf(stderr,
			"invalid argument for -ncols (not an integer): %s\n",
			optarg);
		return true;
	    }

	    n = atoi(optarg);

	    if(n < 1) {
		fprintf(stderr, "-ncols requires an argument > 0\n");
		return true;
	    }

	    top_prefs_set_ncols(n);
	}
	    break;

	case OPT_NPROCS: {
	    int n;

	    if(!string_is_integer(optarg)) {
		fprintf(stderr,
			"invalid argument for -n (not an integer): %s\n",
			optarg);
		return true;
	    }

	    n = atoi(optarg);
	    
	    if(n < 0) {
		fprintf(stderr, "-n argument may not be negative: %s\n",
			optarg);
		return true;
	    }

	    top_prefs_set_nprocs(n);
	}
	    break;

	case OPT_ORDER:
	    if(top_prefs_set_sort(optarg)) {
		fprintf(stderr, "invalid argument -o: %s\n", optarg);
 		return true;
	    }
	    break;
	    
	case OPT_SECONDARY_ORDER:
	    if(top_prefs_set_secondary_sort(optarg)) {
		fprintf(stderr, "invalid argument -O: %s\n", optarg);
		return true;
	    }
	    break;
	    
	case OPT_SLEEP: {
	    int s;

	    if(!string_is_integer(optarg)) {
		fprintf(stderr, 
			"invalid argument for sleep interval (not an integer):"
			" %s\n",
			optarg);
		return true;
	    }

	    s = atoi(optarg);

	    if(s < 0) {
		fprintf(stderr, "invalid argument for -s: %s\n", optarg);
		return true;
	    }

	    top_prefs_set_sleep(s);
	}
	    break;
	    	    
	case OPT_STATS:
	    if(top_prefs_set_stats(optarg)) {
		fprintf(stderr, "invalid argument for -stats: %s\n", optarg);
		return true;
	    }
	    break;

	case OPT_PID: {
	    pid_t p;

	    if(!string_is_integer(optarg)) {
		fprintf(stderr,
			"invalid -pid argument (not an integer): %s\n", optarg);
		return true;
	    }

	    p = atoi(optarg);

	    if(p < 0) {
		fprintf(stderr, "pid arguments can not be < 0: %s\n",
			optarg);
		return true;
	    }

	    top_prefs_set_pid(p);
	}
	    break;

	case OPT_USER: {
	    struct passwd *pw;
	    
	    pw = getpwnam(optarg);

	    if(NULL == pw) {
		endpwent();
		fprintf(stderr, "invalid user: %s\n", optarg);
		return true;
	    }

	    top_prefs_set_user(optarg);
	    top_prefs_set_user_uid(pw->pw_uid);

	    endpwent();
	}
	    break;

	case OPT_U_SORT: {
	    if(top_prefs_set_sort("cpu") 
	       || top_prefs_set_secondary_sort("time")) {
		fprintf(stderr, 
			"An unexpected error occurred while performing the -u "
			"preference setting.\n");
		abort();
	    }
	}
	    break;

	case OPT_MMR_OFF:
	    top_prefs_set_mmr(false);
	    break;

	case OPT_MMR_ON:
	    top_prefs_set_mmr(true);
	    break;

	case OPT_SWAP:
	    top_prefs_set_swap(true);
     	    break;

	    /*compat/deprecated options*/
	case OPT_ACCUM:
	    if(top_prefs_set_mode("a")) {
		fprintf(stderr,
			"An internal top error has occurred unexpectedly!\n");
		abort();
	    }
	    break;
	    
	case OPT_DELTA:
	    if(top_prefs_set_mode("d")) {
		fprintf(stderr,
			"An internal top error has occurred unexpectedly!\n");
		abort();
	    }
	    break;

	case OPT_ABSOLUTE:
	    if(top_prefs_set_mode("e")) {
		fprintf(stderr,
			"An internal top error has occurred unexpectedly!\n");
		abort();
	    }
	    break;
	} /*end switch*/
    }

    return false;
}
