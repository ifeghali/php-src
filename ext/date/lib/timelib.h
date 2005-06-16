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

#ifndef __TIMELIB_H__
#define __TIMELIB_H__

#include "timelib_structs.h"

#define TIMELIB_NONE             0x00
#define TIMELIB_OVERRIDE_TIME    0x01

#ifndef LONG_MAX
#define LONG_MAX 2147483647L
#endif

#ifndef LONG_MIN
#define LONG_MIN (- LONG_MAX - 1)
#endif

/* From dow.c */
timelib_sll timelib_day_of_week(timelib_sll y, timelib_sll m, timelib_sll d);
timelib_sll timelib_daynr_from_weeknr(timelib_sll y, timelib_sll w, timelib_sll d);

/* From parse_date.re */
timelib_time *timelib_strtotime(char *s);
void timelib_fill_holes(timelib_time *parsed, timelib_time *now, int options);

/* From tm2unixtime.c */
void timelib_update_ts(timelib_time* time, timelib_tzinfo* tzi);

/* From unixtime2tm.c */
int timelib_apply_localtime(timelib_time *t, unsigned int localtime);
void timelib_unixtime2gmt(timelib_time* tm, timelib_sll ts);
void timelib_unixtime2local(timelib_time *tm, timelib_sll ts, timelib_tzinfo* tz);
void timelib_set_timezone(timelib_time *t, timelib_tzinfo *tz);

/* From parse_tz.c */
timelib_tzinfo *timelib_parse_tzfile(char *timezone);
int timelib_timestamp_is_in_dst(timelib_sll ts, timelib_tzinfo *tz);
timelib_time_offset *timelib_get_time_zone_info(timelib_sll ts, timelib_tzinfo *tz);
void timelib_dump_tzinfo(timelib_tzinfo *tz);

/* From timelib.c */
timelib_tzinfo* timelib_tzinfo_ctor();
void timelib_time_tz_abbr_update(timelib_time* tm, char* tz_abbr);
void timelib_time_tz_name_update(timelib_time* tm, char* tz_name);
void timelib_tzinfo_dtor(timelib_tzinfo *tz);
timelib_tzinfo* timelib_tzinfo_clone(timelib_tzinfo *tz);

timelib_time* timelib_time_ctor();
void timelib_time_dtor(timelib_time* t);

timelib_time_offset* timelib_time_offset_ctor();
void timelib_time_offset_dtor(timelib_time_offset* t);

signed long timelib_date_to_int(timelib_time *d, int *error);
void timelib_dump_date(timelib_time *d, int options);

#endif
