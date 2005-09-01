/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2005 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Derick Rethans <derick@derickrethans.nl>                    |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#include "timelib.h"

#include <stdio.h>

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "timezonedb.h"

#ifdef WORDS_BIGENDIAN
#define timelib_conv_int(l) (l)
#else
#define timelib_conv_int(l) ((l & 0x000000ff) << 24) + ((l & 0x0000ff00) << 8) + ((l & 0x00ff0000) >> 8) + ((l & 0xff000000) >> 24)
#endif

static void read_header(char **tzf, timelib_tzinfo *tz)
{
	uint32_t buffer[6];

	memcpy(&buffer, *tzf, sizeof(buffer));
	tz->ttisgmtcnt = timelib_conv_int(buffer[0]);
	tz->ttisstdcnt = timelib_conv_int(buffer[1]);
	tz->leapcnt    = timelib_conv_int(buffer[2]);
	tz->timecnt    = timelib_conv_int(buffer[3]);
	tz->typecnt    = timelib_conv_int(buffer[4]);
	tz->charcnt    = timelib_conv_int(buffer[5]);
	*tzf += sizeof(buffer);
}

static void read_transistions(char **tzf, timelib_tzinfo *tz)
{
	int32_t *buffer = NULL;
	uint32_t i;
	unsigned char *cbuffer = NULL;

	if (tz->timecnt) {
		buffer = (int32_t*) malloc(tz->timecnt * sizeof(int32_t));
		if (!buffer) {
			return;
		}
		memcpy(buffer, *tzf, sizeof(int32_t) * tz->timecnt);
		*tzf += (sizeof(int32_t) * tz->timecnt);
		for (i = 0; i < tz->timecnt; i++) {
			buffer[i] = timelib_conv_int(buffer[i]);
		}

		cbuffer = (unsigned char*) malloc(tz->timecnt * sizeof(unsigned char));
		if (!cbuffer) {
			return;
		}
		memcpy(cbuffer, *tzf, sizeof(unsigned char) * tz->timecnt);
		*tzf += sizeof(unsigned char) * tz->timecnt;
	}
	
	tz->trans = buffer;
	tz->trans_idx = cbuffer;
}

static void read_types(char **tzf, timelib_tzinfo *tz)
{
	unsigned char *buffer;
	int32_t *leap_buffer;
	unsigned int i, j;

	buffer = (unsigned char*) malloc(tz->typecnt * sizeof(unsigned char) * 6);
	if (!buffer) {
		return;
	}
	memcpy(buffer, *tzf, sizeof(unsigned char) * 6 * tz->typecnt);
	*tzf += sizeof(unsigned char) * 6 * tz->typecnt;

	tz->type = (ttinfo*) malloc(tz->typecnt * sizeof(struct ttinfo));
	if (!tz->type) {
		return;
	}

	for (i = 0; i < tz->typecnt; i++) {
		j = i * 6;
		tz->type[i].offset = (buffer[j] * 16777216) + (buffer[j + 1] * 65536) + (buffer[j + 2] * 256) + buffer[j + 3];
		tz->type[i].isdst = buffer[j + 4];
		tz->type[i].abbr_idx = buffer[j + 5];
	}
	free(buffer);

	tz->timezone_abbr = (char*) malloc(tz->charcnt);
	if (!tz->timezone_abbr) {
		return;
	}
	memcpy(tz->timezone_abbr, *tzf, sizeof(char) * tz->charcnt);
	*tzf += sizeof(char) * tz->charcnt;

	leap_buffer = (int32_t *) malloc(tz->leapcnt * 2 * sizeof(int32_t));
	if (!leap_buffer) {
		return;
	}
	memcpy(leap_buffer, *tzf, sizeof(int32_t) * tz->leapcnt * 2);
	*tzf += sizeof(int32_t) * tz->leapcnt * 2;

	tz->leap_times = (tlinfo*) malloc(tz->leapcnt * sizeof(tlinfo));
	if (!tz->leap_times) {
		return;
	}
	for (i = 0; i < tz->leapcnt; i++) {
		tz->leap_times[i].trans = timelib_conv_int(leap_buffer[i * 2]);
		tz->leap_times[i].offset = timelib_conv_int(leap_buffer[i * 2 + 1]);
	}
	free(leap_buffer);
	
	buffer = (unsigned char*) malloc(tz->ttisstdcnt * sizeof(unsigned char));
	if (!buffer) {
		return;
	}
	memcpy(buffer, *tzf, sizeof(unsigned char) * tz->ttisstdcnt);
	*tzf += sizeof(unsigned char) * tz->ttisstdcnt;

	for (i = 0; i < tz->ttisstdcnt; i++) {
		tz->type[i].isstdcnt = buffer[i];
	}
	free(buffer);

	buffer = (unsigned char*) malloc(tz->ttisgmtcnt * sizeof(unsigned char));
	if (!buffer) {
		return;
	}
	memcpy(buffer, *tzf, sizeof(unsigned char) * tz->ttisgmtcnt);
	*tzf += sizeof(unsigned char) * tz->ttisgmtcnt;

	for (i = 0; i < tz->ttisgmtcnt; i++) {
		tz->type[i].isgmtcnt = buffer[i];
	}
	free(buffer);
}

void timelib_dump_tzinfo(timelib_tzinfo *tz)
{
	uint32_t i;

	printf("UTC/Local count:   %lu\n", (unsigned long) tz->ttisgmtcnt);
	printf("Std/Wall count:    %lu\n", (unsigned long) tz->ttisstdcnt);
	printf("Leap.sec. count:   %lu\n", (unsigned long) tz->leapcnt);
	printf("Trans. count:      %lu\n", (unsigned long) tz->timecnt);
	printf("Local types count: %lu\n", (unsigned long) tz->typecnt);
	printf("Zone Abbr. count:  %lu\n", (unsigned long) tz->charcnt);

	printf ("%8s (%12s) = %3d [%5ld %1d %3d '%s' (%d,%d)]\n",
		"", "", 0,
		(long int) tz->type[0].offset,
		tz->type[0].isdst,
		tz->type[0].abbr_idx,
		&tz->timezone_abbr[tz->type[0].abbr_idx],
		tz->type[0].isstdcnt,
		tz->type[0].isgmtcnt
		);
	for (i = 0; i < tz->timecnt; i++) {
		printf ("%08X (%12d) = %3d [%5ld %1d %3d '%s' (%d,%d)]\n",
			tz->trans[i], tz->trans[i], tz->trans_idx[i],
			(long int) tz->type[tz->trans_idx[i]].offset,
			tz->type[tz->trans_idx[i]].isdst,
			tz->type[tz->trans_idx[i]].abbr_idx,
			&tz->timezone_abbr[tz->type[tz->trans_idx[i]].abbr_idx],
			tz->type[tz->trans_idx[i]].isstdcnt,
			tz->type[tz->trans_idx[i]].isgmtcnt
			);
	}
	for (i = 0; i < tz->leapcnt; i++) {
		printf ("%08X (%12ld) = %d\n",
			tz->leap_times[i].trans,
			(long) tz->leap_times[i].trans,
			tz->leap_times[i].offset);
	}
}

static int tz_search(char *timezone, int left, int right)
{
	int mid, cmp;

	if (left > right) {
		return -1; /* not found */
	}
 
	mid = (left + right) / 2;
 
	cmp = strcasecmp(timezone, timezonedb_idx[mid].id);
	if (cmp < 0) {
		return tz_search(timezone, left, mid - 1);
	} else if (cmp > 0) {
		return tz_search(timezone, mid + 1, right);
	} else { /* (cmp == 0) */
		return timezonedb_idx[mid].pos;
	}
}


static int seek_to_tz_position(char **tzf, char *timezone)
{
	int	pos = tz_search(timezone, 0, sizeof(timezonedb_idx)/sizeof(*timezonedb_idx)-1);

	if (pos == -1) {
		return 0;
	}

	(*tzf) = &timelib_timezone_db_data[pos + 20];
	return 1;
}

timelib_tzdb_index_entry *timelib_timezone_identifiers_list(int *count)
{
	*count = sizeof(timezonedb_idx) / sizeof(*timezonedb_idx);
	return timezonedb_idx;
}

timelib_tzinfo *timelib_parse_tzfile(char *timezone)
{
	char *tzf;
	timelib_tzinfo *tmp;

	if (seek_to_tz_position((char**) &tzf, timezone)) {
		tmp = timelib_tzinfo_ctor(timezone);

		read_header((char**) &tzf, tmp);
		read_transistions((char**) &tzf, tmp);
		read_types((char**) &tzf, tmp);
	} else {
		tmp = NULL;
	}

	return tmp;
}

static ttinfo* fetch_timezone_offset(timelib_tzinfo *tz, timelib_sll ts, timelib_sll *transition_time)
{
	uint32_t i;

	/* If there is no transistion time, we pick the first one, if that doesn't
	 * exist we return NULL */
	if (!tz->timecnt || !tz->trans) {
		*transition_time = 0;
		if (tz->typecnt == 1) {
			return &(tz->type[0]);
		}
		return NULL;
	}

	/* If the TS is lower than the first transistion time, then we scan over
	 * all the transistion times to find the first non-DST one, or the first
	 * one in case there are only DST entries. Not sure which smartass came up
	 * with this idea in the first though :) */
	if (ts < tz->trans[0]) {
		uint32_t j;

		*transition_time = 0;
		j = 0;
		while (j < tz->timecnt && tz->type[j].isdst) {
			++j;
		}
		if (j == tz->timecnt) {
			j = 0;
		}
		return &(tz->type[j]);
	}

	/* In all other cases we loop through the available transtion times to find
	 * the correct entry */
	for (i = 0; i < tz->timecnt; i++) {
		if (ts < tz->trans[i]) {
			*transition_time = tz->trans[i - 1];
			return &(tz->type[tz->trans_idx[i - 1]]);
		}
	}
	*transition_time = tz->trans[tz->timecnt - 1];
	return &(tz->type[tz->trans_idx[tz->timecnt - 1]]);
}

static tlinfo* fetch_leaptime_offset(timelib_tzinfo *tz, timelib_sll ts)
{
	int i;

	if (!tz->leapcnt || !tz->leap_times) {
		return NULL;
	}

	for (i = tz->leapcnt - 1; i > 0; i--) {
		if (ts > tz->leap_times[i].trans) {
			return &(tz->leap_times[i]);
		}
	}
	return NULL;
}

int timelib_timestamp_is_in_dst(timelib_sll ts, timelib_tzinfo *tz)
{
	ttinfo *to;
	timelib_sll dummy;
	
	if ((to = fetch_timezone_offset(tz, ts, &dummy))) {
		return to->isdst;
	}
	return -1;
}

timelib_time_offset *timelib_get_time_zone_info(timelib_sll ts, timelib_tzinfo *tz)
{
	ttinfo *to;
	tlinfo *tl;
	int32_t offset = 0, leap_secs = 0;
	char *abbr;
	timelib_time_offset *tmp = timelib_time_offset_ctor();
	timelib_sll                transistion_time;

	if ((to = fetch_timezone_offset(tz, ts, &transistion_time))) {
		offset = to->offset;
		abbr = &(tz->timezone_abbr[to->abbr_idx]);
		tmp->is_dst = to->isdst;
		tmp->transistion_time = transistion_time;
	} else {
		offset = 0;
		abbr = tz->timezone_abbr;
		tmp->is_dst = 0;
		tmp->transistion_time = 0;
	}

	if ((tl = fetch_leaptime_offset(tz, ts))) {
		leap_secs = -tl->offset;
	}

	tmp->offset = offset;
	tmp->leap_secs = leap_secs;
	tmp->abbr = abbr ? strdup(abbr) : strdup("GMT");

	return tmp;
}
