/* 
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_01.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author:  Andrei Zmievski <andrei@ispi.net>                           |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#ifndef PHP_WDDX_API_H
#define PHP_WDDX_API_H

#define WDDX_ARRAY_S			"<array length='%d'>"
#define WDDX_ARRAY_E			"</array>"
#define WDDX_BOOLEAN			"<boolean value='%s'/>"
#define WDDX_CHAR				"<char code='%02X'/>"
#define WDDX_COMMENT_S			"<comment>"
#define WDDX_COMMENT_E			"</comment>"
#define WDDX_DATA_S				"<data>"
#define WDDX_DATA_E				"</data>"
#define WDDX_HEADER				"<header/>"
#define WDDX_HEADER_S			"<header>"
#define WDDX_HEADER_E			"</header>"
#define WDDX_NULL				"<null/>"
#define WDDX_NUMBER				"<number>%s</number>"
#define WDDX_PACKET_S			"<wddxPacket version='1.0'>"
#define WDDX_PACKET_E			"</wddxPacket>"
#define WDDX_STRING_S			"<string>"
#define WDDX_STRING_E			"</string>"
#define WDDX_STRUCT_S			"<struct>"
#define WDDX_STRUCT_E			"</struct>"
#define WDDX_VAR_S				"<var name='%s'>"
#define WDDX_VAR_E				"</var>"

typedef struct _wddx_packet wddx_packet;

wddx_packet *php_wddx_constructor(void);
void 		 php_wddx_destructor(wddx_packet *packet);

void 		 php_wddx_packet_start(wddx_packet *packet, char *comment);
void 		 php_wddx_packet_end(wddx_packet *packet);

void 		 php_wddx_serialize_var(wddx_packet *packet, zval *var, char *name);
void 		 php_wddx_add_chunk(wddx_packet *packet, char *str);
void 		 php_wddx_deserialize_ex(char *, int, zval *return_value);
char		*php_wddx_gather(wddx_packet *packet);

#endif /* PHP_WDDX_API_H */
