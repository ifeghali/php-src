/*
   +----------------------------------------------------------------------+
   | PHP Version 4                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 2001 The PHP Group                                     |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Rui Hirokawa <hirokawa@php.net>                              |
   +----------------------------------------------------------------------+
 */

/*
 * "streamable korean code filter and converter"
 */

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_globals.h"

#if defined(HAVE_MBSTR_KR)
#include "mbfilter.h"
#include "mbfilter_cn.h"

#include "unicode_table_kr.h"

#define CK(statement)	do { if ((statement) < 0) return (-1); } while (0)


/*
 * EUC-KR => wchar
 */
int
mbfl_filt_conv_euckr_wchar(int c, mbfl_convert_filter *filter TSRMLS_DC)
{
	int c1, w, flag;

	switch (filter->status) {
	case 0:
		if (c >= 0 && c < 0x80) {	/* latin */
			CK((*filter->output_function)(c, filter->data TSRMLS_CC));
		} else if (c > 0xa0 && c < 0xff && c != 0xc9) {	/* dbcs lead byte */
			filter->status = 1;
			filter->cache = c;
		} else {
			w = c & MBFL_WCSGROUP_MASK;
			w |= MBFL_WCSGROUP_THROUGH;
			CK((*filter->output_function)(w, filter->data TSRMLS_CC));
		}
		break;

	case 1:		/* dbcs second byte */
		filter->status = 0;
		c1 = filter->cache;
		flag = 0;
		if (c1 >= 0xa1 && c1 <= 0xc6) {
			flag = 1;
		} else if (c1 >= 0xc7 && c1 <= 0xfe && c1 != 0xc9) {
			flag = 2;
		}
		if (flag > 0 && c >= 0xa1 && c <= 0xfe) {
			if (flag == 1){
				w = (c1 - 0xa1)*178 + (c - 0xa1) + 0x54;
				if (w >= 0 && w < uhc2_ucs_table_size) {
					w = uhc2_ucs_table[w];
				} else {
					w = 0;
				}
			} else {
				if (c1 < 0xc9){
					w = (c1 - 0xc7)*94 + c - 0xa1;
				} else {
					w = (c1 - 0xc8)*94 + c - 0xa1;
				}
				if (w >= 0 && w < uhc3_ucs_table_size) {
					w = uhc3_ucs_table[w];
				} else {
					w = 0;
				}
			}
			
			if (w <= 0) {
				w = (c1 << 8) | c;
				w &= MBFL_WCSPLANE_MASK;
				w |= MBFL_WCSPLANE_KSC5601;
			}
			CK((*filter->output_function)(w, filter->data TSRMLS_CC));
		} else if ((c >= 0 && c < 0x21) || c == 0x7f) {		/* CTLs */
			CK((*filter->output_function)(c, filter->data TSRMLS_CC));
		} else {
			w = (c1 << 8) | c;
			w &= MBFL_WCSGROUP_MASK;
			w |= MBFL_WCSGROUP_THROUGH;
			CK((*filter->output_function)(w, filter->data TSRMLS_CC));
		}
		break;

	default:
		filter->status = 0;
		break;
	}

	return c;
}

/*
 * wchar => EUC-KR
 */
int
mbfl_filt_conv_wchar_euckr(int c, mbfl_convert_filter *filter TSRMLS_DC)
{
	int c1, c2, s;

	s = 0;

	if (c >= ucs_a1_uhc_table_min && c < ucs_a1_uhc_table_max) {
		s = ucs_a1_uhc_table[c - ucs_a1_uhc_table_min];
	} else if (c >= ucs_a2_uhc_table_min && c < ucs_a2_uhc_table_max) {
		s = ucs_a2_uhc_table[c - ucs_a2_uhc_table_min];
	} else if (c >= ucs_a3_uhc_table_min && c < ucs_a3_uhc_table_max) {
		s = ucs_a3_uhc_table[c - ucs_a3_uhc_table_min];
	} else if (c >= ucs_i_uhc_table_min && c < ucs_i_uhc_table_max) {
		s = ucs_i_uhc_table[c - ucs_i_uhc_table_min];
	} else if (c >= ucs_r1_uhc_table_min && c < ucs_r1_uhc_table_max) {
		s = ucs_r1_uhc_table[c - ucs_r1_uhc_table_min];
	} else if (c >= ucs_r2_uhc_table_min && c < ucs_r2_uhc_table_max) {
		s = ucs_r2_uhc_table[c - ucs_r2_uhc_table_min];
	}

	c1 = (s >> 8) & 0xff;
	c2 = s & 0xff;
	/* exclude UHC extension area */
	if (c1 < 0xa1 || c1 > 0xfe || c2 < 0xa1 && c2 > 0xfe){ 
		s = 0;
	}

	if (s <= 0) {
		c1 = c & ~MBFL_WCSPLANE_MASK;
		if (c1 == MBFL_WCSPLANE_KSC5601) {
			s = c & MBFL_WCSPLANE_MASK;
		}
		if (c == 0) {
			s = 0;
		} else if (s <= 0) {
			s = -1;
		}
	}
	if (s >= 0) {
		if (s < 0x80) {	/* latin */
			CK((*filter->output_function)(s, filter->data TSRMLS_CC));
		} else {
			CK((*filter->output_function)((s >> 8) & 0xff, filter->data TSRMLS_CC));
			CK((*filter->output_function)(s & 0xff, filter->data TSRMLS_CC));
		}
	} else {
		if (filter->illegal_mode != MBFL_OUTPUTFILTER_ILLEGAL_MODE_NONE) {
			CK(mbfl_filt_conv_illegal_output(c, filter TSRMLS_CC));
		}
	}

	return c;
}

/*
 * UHC => wchar
 */
int
mbfl_filt_conv_uhc_wchar(int c, mbfl_convert_filter *filter TSRMLS_DC)
{
	int c1, w, flag;
	const short ofst1[] = { 0x41, 0x61, 0x81, 0xa1}; 
	const short ofst2[] = { 0x0,  0x1a, 0x34, 0x54}; 

	switch (filter->status) {
	case 0:
		if (c >= 0 && c < 0x80) {	/* latin */
			CK((*filter->output_function)(c, filter->data TSRMLS_CC));
		} else if (c > 0x80 && c < 0xff && c != 0xc9) {	/* dbcs lead byte */
			filter->status = 1;
			filter->cache = c;
		} else {
			w = c & MBFL_WCSGROUP_MASK;
			w |= MBFL_WCSGROUP_THROUGH;
			CK((*filter->output_function)(w, filter->data TSRMLS_CC));
		}
		break;

	case 1:		/* dbcs second byte */
		filter->status = 0;
		c1 = filter->cache;

		flag = 0;
		if ( c >= 0x41 && c <= 0x5a){
			flag = 1;
		} else if (c >= 0x61 && c <= 0x7a){
			flag = 2;
		} else if (c >= 0x81 && c <= 0xa0){
			flag = 3;
		} else if (c >= 0xa1 && c <= 0xfe){
			flag = 4;
		}
		if ( c1 >= 0x81 && c1 <= 0xa0 && flag > 0){
			w = (c1 - 0x81)*178 + (c - ofst1[flag-1] + ofst2[flag-1]);
			if (w >= 0 && w < uhc1_ucs_table_size) {
				w = uhc1_ucs_table[w];
			} else {
				w = 0;
			}			
		} else if ( c1 >= 0xa1 && c1 <= 0xc6 && flag > 0){
			w = (c1 - 0xa1)*178 + (c - ofst1[flag-1] + ofst2[flag-1]);			
			if (w >= 0 && w < uhc2_ucs_table_size) {
				w = uhc2_ucs_table[w];
			} else {
				w = 0;
			}			
		} else if ( c1 >= 0xc7 && c1 <= 0xfe && flag == 4){
			if (c1 < 0xc9){
				w = (c1 - 0xc7)*94 + (c - ofst1[flag-1]);		
			} else {
				w = (c1 - 0xc8)*94 + (c - ofst1[flag-1]);		
			}
			if (w >= 0 && w < uhc3_ucs_table_size) {
				w = uhc3_ucs_table[w];
			} else {
				w = 0;
			}			
		}
		if (flag > 0){
			if (w <= 0) {
				w = (c1 << 8) | c;
				w &= MBFL_WCSPLANE_MASK;
				w |= MBFL_WCSPLANE_UHC;
			}
			CK((*filter->output_function)(w, filter->data TSRMLS_CC));
		} else {
			if ((c >= 0 && c < 0x21) || c == 0x7f) {		/* CTLs */
				CK((*filter->output_function)(c, filter->data TSRMLS_CC));
			} else {
				w = (c1 << 8) | c;
				w &= MBFL_WCSGROUP_MASK;
				w |= MBFL_WCSGROUP_THROUGH;
				CK((*filter->output_function)(w, filter->data TSRMLS_CC));
			}
		}
		break;

	default:
		filter->status = 0;
		break;
	}

	return c;
}

/*
 * wchar => UHC
 */
int
mbfl_filt_conv_wchar_uhc(int c, mbfl_convert_filter *filter TSRMLS_DC)
{
	int c1, s;

	s = 0;
	if (c >= ucs_a1_uhc_table_min && c < ucs_a1_uhc_table_max) {
		s = ucs_a1_uhc_table[c - ucs_a1_uhc_table_min];
	} else if (c >= ucs_a2_uhc_table_min && c < ucs_a2_uhc_table_max) {
		s = ucs_a2_uhc_table[c - ucs_a2_uhc_table_min];
	} else if (c >= ucs_a3_uhc_table_min && c < ucs_a3_uhc_table_max) {
		s = ucs_a3_uhc_table[c - ucs_a3_uhc_table_min];
	} else if (c >= ucs_i_uhc_table_min && c < ucs_i_uhc_table_max) {
		s = ucs_i_uhc_table[c - ucs_i_uhc_table_min];
	} else if (c >= ucs_s_uhc_table_min && c < ucs_s_uhc_table_max) {
		s = ucs_s_uhc_table[c - ucs_s_uhc_table_min];
	} else if (c >= ucs_r1_uhc_table_min && c < ucs_r1_uhc_table_max) {
		s = ucs_r1_uhc_table[c - ucs_r1_uhc_table_min];
	} else if (c >= ucs_r2_uhc_table_min && c < ucs_r2_uhc_table_max) {
		s = ucs_r2_uhc_table[c - ucs_r2_uhc_table_min];
	}
	if (s <= 0) {
		c1 = c & ~MBFL_WCSPLANE_MASK;
		if (c1 == MBFL_WCSPLANE_UHC) {
			s = c & MBFL_WCSPLANE_MASK;
		}
		if (c == 0) {
			s = 0;
		} else if (s <= 0) {
			s = -1;
		}
	}
	if (s >= 0) {
		if (s < 0x80) {	/* latin */
			CK((*filter->output_function)(s, filter->data TSRMLS_CC));
		} else {
			CK((*filter->output_function)((s >> 8) & 0xff, filter->data TSRMLS_CC));
			CK((*filter->output_function)(s & 0xff, filter->data TSRMLS_CC));
		}
	} else {
		if (filter->illegal_mode != MBFL_OUTPUTFILTER_ILLEGAL_MODE_NONE) {
			CK(mbfl_filt_conv_illegal_output(c, filter TSRMLS_CC));
		}
	}

	return c;
}

#endif /* HAVE_MBSTR_KR */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
