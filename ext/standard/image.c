/*
   +----------------------------------------------------------------------+
   | PHP Version 4                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2002 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Rasmus Lerdorf <rasmus@php.net>                             |
   |          Marcus Boerger <helly@php.net>                              |
   +----------------------------------------------------------------------+
 */
/* $Id$ */
/*
 * Based on Daniel Schmitt's imageinfo.c which carried the following
 * Copyright notice.
 */

/*
 * imageinfo.c
 *
 * Simple routines to extract image width/height data from GIF/JPEG files.
 *
 * Copyright (c) 1997 Daniel Schmitt, opal online publishing, Bonn, Germany.
 *
 * Includes code snippets from rdjpgcom.c,
 * Copyright (c) 1994-1995 Thomas G. Lane
 * from release 6a of the Independent JPEG Group's software.
 *
 * Legal status: see GNU General Public License version 2 or later.
 *
 */

#include "php.h"
#include <stdio.h>
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "fopen_wrappers.h"
#include "ext/standard/fsock.h"
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "php_image.h"

/* file type markers */
PHPAPI const char php_sig_gif[3] = {'G', 'I', 'F'};
PHPAPI const char php_sig_psd[4] = {'8', 'B', 'P', 'S'};
PHPAPI const char php_sig_bmp[2] = {'B', 'M'};
PHPAPI const char php_sig_swf[3] = {'F', 'W', 'S'};
PHPAPI const char php_sig_jpg[3] = {(char) 0xff, (char) 0xd8, (char) 0xff};
PHPAPI const char php_sig_png[8] = {(char) 0x89, (char) 0x50, (char) 0x4e, (char) 0x47,
(char) 0x0d, (char) 0x0a, (char) 0x1a, (char) 0x0a};
PHPAPI const char php_sig_tif_ii[4] = {'I','I', (char)0x2A, (char)0x00};
PHPAPI const char php_sig_tif_mm[4] = {'M','M', (char)0x00, (char)0x2A};
PHPAPI const char php_sig_jpc[3] = {(char)0xFF, (char)0x4F, (char)0xff};


/* return info as a struct, to make expansion easier */

struct gfxinfo {
	unsigned int width;
	unsigned int height;
	unsigned int bits;
	unsigned int channels;
};

/* {{{ php_handle_gif
 * routine to handle GIF files. If only everything were that easy... ;} */
static struct gfxinfo *php_handle_gif (php_stream * stream)
{
	struct gfxinfo *result = NULL;
	unsigned char a[2];

	result = (struct gfxinfo *) ecalloc(1, sizeof(struct gfxinfo));

	php_stream_seek(stream, 3, SEEK_CUR);

	php_stream_read(stream, a, sizeof(a)); /*	fread(a, sizeof(a), 1, fp); */
	result->width = (unsigned short)a[0] | (((unsigned short)a[1])<<8);

	php_stream_read(stream, a, sizeof(a)); /*	fread(a, sizeof(a), 1, fp); */
	result->height = (unsigned short)a[0] | (((unsigned short)a[1])<<8);

	return result;
}
/* }}} */

/* {{{ php_handle_psd
 */
static struct gfxinfo *php_handle_psd (php_stream * stream)
{
	struct gfxinfo *result = NULL;
	unsigned char a[8];
	char temp[11];
	unsigned long in_width, in_height;

	result = (struct gfxinfo *) ecalloc(1, sizeof(struct gfxinfo));
	php_stream_read(stream, temp, sizeof(temp));

	if((php_stream_read(stream, a, sizeof(a))) <= 0) {
		in_height = 0;
		in_width  = 0;
	} else {
		in_height =  (((unsigned long) a[ 0 ]) << 24) + (((unsigned long) a[ 1 ]) << 16) + (((unsigned long) a[ 2 ]) << 8) + ((unsigned long) a[ 3 ]);
		in_width  =  (((unsigned long) a[ 4 ]) << 24) + (((unsigned long) a[ 5 ]) << 16) + (((unsigned long) a[ 6 ]) << 8) + ((unsigned long) a[ 7 ]);
	}

	result->width  = (unsigned int) in_width;
	result->height = (unsigned int) in_height;

	return result;
}
/* }}} */

/* {{{ php_handle_bmp
 */
static struct gfxinfo *php_handle_bmp (php_stream * stream)
{
	struct gfxinfo *result = NULL;
	char temp[15];

	struct {
		unsigned long in_width, in_height;
	} dim;

	result = (struct gfxinfo *) ecalloc (1, sizeof(struct gfxinfo));

	php_stream_read(stream, temp, sizeof(temp));
	php_stream_read(stream, (char*) &dim, sizeof(dim));
	result->width = dim.in_width;
	result->height = dim.in_height;

	return result;
}
/* }}} */

/* {{{ php_swf_get_bits
 * routines to handle SWF files. */
static unsigned long int php_swf_get_bits (unsigned char* buffer, unsigned int pos, unsigned int count)
{
	unsigned int loop;
	unsigned long int result = 0;

	for (loop = pos; loop < pos + count; loop++)
	{
		result = result +
			((((buffer[loop / 8]) >> (7 - (loop % 8))) & 0x01) << (count - (loop - pos) - 1));
	}
	return result;
}
/* }}} */

/* {{{ php_handle_swf
 */
static struct gfxinfo *php_handle_swf (php_stream * stream)
{
	struct gfxinfo *result = NULL;
	long bits;
	unsigned char a[32];

	result = (struct gfxinfo *) ecalloc (1, sizeof (struct gfxinfo));
	php_stream_seek(stream, 5, SEEK_CUR);

	php_stream_read(stream, a, sizeof(a)); /*	fread(a, sizeof(a), 1, fp); */
	bits = php_swf_get_bits (a, 0, 5);
	result->width = (php_swf_get_bits (a, 5 + bits, bits) -
		php_swf_get_bits (a, 5, bits)) / 20;
	result->height = (php_swf_get_bits (a, 5 + (3 * bits), bits) -
		php_swf_get_bits (a, 5 + (2 * bits), bits)) / 20;
	return result;
}
/* }}} */

/* {{{ php_handle_png
 * routine to handle PNG files */
static struct gfxinfo *php_handle_png (php_stream * stream)
{
	struct gfxinfo *result = NULL;
	unsigned long in_width, in_height;
	unsigned char a[8];

	result = (struct gfxinfo *) ecalloc(1, sizeof(struct gfxinfo));

	php_stream_seek(stream, 8, SEEK_CUR);

	if((php_stream_read(stream, a, sizeof(a))) <= 0) {
		in_width  = 0;
		in_height = 0;
	} else {
		in_width =  (((unsigned long) a[ 0 ]) << 24) + (((unsigned long) a[ 1 ]) << 16) + (((unsigned long) a[ 2 ]) << 8) + ((unsigned long) a[ 3 ]);
		in_height = (((unsigned long) a[ 4 ]) << 24) + (((unsigned long) a[ 5 ]) << 16) + (((unsigned long) a[ 6 ]) << 8) + ((unsigned long) a[ 7 ]);
	}

	result->width  = (unsigned int) in_width;
	result->height = (unsigned int) in_height;
	return result;
}
/* }}} */

/* routines to handle JPEG data */

/* some defines for the different JPEG block types */
#define M_SOF0  0xC0			/* Start Of Frame N */
#define M_SOF1  0xC1			/* N indicates which compression process */
#define M_SOF2  0xC2			/* Only SOF0-SOF2 are now in common use */
#define M_SOF3  0xC3
#define M_SOF5  0xC5			/* NB: codes C4 and CC are NOT SOF markers */
#define M_SOF6  0xC6
#define M_SOF7  0xC7
#define M_SOF9  0xC9
#define M_SOF10 0xCA
#define M_SOF11 0xCB
#define M_SOF13 0xCD
#define M_SOF14 0xCE
#define M_SOF15 0xCF
#define M_SOI   0xD8
#define M_EOI   0xD9			/* End Of Image (end of datastream) */
#define M_SOS   0xDA			/* Start Of Scan (begins compressed data) */
#define M_APP0  0xe0
#define M_APP1  0xe1
#define M_APP2  0xe2
#define M_APP3  0xe3
#define M_APP4  0xe4
#define M_APP5  0xe5
#define M_APP6  0xe6
#define M_APP7  0xe7
#define M_APP8  0xe8
#define M_APP9  0xe9
#define M_APP10 0xea
#define M_APP11 0xeb
#define M_APP12 0xec
#define M_APP13 0xed
#define M_APP14 0xee
#define M_APP15 0xef
#define M_COM   0xFE            /* COMment                                  */

#define M_PSEUDO 0xFFD8			/* pseudo marker for start of image(byte 0) */

/* {{{ php_read2
 */
static unsigned short php_read2(php_stream * stream)
{
	unsigned char a[2];

	/* just return 0 if we hit the end-of-file */
	if((php_stream_read(stream, a, sizeof(a))) <= 0) return 0;

	return (((unsigned short) a[ 0 ]) << 8) + ((unsigned short) a[ 1 ]);
}
/* }}} */

/* {{{ php_next_marker
 * get next marker byte from file */
static unsigned int php_next_marker(php_stream * stream, int last_marker, int comment_correction, int ff_read)
{
	int a=0, marker;

	/* get marker byte, swallowing possible padding                           */
	if ( last_marker==M_COM && comment_correction) {
		/* some software does not count the length bytes of COM section           */
		/* one company doing so is very much envolved in JPEG... so we accept too */
		/* by the way: some of those companies changed their code now...          */
		comment_correction = 2;
	} else {
		last_marker = 0;
		comment_correction = 0;
	}
	if ( ff_read) {
		a = 1; /* already read 0xff in filetype detection */
	}
	do {
		if ((marker = php_stream_getc(stream)) == EOF)
		{
			return M_EOI;/* we hit EOF */
		}
		if ( last_marker==M_COM && comment_correction>0)
		{
			if ( marker != 0xFF)
			{
				marker = 0xff;
				comment_correction--;
            } else {
				last_marker = M_PSEUDO; /* stop skipping non 0xff for M_COM */
			}
		}
		if ( ++a > 10)
		{
			/* who knows the maxim amount of 0xff? though 7 */
			/* but found other implementations              */
			return M_EOI;
		}
	} while ( marker == 0xff);
	if ( a < 2)
	{
		return M_EOI; /* at least one 0xff is needed before marker code */
	}
	if ( last_marker==M_COM && comment_correction)
	{
		return M_EOI; /* ah illegal: char after COM section not 0xFF */
	}
	return (unsigned int)marker;
}
/* }}} */

/* {{{ php_skip_variable
 * skip over a variable-length block; assumes proper length marker */
static void php_skip_variable(php_stream * stream)
{
	off_t length = ((unsigned int)php_read2(stream));

	length = length-2;
	if (length)
	{
		php_stream_seek(stream, (long)length, SEEK_CUR);
	}
}
/* }}} */

/* {{{ php_read_APP
 */
static void php_read_APP(php_stream * stream, unsigned int marker, zval *info)
{
	unsigned short length;
	unsigned char *buffer;
	unsigned char markername[ 16 ];
	zval *tmp;

	length = php_read2(stream);
	length -= 2;				/* length includes itself */

	buffer = emalloc(length);
	if ( !buffer) return;

 	if (php_stream_read(stream, buffer, (long) length) <= 0) {
		efree(buffer);
		return;
	}

	sprintf(markername, "APP%d", marker - M_APP0);

	if (zend_hash_find(Z_ARRVAL_P(info), markername, strlen(markername)+1, (void **) &tmp) == FAILURE) {
		/* XXX we onyl catch the 1st tag of it's kind! */
		add_assoc_stringl(info, markername, buffer, length, 1);
	}

	efree(buffer);
}
/* }}} */

/* {{{ php_handle_jpeg
   main loop to parse JPEG structure */
static struct gfxinfo *php_handle_jpeg (php_stream * stream, pval *info)
{
	struct gfxinfo *result = NULL;
	unsigned int marker = M_PSEUDO;
	unsigned short length, ff_read=1;

	for (;;) {
		marker = php_next_marker(stream, marker, 1, ff_read);
		ff_read = 0;
		switch (marker) {
			case M_SOF0:
			case M_SOF1:
			case M_SOF2:
			case M_SOF3:
			case M_SOF5:
			case M_SOF6:
			case M_SOF7:
			case M_SOF9:
			case M_SOF10:
			case M_SOF11:
			case M_SOF13:
			case M_SOF14:
			case M_SOF15:
				if (result == NULL) {
					/* handle SOFn block */
					result = (struct gfxinfo *) ecalloc(1, sizeof(struct gfxinfo));
					if ( !result)
						return NULL;
					length = php_read2(stream);
					result->bits     = php_stream_getc(stream);
					result->height   = php_read2(stream);
					result->width    = php_read2(stream);
					result->channels = php_stream_getc(stream);
					if (!info || length<8) /* if we don't want an extanded info -> return */
						return result;
					php_stream_seek(stream, length-8, SEEK_CUR);
				} else {
					php_skip_variable(stream);
				}
				break;

			case M_APP0:
			case M_APP1:
			case M_APP2:
			case M_APP3:
			case M_APP4:
			case M_APP5:
			case M_APP6:
			case M_APP7:
			case M_APP8:
			case M_APP9:
			case M_APP10:
			case M_APP11:
			case M_APP12:
			case M_APP13:
			case M_APP14:
			case M_APP15:
				if (info) {
					php_read_APP(stream, marker, info); /* read all the app markes... */
				} else {
					php_skip_variable(stream);
				}
				break;

			case M_SOS:
			case M_EOI:
				return result;	/* we're about to hit image data, or are at EOF. stop processing. */
				break;

			default:
				php_skip_variable(stream);		/* anything else isn't interesting */
				break;
		}
	}

	return result; /* perhaps image broken -> no info but size */
}
/* }}} */

/* {{{ jpeg2000 constants
  See ext/exif for more */
#define JC_SOC   0x4F		/* Start of codestream */
#define JC_SIZ   0x51		/* Image and tile size */
/* }}} */

/* {{{ php_read4
 */
static unsigned int php_read4(php_stream * stream)
{
	unsigned char a[4];

	/* just return 0 if we hit the end-of-file */
	if((php_stream_read(stream, a, sizeof(a))) <= 0) return 0;

	return (((unsigned int)a[0]) << 24)
	     + (((unsigned int)a[1]) << 16)
	     + (((unsigned int)a[2]) <<  8)
	     + (((unsigned int)a[3]));
}
/* }}} */

/* {{{ php_handle_tiff
   main loop to parse TIFF structure */
static struct gfxinfo *php_handle_jpc(php_stream * stream)
{
	struct gfxinfo *result = NULL;
	unsigned int marker, dummy;
	unsigned short length, ff_read = 1;

	marker = php_next_marker(stream, 0, 0, ff_read);
	ff_read = 0;
	if ( marker == JC_SIZ)
	{
		length           = php_read2(stream); /* Lsiz: length of segment */
		if ( length<42 || length>49191) /* read the spec */
			return NULL;
		result = (struct gfxinfo *) ecalloc(1, sizeof(struct gfxinfo));
		if ( !result)
			return NULL;
		dummy            = php_read2(stream); /* Rsiz: capabilities */
		result->height   = php_read4(stream); /* Xsiz */
		result->width    = php_read4(stream); /* Ysiz */
		dummy            = php_read4(stream); /* X0siz */
		dummy            = php_read4(stream); /* Y0siz */
		dummy            = php_read4(stream); /* XTsiz */
		dummy            = php_read4(stream); /* YTsiz */
		dummy            = php_read4(stream); /* XT0siz */
		dummy            = php_read4(stream); /* YT0siz */
		result->bits     = php_read2(stream); /* Csiz: precision in bitss */
		result->channels = 0; /* don't know yet */
		return result;
	}
	return result;
}
/* }}} */

/* {{{ tiff constants
 */
PHPAPI const int php_tiff_bytes_per_format[] = {0, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8};

/* uncompressed only */
#define TAG_IMAGEWIDTH              0x0100
#define TAG_IMAGEHEIGHT             0x0101
/* compressed images only */
#define TAG_COMP_IMAGEWIDTH         0xA002
#define TAG_COMP_IMAGEHEIGHT        0xA003

#define TAG_FMT_BYTE       1
#define TAG_FMT_STRING     2
#define TAG_FMT_USHORT     3
#define TAG_FMT_ULONG      4
#define TAG_FMT_URATIONAL  5
#define TAG_FMT_SBYTE      6
#define TAG_FMT_UNDEFINED  7
#define TAG_FMT_SSHORT     8
#define TAG_FMT_SLONG      9
#define TAG_FMT_SRATIONAL 10
#define TAG_FMT_SINGLE    11
#define TAG_FMT_DOUBLE    12

typedef unsigned char uchar;
/* }}} */

/* {{{ php_ifd_get16u
 * Convert a 16 bit unsigned value from file's native byte order */
static int php_ifd_get16u(void *Short, int motorola_intel)
{
	if (motorola_intel) {
		return (((uchar *)Short)[0] << 8) | ((uchar *)Short)[1];
	} else {
		return (((uchar *)Short)[1] << 8) | ((uchar *)Short)[0];
	}
}
/* }}} */

/* {{{ php_ifd_get16s
 * Convert a 16 bit signed value from file's native byte order */
static signed short php_ifd_get16s(void *Short, int motorola_intel)
{
	return (signed short)php_ifd_get16u(Short, motorola_intel);
}
/* }}} */

/* {{{ php_ifd_get32s
 * Convert a 32 bit signed value from file's native byte order */
static int php_ifd_get32s(void *Long, int motorola_intel)
{
	if (motorola_intel) {
		return  ((( char *)Long)[0] << 24) | (((uchar *)Long)[1] << 16)
			  | (((uchar *)Long)[2] << 8 ) | (((uchar *)Long)[3] << 0 );
	} else {
		return  ((( char *)Long)[3] << 24) | (((uchar *)Long)[2] << 16)
			  | (((uchar *)Long)[1] << 8 ) | (((uchar *)Long)[0] << 0 );
	}
}
/* }}} */

/* {{{ php_ifd_get32u
 * Convert a 32 bit unsigned value from file's native byte order */
static unsigned php_ifd_get32u(void *Long, int motorola_intel)
{
	return (unsigned)php_ifd_get32s(Long, motorola_intel) & 0xffffffff;
}
/* }}} */

/* {{{ php_handle_tiff
   main loop to parse TIFF structure */
static struct gfxinfo *php_handle_tiff (php_stream * stream, pval *info, int motorola_intel)
{
	struct gfxinfo *result = NULL;
	int i, num_entries;
	unsigned char *dir_entry;
	size_t ifd_size, dir_size, entry_value, entry_length, width, height, ifd_addr;
	int entry_tag , entry_type;
	char *ifd_data, ifd_ptr[4];

	php_stream_read(stream, ifd_ptr, 4);
	ifd_addr = php_ifd_get32u(ifd_ptr, motorola_intel);
	php_stream_seek(stream, ifd_addr-8, SEEK_CUR);
	ifd_size = 2;
	ifd_data = emalloc(ifd_size);
	php_stream_read(stream, ifd_data, 2);
    num_entries = php_ifd_get16u(ifd_data, motorola_intel);
	dir_size = 2/*num dir entries*/ +12/*length of entry*/*num_entries +4/* offset to next ifd (points to thumbnail or NULL)*/;
	ifd_size = dir_size;
	ifd_data = erealloc(ifd_data,ifd_size);
	php_stream_read(stream, ifd_data+2, dir_size-2);
	/* now we have the directory we can look how long it should be */
	ifd_size = dir_size;
	for(i=0;i<num_entries;i++) {
		dir_entry 	 = ifd_data+2+i*12;
		entry_tag    = php_ifd_get16u(dir_entry+0, motorola_intel);
		entry_type   = php_ifd_get16u(dir_entry+2, motorola_intel);
		entry_length = php_ifd_get32u(dir_entry+4, motorola_intel) * php_tiff_bytes_per_format[entry_type];
		switch(entry_type) {
			case TAG_FMT_BYTE:
			case TAG_FMT_SBYTE:
				entry_value  = (size_t)(dir_entry[8]);
				break;
			case TAG_FMT_USHORT:
				entry_value  = php_ifd_get16u(dir_entry+8, motorola_intel);
				break;
			case TAG_FMT_SSHORT:
				entry_value  = php_ifd_get16s(dir_entry+8, motorola_intel);
				break;
			case TAG_FMT_ULONG:
				entry_value  = php_ifd_get32u(dir_entry+8, motorola_intel);
				break;
			case TAG_FMT_SLONG:
				entry_value  = php_ifd_get32s(dir_entry+8, motorola_intel);
				break;
			default:
			    continue;
		}
		switch(entry_tag) {
			case TAG_IMAGEWIDTH:
			case TAG_COMP_IMAGEWIDTH:
				width  = entry_value;
				break;
			case TAG_IMAGEHEIGHT:
			case TAG_COMP_IMAGEHEIGHT:
				height = entry_value;
				break;
		}
	}
	efree(ifd_data);
	if ( width && height) {
		/* not the same when in for-loop */
		result = (struct gfxinfo *) ecalloc(1, sizeof(struct gfxinfo));
		result->height = height;
		result->width  = width;
		return result;
	}
	return NULL;
}
/* }}} */

/* {{{ php_imagetype
   detect filetype from first bytes */
int php_getimagetype(php_stream * stream, char *filetype)
{
	char tmp[8];

	if ( !filetype) filetype = tmp;
	if((php_stream_read(stream, filetype, 3)) <= 0) {
		php_error(E_WARNING, "getimagesize: Read error!");
		return IMAGE_FILETYPE_UNKNOWN;
	}

	if (!memcmp(filetype, php_sig_gif, 3)) {
		return IMAGE_FILETYPE_GIF;
	} else if (!memcmp(filetype, php_sig_jpg, 3)) {
		return IMAGE_FILETYPE_JPEG;
	} else if (!memcmp(filetype, php_sig_png, 3)) {
		php_stream_read(stream, filetype+3, 5);
		if (!memcmp(filetype, php_sig_png, 8)) {
			return IMAGE_FILETYPE_PNG;
		} else {
			php_error(E_WARNING, "PNG file corrupted by ASCII conversion");
			return IMAGE_FILETYPE_UNKNOWN;
		}
	} else if (!memcmp(filetype, php_sig_swf, 3)) {
		return IMAGE_FILETYPE_SWF;
	} else if (!memcmp(filetype, php_sig_psd, 3)) {
		return IMAGE_FILETYPE_PSD;
	} else if (!memcmp(filetype, php_sig_bmp, 2)) {
		return IMAGE_FILETYPE_BMP;
	} else if (!memcmp(filetype, php_sig_jpc, 3)) {
		return IMAGE_FILETYPE_JPC;
	} else {
		php_stream_read(stream, filetype+3, 1);
		if (!memcmp(filetype, php_sig_tif_ii, 4)) {
			return IMAGE_FILETYPE_TIFF_II;
		} else
		if (!memcmp(filetype, php_sig_tif_mm, 4)) {
			return IMAGE_FILETYPE_TIFF_MM;
		}
	}

	return IMAGE_FILETYPE_UNKNOWN;
}
/* }}} */

/* {{{ proto array getimagesize(string imagefile [, array info])
   Get the size of an image as 4-element array */
PHP_FUNCTION(getimagesize)
{
	zval **arg1, **info = NULL;
	int rsrc_id;
 	int itype = 0;
	char temp[64];
	struct gfxinfo *result = NULL;
	php_stream * stream = NULL;

	switch(ZEND_NUM_ARGS()) {

	case 1:
		if (zend_get_parameters_ex(1, &arg1) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		convert_to_string_ex(arg1);
		break;

	case 2:
		if (zend_get_parameters_ex(2, &arg1, &info) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		zval_dtor(*info);

		if (array_init(*info) == FAILURE) {
			return;
		}

		convert_to_string_ex(arg1);
		break;

	default:
		WRONG_PARAM_COUNT;
		break;
	}

	stream = php_stream_open_wrapper(Z_STRVAL_PP(arg1), "rb", REPORT_ERRORS|IGNORE_PATH|ENFORCE_SAFE_MODE, NULL);

	if (!stream) {
		RETURN_FALSE;
	}

	rsrc_id = ZEND_REGISTER_RESOURCE(NULL, stream, php_file_le_stream());

	itype = php_getimagetype(stream, NULL);
	switch( itype) {
        case IMAGE_FILETYPE_GIF:
			result = php_handle_gif (stream);
	        break;
        case IMAGE_FILETYPE_JPEG:
			if (info) {
				result = php_handle_jpeg(stream, *info);
			} else {
				result = php_handle_jpeg(stream, NULL);
			}
	        break;
        case IMAGE_FILETYPE_PNG:
			result = php_handle_png(stream);
	        break;
        case IMAGE_FILETYPE_SWF:
			result = php_handle_swf(stream);
	        break;
        case IMAGE_FILETYPE_PSD:
			result = php_handle_psd(stream);
	        break;
        case IMAGE_FILETYPE_BMP:
			result = php_handle_bmp(stream);
	        break;
        case IMAGE_FILETYPE_TIFF_II:
			result = php_handle_tiff(stream, NULL, 0);
	        break;
        case IMAGE_FILETYPE_TIFF_MM:
			result = php_handle_tiff(stream, NULL, 1);
	        break;
        case IMAGE_FILETYPE_JPC:
			result = php_handle_jpc(stream);
	        break;
		default:
        case IMAGE_FILETYPE_UNKNOWN:
	        break;
	}

	zend_list_delete(rsrc_id);

	if (result) {
		if (array_init(return_value) == FAILURE) {
			php_error(E_ERROR, "Unable to initialize array");
			efree(result);
			return;
		}
		add_index_long(return_value, 0, result->width);
		add_index_long(return_value, 1, result->height);
		add_index_long(return_value, 2, itype);
		sprintf(temp, "width=\"%d\" height=\"%d\"", result->width, result->height); /* safe */
		add_index_string(return_value, 3, temp, 1);

		if (result->bits != 0) {
			add_assoc_long(return_value, "bits", result->bits);
		}
		if (result->channels != 0) {
			add_assoc_long(return_value, "channels", result->channels);
		}
		efree(result);
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
