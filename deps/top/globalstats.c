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
#include <libutil.h>
#include <inttypes.h>
#include <panel.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "libtop.h"
#include "globalstats.h"
#include "generic.h"
#include "log.h"
#include "preferences.h"
#include "uinteger.h"
#include "top.h"

enum { 
    GLOBALSTAT_PROCESSES, GLOBALSTAT_TIME, GLOBALSTAT_LOAD, 
    GLOBALSTAT_CPU, GLOBALSTAT_SHAREDLIBS,
    GLOBALSTAT_MEMREGIONS, GLOBALSTAT_PHYSMEM,
    GLOBALSTAT_VM, GLOBALSTAT_SWAP, GLOBALSTAT_PURGEABLE,
    GLOBALSTAT_NETWORKS, GLOBALSTAT_DISKS
};

enum { GLOBALSTAT_TOTAL = GLOBALSTAT_DISKS + 1 };

enum { GLOBALSTAT_HN_FORMAT = HN_NOSPACE | HN_B };

struct globalstat_size {
    int width, height;
};

struct globalstat {
    WINDOW *window;
    PANEL *panel;
    int type;
    char data[120];
    int length;
    int x, y;
    struct globalstat_size size;
    int max_width;
};

struct globalstat_controller {
    struct globalstat stats[GLOBALSTAT_TOTAL];
    WINDOW *window;
    PANEL *panel;
};

/* Return -1 if an error occurred. */
static int humanize_globalstat(char *out, size_t s, int64_t n) {
    return humanize_number(out, s, n, "", HN_AUTOSCALE, GLOBALSTAT_HN_FORMAT);
}

static bool init_globalstat(WINDOW *parent, struct globalstat *gs, int type) {
 
    if(!top_prefs_get_logging_mode()) {
	gs->window = newwin(1, 1, 0, 0);
	
	if(NULL == gs->window)
	    return true;

	gs->panel = new_panel(gs->window);
	
	if(NULL == gs->panel) {
	    delwin(gs->window);
	    return true;
	}
    }

    gs->type = type;
    gs->length = 0;
    gs->x = 0;
    gs->y = 0;
    gs->size.width = 0;
    gs->size.height = 0;
    gs->max_width = 0;

    return false;
};

static void update_max(struct globalstat *gs) {
    if(gs->length > gs->max_width) {
	gs->max_width = gs->length;
	top_relayout_force();
    }
}

static int get_globalstat_reqwidth(struct globalstat *gs) {
    return gs->length + 1;
}

static void set_globalstat_geometry(struct globalstat *gs, int x, int y,
				   int width, int height) {
    gs->x = x;
    gs->y = y;
    gs->size.width = width;
    gs->size.height = height;
}

void *top_globalstats_create(WINDOW *parent) {
    struct globalstat_controller *c;
    int i;
    
    c = malloc(sizeof *c);    

    if(NULL == c)
	return NULL;
   
        
    if(!top_prefs_get_logging_mode()) {
	c->window = newwin(1, 1, 0, 0);
	
	if(NULL == c->window) {
	    free(c);
	    return NULL;
	}
	
	c->panel = new_panel(c->window);
	
	if(NULL == c->panel) {
	    delwin(c->window);
	    free(c);
	    return NULL;
	}
    } else {
	c->window = NULL;
	c->panel = NULL;
    }

    for(i = 0; i < GLOBALSTAT_TOTAL; ++i) {
	if(GLOBALSTAT_SHAREDLIBS == i && !top_prefs_get_frameworks()) {
	    /* Skip the framework update. */
	    continue;
	}

	if(init_globalstat(c->window, c->stats + i, i)) {
	    --i;
	    /* Cleanup the already created windows. */
	    for(; i >= 0; --i) {
		if(!top_prefs_get_logging_mode())
		    delwin(c->stats[i].window);
	    }
	    free(c);
	    return NULL;
	}
    }

    return c;
}

void top_globalstats_draw(void *ptr) {
    struct globalstat_controller *c = ptr;
    int i;
    
    for(i = 0; i < GLOBALSTAT_TOTAL; ++i) {
		// MWW: Skip the frameworks window if we've got them disabled
		if (!top_prefs_get_frameworks() && (i == GLOBALSTAT_SHAREDLIBS)) 
			continue;
		if(strlen(c->stats[i].data))
	    	mvwaddstr(c->stats[i].window, 0, 0, c->stats[i].data);
    }
}

static void reset_globalstat(struct globalstat *gs) {
    gs->data[0] = '\0';
    gs->length = 0;
}


static void update_time(struct globalstat *gs,
			const libtop_tsamp_t *tsamp) {
    struct timeval tval;
    struct tm tm;
    
    gettimeofday(&tval, NULL);
    localtime_r(&(tval.tv_sec), &tm);

    if(STATMODE_ACCUM == top_prefs_get_mode()) {
	unsigned int sec, min, hour;

	timersub(&tsamp->time, &tsamp->b_time, &tval);

	sec = tval.tv_sec;
	min = sec / 60;
	hour = min / 60;

	gs->length = snprintf(gs->data, sizeof(gs->data), "%u:%02u:%02u",
			      hour, min % 60, sec % 60);

	if(gs->length < 0) {
	    reset_globalstat(gs);
	    return;
	}
    } else if(top_prefs_get_logging_mode()) {
	gs->length = strftime(gs->data, sizeof(gs->data), "%Y/%m/%d %T", &tm);
    } else {
	gs->length = strftime(gs->data, sizeof(gs->data), "%T", &tm);
    }

    if(0 == gs->length) {
	reset_globalstat(gs);
	return;
    }

    update_max(gs);
}


static void update_processes(struct globalstat *gs, 
			     const libtop_tsamp_t *tsamp) {
    int i, offset, chunksize;

    gs->length = snprintf(gs->data, sizeof(gs->data), 
			  "Processes: %" PRIu32 " total, ",
			  (uint32_t)tsamp->nprocs);

    if(gs->length <= 0) {
	reset_globalstat(gs);
	return;
    }

    offset = gs->length;

    for (i = 0; i < LIBTOP_NSTATES; ++i) {
	if(tsamp->state_breakdown[i]) {
	    chunksize = snprintf(gs->data + offset,
				 sizeof(gs->data) - offset,
				 "%d %s, ",
				 tsamp->state_breakdown[i],
				 libtop_state_str(i));
	    if(chunksize < 0)
		break;
	    
	    offset += chunksize; 	
	}
    }

    gs->length = offset;
    
    chunksize = snprintf(gs->data + gs->length, 
			 sizeof(gs->data) - gs->length, 
			 "%" PRIu32 " threads ", tsamp->threads);
    
    if(chunksize < 0) {
	reset_globalstat(gs);
	return;
    }

    gs->length += chunksize;
    assert(gs->length <= sizeof(gs->data));

    update_max(gs);
}

static void update_load(struct globalstat *gs, const libtop_tsamp_t *tsamp) {
    gs->length = snprintf(gs->data, sizeof(gs->data),
			  "Load Avg: %.2f, %.2f, %.2f ",
			  tsamp->loadavg[0], tsamp->loadavg[1],
			  tsamp->loadavg[2]);

    if(gs->length < 0) {
	reset_globalstat(gs);
	return;
    }

    update_max(gs);
}

static void cpu_percent(unsigned long long ticks, unsigned long long totalticks,
			unsigned long long *whole, unsigned long long *part) {
    *whole = 100ULL * ticks / totalticks;
    *part = (((100ULL * ticks) - (*whole * totalticks)) * 100ULL) 
	/ totalticks;
}

static void update_cpu(struct globalstat *gs, const libtop_tsamp_t *tsamp) {
    unsigned long long userticks, systicks, idleticks, totalticks;
    int mode;
    unsigned long long userwhole, userpart, syswhole, syspart, 
	idlewhole, idlepart;
  
    userticks = tsamp->cpu.cpu_ticks[CPU_STATE_USER] 
	+ tsamp->cpu.cpu_ticks[CPU_STATE_NICE];
    systicks = tsamp->cpu.cpu_ticks[CPU_STATE_SYSTEM];
    idleticks = tsamp->cpu.cpu_ticks[CPU_STATE_IDLE];

    mode = top_prefs_get_mode();

    if(STATMODE_ACCUM == mode) {
	userticks -= (tsamp->b_cpu.cpu_ticks[CPU_STATE_USER] +
		      tsamp->b_cpu.cpu_ticks[CPU_STATE_NICE]);
    
	systicks -= tsamp->b_cpu.cpu_ticks[CPU_STATE_SYSTEM];
	idleticks -= tsamp->b_cpu.cpu_ticks[CPU_STATE_IDLE];
    }

    if(STATMODE_DELTA == mode || STATMODE_NON_EVENT == mode) {
	userticks -= (tsamp->p_cpu.cpu_ticks[CPU_STATE_USER] +
		      tsamp->p_cpu.cpu_ticks[CPU_STATE_NICE]);
    
	systicks -= tsamp->p_cpu.cpu_ticks[CPU_STATE_SYSTEM];
	idleticks -= tsamp->p_cpu.cpu_ticks[CPU_STATE_IDLE];
    }

    totalticks = userticks + systicks + idleticks;

    if(0 == totalticks)
	return;


    cpu_percent(userticks, totalticks, &userwhole, &userpart);
    cpu_percent(systicks, totalticks, &syswhole, &syspart);
    cpu_percent(idleticks, totalticks, &idlewhole, &idlepart);

    gs->length = snprintf(gs->data, sizeof(gs->data),
			  "CPU usage: " 
			  "%" PRIu64 "." "%" PRIu64 "%% user, "
			  "%" PRIu64 "." "%" PRIu64 "%% sys, "
			  "%" PRIu64 "." "%" PRIu64 "%% idle ",
			  userwhole, userpart, 
			  syswhole, syspart,
			  idlewhole, idlepart);

    if(gs->length < 0) {
	reset_globalstat(gs);
	return;
    }

    update_max(gs);
}


static void update_sharedlibs(struct globalstat *gs, 
			      const libtop_tsamp_t *tsamp) {
    char code[6];
    char data[6];
    char linkedit[6];

    if((-1 == humanize_number(code, sizeof(code), tsamp->fw_code, "", 
			      HN_AUTOSCALE, GLOBALSTAT_HN_FORMAT))
       || (-1 == humanize_number(data, sizeof(data), tsamp->fw_data, "",
				 HN_AUTOSCALE, GLOBALSTAT_HN_FORMAT))
       || (-1 == humanize_number(linkedit, sizeof(linkedit), 
				 tsamp->fw_linkedit, "", HN_AUTOSCALE, 
				 GLOBALSTAT_HN_FORMAT))) {
	
	reset_globalstat(gs);
	return;
    }

    gs->length = snprintf(gs->data, sizeof(gs->data),
			  "SharedLibs: "
			  "%s resident, "
			  "%s data, "
			  "%s linkedit.",
			  code,
			  data,
			  linkedit);

    if(gs->length < 0) {
	reset_globalstat(gs);
	return;
    }

    update_max(gs);
}

static void update_memregions(struct globalstat *gs,
			      const libtop_tsamp_t *tsamp) {
    char resident[6];
    char priv[6];
    char shared[6];

    
    

    if((-1 == humanize_globalstat(resident, sizeof(resident), tsamp->rprvt))
       || (-1 == humanize_globalstat(priv, sizeof(priv), tsamp->fw_private))
       || (-1 == humanize_globalstat(shared, sizeof(shared), tsamp->rshrd))) {
	reset_globalstat(gs);
	return;
    }

    gs->length = snprintf(gs->data, sizeof(gs->data),
			  "MemRegions: %" PRIu32 " total, "
			  "%s resident, "
			  "%s private, "
			  "%s shared.",
			  tsamp->reg,
			  resident,
			  priv,
			  shared);
    if(gs->length < 0) {
	reset_globalstat(gs);
	return;
    }
    
    update_max(gs);    
}

static void update_physmem(struct globalstat *gs,
			   const libtop_tsamp_t *tsamp) {
    char wired[6];
    char active[6];
    char inactive[6];
    char used[6];
    char physfree[6];
    uint64_t total_free, total_used, total_used_count;
    struct top_uinteger wiredresult, activeresult, inactiveresult, usedresult,
	physfreeresult;
    
    total_free = (uint64_t)tsamp->vm_stat.free_count * tsamp->pagesize;
    total_used_count = (uint64_t)tsamp->vm_stat.wire_count + tsamp->vm_stat.inactive_count
			+ tsamp->vm_stat.active_count;
    total_used = total_used_count * tsamp->pagesize;

    wiredresult = top_init_uinteger(tsamp->vm_stat.wire_count
				    * tsamp->pagesize, false);
    activeresult = top_init_uinteger(tsamp->vm_stat.active_count
				     * tsamp->pagesize, false);
    inactiveresult = top_init_uinteger(tsamp->vm_stat.inactive_count
				       * tsamp->pagesize, false);
    usedresult = top_init_uinteger(total_used, false);
    physfreeresult = top_init_uinteger(total_free, false);

    if(top_humanize_uinteger(wired, sizeof(wired), wiredresult)
       || top_humanize_uinteger(active, sizeof(active), activeresult)
       || top_humanize_uinteger(inactive, sizeof(inactive), inactiveresult)
       || top_humanize_uinteger(used, sizeof(used), usedresult)
       || top_humanize_uinteger(physfree, sizeof(physfree), physfreeresult)) {
	fprintf(stderr, "top_humanize_uinteger failure in %s\n", __func__);
	reset_globalstat(gs);
	return;
    }

    gs->length = snprintf(gs->data, sizeof(gs->data),
			  "PhysMem: "
			  "%s wired, "
			  "%s active, "
			  "%s inactive, "
			  "%s used, "
			  "%s free.",
			  wired,
			  active,
			  inactive,
			  used,
			  physfree);

    if(gs->length < 0) {
	reset_globalstat(gs);
	return;
    }

    update_max(gs);
}

static void update_vm(struct globalstat *gs,
		      const libtop_tsamp_t *tsamp) {
    char vsize[6];
    char fwvsize[6];
    struct top_uinteger vsize_result, fw_vsize_result;

    vsize_result = top_init_uinteger(tsamp->vsize, false);
    fw_vsize_result = top_init_uinteger(tsamp->fw_vsize, false);

    if(top_humanize_uinteger(vsize, sizeof(vsize), vsize_result))
	return;

    if(top_humanize_uinteger(fwvsize, sizeof(fwvsize), fw_vsize_result))
	return;

    gs->length = snprintf(gs->data, sizeof(gs->data),
			  "VM: "
			  "%s vsize, "
			  "%s framework vsize, "
			  "%" PRIu64 "(%" PRIu64 ") pageins, "
			  "%" PRIu64 "(%" PRIu64 ") pageouts.",
			  vsize,
			  fwvsize,
			  (unsigned long long)tsamp->vm_stat.pageins,
			  (unsigned long long)(tsamp->vm_stat.pageins 
					       - tsamp->p_vm_stat.pageins),
			  (unsigned long long)tsamp->vm_stat.pageouts,
			  (unsigned long long)(tsamp->vm_stat.pageouts 
					       - tsamp->p_vm_stat.pageouts));

    if(gs->length < 0) {
	reset_globalstat(gs);
	return;
    }

    update_max(gs);
}

static void update_swap(struct globalstat *gs,
			const libtop_tsamp_t *tsamp) {
    struct top_uinteger used_result, available_result;
    char used[6];
    char avail[6];

    if(!tsamp->xsu_is_valid) {
	gs->length = snprintf(gs->data, sizeof(gs->data),
			      "Swap: N/A");

	if(gs->length < 0)
	    reset_globalstat(gs);

	return;
    }

    /*FIXME a p_xsu added to libtop for delta mode would be nice.*/
    used_result = top_init_uinteger(tsamp->xsu.xsu_used, false);
    available_result = top_init_uinteger(tsamp->xsu.xsu_avail, false);

    if(top_humanize_uinteger(used, sizeof(used), used_result)
       || top_humanize_uinteger(avail, sizeof(avail), available_result)) {
	reset_globalstat(gs);
	return;
    }
    
    gs->length = snprintf(gs->data, sizeof(gs->data),
			  "Swap: %s + %s free.", used, avail);

    if(gs->length < 0)
	reset_globalstat(gs);

    update_max(gs);
}

static void update_purgeable(struct globalstat *gs,
			const libtop_tsamp_t *tsamp) {
    struct top_uinteger purgeable_result, purges_result, prev_purges, 
	purges_delta;
    char purgeable[6];
    char purges[6];
    char pdelta[6];
    uint64_t purgeable_mem;

    if(!tsamp->purgeable_is_valid) {
	gs->length = snprintf(gs->data, sizeof(gs->data),
			      "Purgeable: N/A");

	if(gs->length < 0)
	    reset_globalstat(gs);

	return;
    }

    purgeable_mem = tsamp->vm_stat.purgeable_count * tsamp->pagesize;

    purgeable_result = top_init_uinteger(purgeable_mem, false);
    purges_result = top_init_uinteger(tsamp->vm_stat.purges, false);
    prev_purges = top_init_uinteger(tsamp->p_vm_stat.purges, false);
    purges_delta = top_sub_uinteger(&purges_result, &prev_purges);

    if(top_humanize_uinteger(purgeable, sizeof(purgeable), purgeable_result)
       || top_sprint_uinteger(purges, sizeof(purges), purges_result)
       || top_sprint_uinteger(pdelta, sizeof(pdelta), purges_delta)) {
	reset_globalstat(gs);
	return;
    }
    
    gs->length = snprintf(gs->data, sizeof(gs->data),
			  "Purgeable: %s %s(%s) pages purged.", 
			  purgeable, purges,
			  pdelta);

    if(gs->length < 0)
	reset_globalstat(gs);

    update_max(gs);
}

/* Return true if an error occurred. */
static bool update_networks(struct globalstat *gs,
			    const libtop_tsamp_t *tsamp) {
    struct top_uinteger inp, outp, inbytes, outbytes;
    char inpbuf[GENERIC_INT_SIZE];
    char outpbuf[GENERIC_INT_SIZE];
    char inbytesbuf[6];
    char outbytesbuf[6];
 
    inp = top_init_uinteger(tsamp->net_ipackets, false);
    outp = top_init_uinteger(tsamp->net_opackets, false);

    inbytes = top_init_uinteger(tsamp->net_ibytes, false);
    outbytes = top_init_uinteger(tsamp->net_obytes, false);

    switch(top_prefs_get_mode()) {
    case STATMODE_ACCUM: {
	/* The changes that occurred since the beginning sample. */
	struct top_uinteger binp, boutp, binbytes, boutbytes;
	
	binp = top_init_uinteger(tsamp->b_net_ipackets, false);
	boutp = top_init_uinteger(tsamp->b_net_opackets, false);
	binbytes = top_init_uinteger(tsamp->b_net_ibytes, false);
	boutbytes = top_init_uinteger(tsamp->b_net_obytes, false);

	inp = top_sub_uinteger(&inp, &binp);
	outp = top_sub_uinteger(&outp, &boutp);
	
	inbytes = top_sub_uinteger(&inbytes, &binbytes);
	outbytes = top_sub_uinteger(&outbytes, &boutbytes);
    }
	break;

    case STATMODE_DELTA: {
	/* The changes that occurred since the previous sample. */ 
	struct top_uinteger pinp, poutp, pinbytes, poutbytes;

	pinp = top_init_uinteger(tsamp->p_net_ipackets, false);
	poutp = top_init_uinteger(tsamp->p_net_opackets, false);
	pinbytes = top_init_uinteger(tsamp->p_net_ibytes, false);
	poutbytes = top_init_uinteger(tsamp->p_net_obytes, false);

	inp = top_sub_uinteger(&inp, &pinp);
	outp = top_sub_uinteger(&outp, &poutp);
	
	inbytes = top_sub_uinteger(&inbytes, &pinbytes);
	outbytes = top_sub_uinteger(&outbytes, &poutbytes);
    }
	break;
    }

    if(top_sprint_uinteger(inpbuf, sizeof(inpbuf), inp))
	return true;

    if(top_sprint_uinteger(outpbuf, sizeof(outpbuf), outp))
	return true;

    if(top_humanize_uinteger(inbytesbuf, sizeof(inbytesbuf), inbytes))
	return true;

    if(top_humanize_uinteger(outbytesbuf, sizeof(outbytesbuf), outbytes))
	return true;

    gs->length = snprintf(gs->data, sizeof(gs->data),
			  "Networks: "
			  "packets: "
			  "%s/%s in, "
			  "%s/%s out.",
			  inpbuf, inbytesbuf,
			  outpbuf, outbytesbuf);
    if(-1 == gs->length) {
	reset_globalstat(gs);
	return true;
    }

    update_max(gs);
    
    return false;
}

/* Return true if an error ocurred. */
static bool update_disks(struct globalstat *gs, const libtop_tsamp_t *tsamp) {
    char rbuf[GENERIC_INT_SIZE];
    char wbuf[GENERIC_INT_SIZE];
    char rsizebuf[6];
    char wsizebuf[6];
    struct top_uinteger rops, wops, rbytes, wbytes;

    rops = top_init_uinteger(tsamp->disk_rops, false);
    wops = top_init_uinteger(tsamp->disk_wops, false);
    rbytes = top_init_uinteger(tsamp->disk_rbytes, false);
    wbytes = top_init_uinteger(tsamp->disk_wbytes, false);

    switch(top_prefs_get_mode()) {
    case STATMODE_ACCUM: {
	struct top_uinteger brops, bwops, brbytes, bwbytes;
	
	brops = top_init_uinteger(tsamp->b_disk_rops, false);
	bwops = top_init_uinteger(tsamp->b_disk_wops, false);
	brbytes = top_init_uinteger(tsamp->b_disk_rbytes, false);
	bwbytes = top_init_uinteger(tsamp->b_disk_wbytes, false);

	rops = top_sub_uinteger(&rops, &brops);
	wops = top_sub_uinteger(&wops, &bwops);
	rbytes = top_sub_uinteger(&rbytes, &brbytes);
	wbytes = top_sub_uinteger(&wbytes, &bwbytes);
    }
	break;

    case STATMODE_DELTA: {
	struct top_uinteger props, pwops, prbytes, pwbytes;
	
	props = top_init_uinteger(tsamp->p_disk_rops, false);
	pwops = top_init_uinteger(tsamp->p_disk_wops, false);
	prbytes = top_init_uinteger(tsamp->p_disk_rbytes, false);
	pwbytes = top_init_uinteger(tsamp->p_disk_wbytes, false);

	rops = top_sub_uinteger(&rops, &props);
	wops = top_sub_uinteger(&wops, &pwops);
	rbytes = top_sub_uinteger(&rbytes, &prbytes);
	wbytes = top_sub_uinteger(&wbytes, &pwbytes);
    }
	break;
    }

    if(top_sprint_uinteger(rbuf, sizeof(rbuf), rops))
	return true;

    if(top_sprint_uinteger(wbuf, sizeof(wbuf), wops))
	return true;
    
    if(top_humanize_uinteger(rsizebuf, sizeof(rsizebuf), rbytes))
	return true;

    if(top_humanize_uinteger(wsizebuf, sizeof(wsizebuf), wbytes))
	return true;
    
    gs->length = snprintf(gs->data, sizeof(gs->data),
			  "Disks: "
			  "%s/%s read, "
			  "%s/%s written.",
			  rbuf, rsizebuf,
			  wbuf, wsizebuf);

    if(-1 == gs->length) {
	reset_globalstat(gs);
	return true;
    }
			
    update_max(gs);

    return false;
}

bool top_globalstats_update(void *ptr, const void *sample) {
    const libtop_tsamp_t *tsamp = sample;
    struct globalstat_controller *c = ptr;
    int i;

	if (!top_prefs_get_logging_mode()) {
		for(i = 0; i < GLOBALSTAT_TOTAL; ++i) {
			// MWW: If we've disabled framework logging one of these windows isn't valid and needs skipping
			if (!top_prefs_get_frameworks() && (i == GLOBALSTAT_SHAREDLIBS)) continue;
			werase(c->stats[i].window);
		}
	}

    update_processes(c->stats + GLOBALSTAT_PROCESSES, tsamp);
    update_load(c->stats + GLOBALSTAT_LOAD, tsamp);
    update_cpu(c->stats + GLOBALSTAT_CPU, tsamp);

    if(top_prefs_get_frameworks())
	update_sharedlibs(c->stats + GLOBALSTAT_SHAREDLIBS, tsamp);

    update_memregions(c->stats + GLOBALSTAT_MEMREGIONS, tsamp);
    update_physmem(c->stats + GLOBALSTAT_PHYSMEM, tsamp);
    update_vm(c->stats + GLOBALSTAT_VM, tsamp);
    update_time(c->stats + GLOBALSTAT_TIME, tsamp);

    if(top_prefs_get_swap()) {
	update_swap(c->stats + GLOBALSTAT_SWAP, tsamp);
	update_purgeable(c->stats + GLOBALSTAT_PURGEABLE, tsamp);
    }

    if(update_networks(c->stats + GLOBALSTAT_NETWORKS, tsamp))
	top_log("An error occurred while updating the network global stats.\n");

    if(update_disks(c->stats + GLOBALSTAT_DISKS, tsamp))
	top_log("An erorr occurred while updating the disk global stats.\n");

    return false;
}

static bool skip_globalstat(int i) {
    if((GLOBALSTAT_SHAREDLIBS == i && !top_prefs_get_frameworks())
       || (GLOBALSTAT_SWAP == i && !top_prefs_get_swap())
       || (GLOBALSTAT_PURGEABLE == i && !top_prefs_get_swap())) {
	return true;
    }

    return false;    
}

static void setup_globalstats_geometry(struct globalstat_controller *c,
				  int availwidth, int availheight,
				  int *outheight) {
    struct globalstat *gs;
    int i;
    int remaining = availwidth;
    int gswidth;
    int x = 0;
    int y = 0;

    *outheight = 0;

    /* First place the Processes in the upper left. */
    gs = c->stats + GLOBALSTAT_PROCESSES;

    gswidth = get_globalstat_reqwidth(gs);

    if(gswidth > availwidth)
	gswidth = availwidth;

    set_globalstat_geometry(gs, x, y, gswidth, /*height*/ 1);
    
    remaining -= gswidth;
    x += gswidth;

    /*
     * Place the time in the upper right, or if it doesn't fit,
     * down 1 column, on the left.
     */
    gs = c->stats + GLOBALSTAT_TIME;

    gswidth = get_globalstat_reqwidth(gs);

    if(gswidth > availwidth)
	gswidth = availwidth;

    if(remaining >= gswidth) {
	/* There is enough room on the first row for the time. */
	set_globalstat_geometry(gs, availwidth - gswidth, y, 
				gswidth, /*height*/ 1);

	/* Go down a row. */
	++y;
	/* Reset the x to the left most. */
	x = 0;
	remaining = availwidth; 
    }  else {
	/* Move the time down a row. */
	++y;
	x = 0;
	remaining = availwidth;

	set_globalstat_geometry(gs, x, y, gswidth, 
				/*height*/ 1);
	x += gswidth;
       	remaining -= gswidth;
    }

    for(i = GLOBALSTAT_LOAD; i < GLOBALSTAT_TOTAL; ++i) {
	if(skip_globalstat(i)) {
	    /* Skip the updates for these. */
	    continue;
	}

	gs = c->stats + i;
	
	gswidth = get_globalstat_reqwidth(gs);

	if(gswidth > availwidth)
	    gswidth = availwidth;
  	
	/* Is the space remaining less than the total globalstat width? */
	if(remaining < gswidth) {
	    /* Reset the remaining width. */
	    ++y;
	    x = 0;
	    remaining = availwidth;
	}

	set_globalstat_geometry(gs, x, y, gswidth, /*height*/ 1);
	remaining -= gswidth;
	x += gswidth;
    }

    if(remaining != availwidth)
	++y;

    *outheight = y;
}

bool top_globalstats_resize(void *ptr, int width, int height,
			    int *consumed_height) {
    struct globalstat_controller *c = ptr;
    int i;
    
    top_log("%s\n", __func__);

    *consumed_height = 0;
    
    setup_globalstats_geometry(c, width, height, consumed_height);
   
    if(ERR == wresize(c->window, *consumed_height, width)) {
	top_log("wresize error: height %d width %d\n",
		*consumed_height, width);
	return true;
    }

    if(ERR == move_panel(c->panel, 0, 0)) {
	top_log("move_panel error: 0 0\n");
	return true;
    }    

    for(i = 0; i < GLOBALSTAT_TOTAL; ++i) {
	struct globalstat *gs = c->stats + i;
	
	// MWW: Skip the frameworks window if we've got them disabled
	if (!top_prefs_get_frameworks() && (i == GLOBALSTAT_SHAREDLIBS)) continue;

	if(skip_globalstat(i)) {
	    /* Skip and hide the panels for these. */
	    hide_panel(gs->panel);
	    continue;
	}

	gs = c->stats + i;

	/* 
	 * Avoid errors below by resizing and moving the panel to a known
	 * good size/offfset.
	 */
	wresize(gs->window, 1, 1);
	move_panel(gs->panel, 0, 0);

	if(ERR == move_panel(gs->panel, gs->y, gs->x))
	    return true;
	
	if(ERR == wresize(gs->window, gs->size.height, gs->size.width))
	    return true;

	if(ERR == werase(gs->window))
	    return true;
    }

    return false;
}


void top_globalstats_iterate(void *ptr, bool (*iter)(char *, void *),
			     void *dataptr) {
    struct globalstat_controller *c = ptr;
    int i;

    for(i = 0; i < GLOBALSTAT_TOTAL; ++i) {
	if(skip_globalstat(i))
	    continue;
	
	if(c->stats[i].length > 0) {
	    if(!iter(c->stats[i].data, dataptr))
		break;
	}	    
    }
}

void top_globalstats_reset(void *ptr) {
    struct globalstat_controller *c = ptr;
    int i;
    
    for(i = 0; i < GLOBALSTAT_TOTAL; ++i) {
	c->stats[i].max_width = c->stats[i].length;
    }
}
