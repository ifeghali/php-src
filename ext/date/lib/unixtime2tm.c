/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2004 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Derick Rethans <dr@ez.no>                                   |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(_MSC_VER)
#define PHP_LL_CONST(n) n ## i64
#else
#define PHP_LL_CONST(n) n ## ll
#endif

#include "datetime.h"

static int month_tab_leap[12] = { -1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
static int month_tab[12] =      { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };


/* Converts a Unix timestamp value into broken down time, in GMT */
void timelib_unixtime2gmt(timelib_time* tm, timelib_sll ts)
{
	timelib_sll days, remainder, tmp_days;
	timelib_sll cur_year = 1970;
	timelib_sll i;
	timelib_sll hours, minutes, seconds;
	int *months;

	days = ts / SECS_PER_DAY;
	remainder = ts - (days * SECS_PER_DAY);
	if (ts < 0 && remainder == 0) {
		days++;
		remainder -= SECS_PER_DAY;
	}
	DEBUG(printf("days=%lld, rem=%lld\n", days, remainder););

	if (ts >= 0) {
		tmp_days = days + 1;
		while (tmp_days >= DAYS_PER_LYEAR) {
			cur_year++;
			if (is_leap(cur_year)) {
				tmp_days -= DAYS_PER_LYEAR;
			} else {
				tmp_days -= DAYS_PER_YEAR;
			}
		}
	} else {
		tmp_days = days;

		/* Guess why this might be for, it has to do with a pope ;-). It's also
		 * only valid for Great Brittain and it's colonies. It needs fixing for
		 * other locales. *sigh*, why is this crap so complex! */
		if (ts <= PHP_LL_CONST(-6857352000)) {
			tmp_days -= 11;
		}

		while (tmp_days <= 0) {
			cur_year--;
			DEBUG(printf("tmp_days=%lld, year=%lld\n", tmp_days, cur_year););
			if (is_leap(cur_year)) {
				tmp_days += DAYS_PER_LYEAR;
			} else {
				tmp_days += DAYS_PER_YEAR;
			}
		}
		remainder += SECS_PER_DAY;
	}
	DEBUG(printf("tmp_days=%lld, year=%lld\n", tmp_days, cur_year););

	months = is_leap(cur_year) ? month_tab_leap : month_tab;
	i = 11;
	while (i > 0) {
		DEBUG(printf("month=%lld (%d)\n", i, months[i]););
		if (tmp_days > months[i]) {
			break;
		}
		i--;
	}
	DEBUG(printf("A: ts=%lld, year=%lld, month=%lld, day=%lld,", ts, cur_year, i + 1, tmp_days - months[i]););

	/* That was the date, now we do the tiiiime */
	hours = remainder / 3600;
	minutes = (remainder - hours * 3600) / 60;
	seconds = remainder % 60;
	DEBUG(printf(" hour=%lld, minute=%lld, second=%lld\n", hours, minutes, seconds););

	tm->y = cur_year;
	tm->m = i + 1;
	tm->d = tmp_days - months[i];
	tm->h = hours;
	tm->i = minutes;
	tm->s = seconds;
	tm->z = 0;
	tm->dst = 0;
	tm->sse = ts;
	tm->sse_uptodate = 1;
	tm->tim_uptodate = 1;
	tm->is_localtime = 0;
	tm->have_zone = 0;
}

void timelib_unixtime2local(timelib_time *tm, timelib_sll ts, timelib_tzinfo* tz)
{
	timelib_time_offset *gmt_offset;

	gmt_offset = timelib_get_time_zone_info(ts, tz);
	timelib_unixtime2gmt(tm, ts + gmt_offset->offset);
	/* we need to reset the sse here as unixtime2gmt modifies it */
	tm->sse = ts; 
	tm->dst = gmt_offset->is_dst;
	tm->z = gmt_offset->offset;
	tm->tz_info = tz;
/*	if (tm->tz_abbr) {
		free(tm->tz_abbr);
	}
	tm->tz_abbr = strdup(gmt_offset->abbr);*/
	timelib_time_tz_abbr_update(tm, gmt_offset->abbr);
	timelib_time_offset_dtor(gmt_offset);

	tm->is_localtime = 1;
	tm->have_zone = 1;
	tm->zone_type = TIMELIB_ZONETYPE_ID;
}

void timelib_set_timezone(timelib_time *t, timelib_tzinfo *tz)
{
	timelib_time_offset *gmt_offset;

	gmt_offset = timelib_get_time_zone_info(t->sse, tz);
	t->z = gmt_offset->offset;
/*
	if (t->dst != gmt_offset->is_dst) {
		printf("ERROR (%d, %d)\n", t->dst, gmt_offset->is_dst);
		exit(1);
	}
*/
	t->dst = gmt_offset->is_dst;
	t->tz_info = tz;
	if (t->tz_abbr) {
		free(t->tz_abbr);
	}
	t->tz_abbr = strdup(gmt_offset->abbr);
	timelib_time_offset_dtor(gmt_offset);

	t->have_zone = 1;
	t->zone_type = TIMELIB_ZONETYPE_ID;
}

/* Converts the time stored in the struct to localtime if localtime = true,
 * otherwise it converts it to gmttime. This is only done when necessary
 * ofcourse. */
int timelib_apply_localtime(timelib_time *t, unsigned int localtime)
{
	if (localtime) {
		/* Converting from GMT time to local time */
		DEBUG(printf("Converting from GMT time to local time\n"););

		/* Check if TZ is set */
		if (!t->tz_info) {
			DEBUG(printf("E: No timezone configured, can't switch to local time\n"););
			return -1;
		}

		timelib_unixtime2local(t, t->sse, t->tz_info);
	} else {
		/* Converting from local time to GMT time */
		DEBUG(printf("Converting from local time to GMT time\n"););

		timelib_unixtime2gmt(t, t->sse);
	}
	return 0;
}
