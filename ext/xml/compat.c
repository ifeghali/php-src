/*
   +----------------------------------------------------------------------+
   | PHP Version 4                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2003 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Sterling Hughes <sterling@php.net>                          |
   +----------------------------------------------------------------------+
 */

#include "php.h"
#if HAVE_LIBXML && HAVE_XML
#include "expat_compat.h"

typedef struct _php_xml_ns {
	xmlNsPtr nsptr;
	int ref_count;
	void *next;
	void *prev;
} php_xml_ns;

#ifdef LIBXML_EXPAT_COMPAT

#define IS_NS_DECL(__ns) \
	((__ns) != NULL && strlen(__ns) == 5 && *(__ns) == 'x' && *((__ns)+1) == 'm' && \
	 *((__ns)+2) == 'l' && *((__ns)+3) == 'n' && *((__ns)+4) == 's')

static void
_find_namespace_decl(XML_Parser parser, const xmlChar *tagname, const xmlChar **attr)
{
	xmlChar **attr_p = (xmlChar **) attr;
	xmlChar  *name;
	xmlChar  *value;
	xmlChar  *partial;
	xmlChar  *namespace;
	php_xml_ns *cur_ns_scope = NULL;
	php_xml_ns *exist_ns_scope;
	xmlNsPtr nsptr, curnsptr;

	exist_ns_scope = xmlHashLookup(parser->_reverse_ns_map, tagname);

	if (exist_ns_scope) {
		while (exist_ns_scope->next != NULL)
			exist_ns_scope = exist_ns_scope->next;
	}

	while (attr_p && *attr_p) {
		name = attr_p[0];
		value = xmlStrdup(attr_p[1]);

		partial = xmlSplitQName(parser->parser, name, &namespace);

		if (IS_NS_DECL(namespace)) {

			if (parser->h_start_ns) {
				parser->h_start_ns(parser->user, partial, (const XML_Char *) value);
			}
			if (xmlHashLookup(parser->_ns_map, partial) == NULL) {
				xmlHashAddEntry(parser->_ns_map, partial, value);
			} else {
				xmlFree(value);
			}

			nsptr = (xmlNsPtr) xmlMalloc(sizeof(xmlNs));

			if (nsptr) {
				memset(nsptr, 0, sizeof(xmlNs));
				nsptr->type = XML_LOCAL_NAMESPACE;

				if (value != NULL)
					nsptr->href = xmlStrdup(value); 
				if (partial != NULL)
					nsptr->prefix = xmlStrdup(partial);

				if (cur_ns_scope == NULL) {
					cur_ns_scope = emalloc(sizeof(php_xml_ns));
					cur_ns_scope->next = NULL;
					cur_ns_scope->prev = NULL;
					cur_ns_scope->nsptr = nsptr;
					cur_ns_scope->ref_count = 0;

					if (exist_ns_scope) {
						exist_ns_scope->next = cur_ns_scope;
						cur_ns_scope->prev = exist_ns_scope;
					} else {
						xmlHashAddEntry(parser->_reverse_ns_map, tagname, cur_ns_scope);
					}

					exist_ns_scope = cur_ns_scope;
				} else {
					curnsptr = cur_ns_scope->nsptr;
					while (curnsptr->next != NULL) {
						curnsptr = curnsptr->next;
					}
					curnsptr->next = nsptr;
				}
			}

		} else {
			xmlFree(value);
		}

		xmlFree(partial);
		if (namespace != NULL) {
			xmlFree(namespace);
		}

		attr_p += 2;
	}

	if (exist_ns_scope) {
		exist_ns_scope->ref_count++;
	}
}

static void 
_qualify_namespace(XML_Parser parser, const xmlChar *name, xmlChar **qualified)
{
	xmlChar *partial;
	xmlChar *namespace;
		
	partial = xmlSplitQName(parser->parser, name, &namespace);
	if (namespace) {
		xmlChar *nsvalue;

		nsvalue = xmlHashLookup(parser->_ns_map, namespace);
		if (nsvalue) {
			/* Use libxml functions otherwise its memory deallocation is screwed up */
			*qualified = xmlStrdup(nsvalue);
			*qualified = xmlStrncat(*qualified, parser->_ns_seperator, 1);
			*qualified = xmlStrncat(*qualified, partial, strlen(partial));
		} else {
			*qualified = xmlStrdup(name);
		}
		xmlFree(namespace);
	} else {
		*qualified = xmlStrdup(name);
	}
	
	xmlFree(partial);
}

static void
_start_element_handler(void *user, const xmlChar *name, const xmlChar **attributes)
{
	XML_Parser  parser = (XML_Parser) user;
	xmlChar    *qualified_name = NULL;

	if (parser->h_start_element == NULL) {
		return;
	}
	
	if (parser->use_namespace) {
		_find_namespace_decl(parser, name, attributes);
		_qualify_namespace(parser, name, &qualified_name);
	} else {
		qualified_name = xmlStrdup(name);
	}

	parser->h_start_element(parser->user, (const XML_Char *) qualified_name, (const XML_Char **) attributes);

	xmlFree(qualified_name);
}

static void
_namespace_handler(XML_Parser parser, xmlNsPtr nsptr)
{
	if (nsptr != NULL) {
		_namespace_handler(parser, nsptr->next);
		parser->h_end_ns(parser->user, nsptr->prefix);
	}
}

static void
_end_element_handler(void *user, const xmlChar *name)
{
	xmlChar    *qualified_name;
	XML_Parser  parser = (XML_Parser) user;

	if (parser->h_end_element == NULL) {
		return;
	}
	
	if (parser->use_namespace) {
		_qualify_namespace(parser, name, &qualified_name);
	} else {
		qualified_name = xmlStrdup(name);
	}

	parser->h_end_element(parser->user, (const XML_Char *) qualified_name);

	if (parser->use_namespace) {
		int			tag_counter;
		php_xml_ns *cur_ns_scope, *prev_ns_scope;
		xmlNsPtr nsptr;

		cur_ns_scope = xmlHashLookup(parser->_reverse_ns_map, name);
		if (cur_ns_scope) {
			while (cur_ns_scope->next != NULL) {
				cur_ns_scope = cur_ns_scope->next;
			}
			tag_counter = --cur_ns_scope->ref_count;
			if (tag_counter == 0) {
				nsptr = cur_ns_scope->nsptr;
				if (nsptr && parser->h_end_ns) {
					_namespace_handler(parser, nsptr);
				}
				xmlFreeNsList(nsptr);
				cur_ns_scope->nsptr = NULL;
				prev_ns_scope = cur_ns_scope->prev;
				if (prev_ns_scope != NULL) {
					efree(cur_ns_scope);
					prev_ns_scope->next = NULL;
				}
			}
		}
	}

	xmlFree(qualified_name);
}

static void
_cdata_handler(void *user, const xmlChar *cdata, int cdata_len)
{
	XML_Parser parser = (XML_Parser) user;

	if (parser->h_cdata == NULL) {
		return;
	}

	parser->h_cdata(parser->user, (const XML_Char *) cdata, cdata_len);
}

static void
_pi_handler(void *user, const xmlChar *target, const xmlChar *data)
{
	XML_Parser parser = (XML_Parser) user;

	if (parser->h_pi == NULL) {
		return;
	}

	parser->h_pi(parser->user, (const XML_Char *) target, (const XML_Char *) data);
}

static void
_unparsed_entity_decl_handler(void *user, 
                              const xmlChar *name, 
							  const xmlChar *sys_id, 
							  const xmlChar *pub_id, 
							  const xmlChar *notation)
{
	XML_Parser parser = (XML_Parser) user;

	if (parser->h_unparsed_entity_decl == NULL) {
		return;
	}

	parser->h_unparsed_entity_decl(parser->user, name, NULL, sys_id, pub_id, notation);
}

static void
_notation_decl_handler(void *user, const xmlChar *notation, const xmlChar *sys_id, const xmlChar *pub_id)
{
	XML_Parser parser = (XML_Parser) user;

	if (parser->h_notation_decl == NULL) {
		return;
	}

	parser->h_notation_decl(parser->user, notation, NULL, sys_id, pub_id);
}

static void
_build_comment(const xmlChar *data, int data_len, xmlChar **comment, int *comment_len)
{
	*comment_len = data_len + 6;
	
	*comment = xmlMalloc(*comment_len + 1);
	memcpy(*comment, "<--", 3);
	memcpy(*comment + 3, data, data_len);
	memcpy(*comment + 3 + data_len, "-->", 3);

	(*comment)[*comment_len] = '\0';
}

static void
_comment_handler(void *user, const xmlChar *comment)
{
	XML_Parser parser = (XML_Parser) user;

	if (parser->h_default) {
		xmlChar *d_comment;
		int      d_comment_len;

		_build_comment(comment, xmlStrlen(comment), &d_comment, &d_comment_len);
		parser->h_default(parser->user, d_comment, d_comment_len);
	}
}

static void
_build_entity(const xmlChar *name, int len, xmlChar **entity, int *entity_len) 
{
	*entity_len = len + 2;
	*entity = xmlMalloc(*entity_len + 1);
	(*entity)[0] = '&';
	memcpy(*entity+1, name, len);
	(*entity)[len+1] = ';';
	(*entity)[*entity_len] = '\0';
}

static xmlEntityPtr
_get_entity(void *user, const xmlChar *name)
{
	XML_Parser parser = (XML_Parser) user;

	if (parser->h_default) {
		xmlChar *entity;
		int      len;
		
		_build_entity(name, xmlStrlen(name), &entity, &len);
		parser->h_default(parser->user, (const xmlChar *) entity, len);
	}

	return NULL;
}

static void
_external_entity_ref_handler(void *user, const xmlChar *names, int type, const xmlChar *sys_id, const xmlChar *pub_id, xmlChar *content)
{
	XML_Parser parser = (XML_Parser) user;

	if (parser->h_external_entity_ref == NULL) {
		return;
	}

	parser->h_external_entity_ref(parser, names, "", sys_id, pub_id);
}

static xmlSAXHandler 
php_xml_compat_handlers = {
    NULL, /* internalSubset */
	NULL, /* isStandalone */
	NULL, /* hasInternalSubset */
	NULL, /* hasExternalSubset */
	NULL, /* resolveEntity */
	_get_entity, /* getEntity */
	_external_entity_ref_handler, /* entityDecl */
	_notation_decl_handler,
	NULL, /* attributeDecl */
	NULL, /* elementDecl */
	_unparsed_entity_decl_handler, /* unparsedEntity */
	NULL, /* setDocumentLocator */
	NULL, /* startDocument */
	NULL, /* endDocument */
	_start_element_handler,
	_end_element_handler,
	NULL, /* reference */
	_cdata_handler,
	NULL, /* ignorableWhitespace */
	_pi_handler,
	_comment_handler, /* comment */
	NULL, /* warning */
	NULL, /* error */
	NULL,  /* fatalError */
	NULL,  /* getParameterEntity */
	_cdata_handler, /* cdataBlock */
	NULL, /* externalSubset */
	1
};

XML_Parser 
XML_ParserCreate(const XML_Char *encoding)
{
	return XML_ParserCreate_MM(encoding, NULL, NULL);
}

XML_Parser
XML_ParserCreateNS(const XML_Char *encoding, const XML_Char sep)
{
	XML_Char tmp[2];
	tmp[0] = sep;
	tmp[1] = '\0';
	return XML_ParserCreate_MM(encoding, NULL, tmp);
}

XML_Parser
XML_ParserCreate_MM(const XML_Char *encoding, const XML_Memory_Handling_Suite *memsuite, const XML_Char *sep)
{
	XML_Parser parser;

	parser = (XML_Parser) emalloc(sizeof(struct _XML_Parser));
	memset(parser, 0, sizeof(struct _XML_Parser));
	parser->use_namespace = 0;
	parser->_ns_seperator = NULL;

	parser->parser = xmlCreatePushParserCtxt((xmlSAXHandlerPtr) &php_xml_compat_handlers, (void *) parser, NULL, 0, NULL);
	if (parser->parser == NULL) {
		efree(parser);
		return NULL;
	}
	if (encoding != NULL) {
		parser->parser->encoding = xmlStrdup(encoding);
	} else {
		parser->parser->charset = XML_CHAR_ENCODING_NONE;
	}
	parser->parser->replaceEntities=1;
	if (sep != NULL) {
		parser->use_namespace = 1;
		parser->_ns_map = xmlHashCreate(10);
		parser->_reverse_ns_map = xmlHashCreate(10);
		parser->_ns_seperator = xmlStrdup(sep);
	}
	return parser;
}

void
XML_SetUserData(XML_Parser parser, void *user)
{
	parser->user = user;
}

void *
XML_GetUserData(XML_Parser parser)
{
	return parser->user;
}

void
XML_SetElementHandler(XML_Parser parser, XML_StartElementHandler start, XML_EndElementHandler end)
{
	parser->h_start_element = start;
	parser->h_end_element = end;
}

void
XML_SetCharacterDataHandler(XML_Parser parser, XML_CharacterDataHandler cdata)
{
	parser->h_cdata = cdata;
}

void
XML_SetProcessingInstructionHandler(XML_Parser parser, XML_ProcessingInstructionHandler pi)
{
	parser->h_pi = pi;
}

void
XML_SetCommentHandler(XML_Parser parser, XML_CommentHandler comment)
{
	parser->h_comment = comment;
}

void 
XML_SetDefaultHandler(XML_Parser parser, XML_DefaultHandler d)
{
	parser->h_default = d;
}

void
XML_SetUnparsedEntityDeclHandler(XML_Parser parser, XML_UnparsedEntityDeclHandler unparsed_decl)
{
	parser->h_unparsed_entity_decl = unparsed_decl;
}

void
XML_SetNotationDeclHandler(XML_Parser parser, XML_NotationDeclHandler notation_decl)
{
	parser->h_notation_decl = notation_decl;
}

void
XML_SetExternalEntityRefHandler(XML_Parser parser, XML_ExternalEntityRefHandler ext_entity)
{
	parser->h_external_entity_ref = ext_entity;
}

void
XML_SetStartNamespaceDeclHandler(XML_Parser parser, XML_StartNamespaceDeclHandler start_ns)
{
	parser->h_start_ns = start_ns;
}

void
XML_SetEndNamespaceDeclHandler(XML_Parser parser, XML_EndNamespaceDeclHandler end_ns)
{
	parser->h_end_ns = end_ns;
}

int
XML_Parse(XML_Parser parser, const XML_Char *data, int data_len, int is_final)
{
	return !xmlParseChunk(parser->parser, data, data_len, is_final);
}

int
XML_GetErrorCode(XML_Parser parser)
{
	return parser->parser->errNo;
}

const XML_Char *error_mapping[] = {
	"No error",
	"Internal error",
	"No memory",
	"XML_ERR_DOCUMENT_START",
	"Empty document",
	"XML_ERR_DOCUMENT_END",
	"Invalid hexadecimal character reference",
	"Invalid decimal character reference",
	"Invalid character reference",
	"Invalid character",
	"XML_ERR_CHARREF_AT_EOF",
	"XML_ERR_CHARREF_IN_PROLOG",
	"XML_ERR_CHARREF_IN_EPILOG",
	"XML_ERR_CHARREF_IN_DTD",
	"XML_ERR_ENTITYREF_AT_EOF",
	"XML_ERR_ENTITYREF_IN_PROLOG",
	"XML_ERR_ENTITYREF_IN_EPILOG",
	"XML_ERR_ENTITYREF_IN_DTD",
	"XML_ERR_PEREF_AT_EOF",
	"XML_ERR_PEREF_IN_PROLOG",
	"XML_ERR_PEREF_IN_EPILOG",
	"XML_ERR_PEREF_IN_INT_SUBSET",
	"XML_ERR_ENTITYREF_NO_NAME",
	"XML_ERR_ENTITYREF_SEMICOL_MISSING",
	"XML_ERR_PEREF_NO_NAME",
	"XML_ERR_PEREF_SEMICOL_MISSING",
	"Undeclared entity error",
	"Undeclared entity warning",
	"Unparsed Entity",
	"XML_ERR_ENTITY_IS_EXTERNAL",
	"XML_ERR_ENTITY_IS_PARAMETER",
	"Unknown encoding",
	"Unsupported encoding",
	"XML_ERR_STRING_NOT_STARTED",
	"XML_ERR_STRING_NOT_CLOSED",
	"Namespace declaration error",
	"XML_ERR_ENTITY_NOT_STARTED",
	"XML_ERR_ENTITY_NOT_FINISHED",
	"XML_ERR_LT_IN_ATTRIBUTE",
	"XML_ERR_ATTRIBUTE_NOT_STARTED",
	"XML_ERR_ATTRIBUTE_NOT_FINISHED",
	"XML_ERR_ATTRIBUTE_WITHOUT_VALUE",
	"XML_ERR_ATTRIBUTE_REDEFINED",
	"XML_ERR_LITERAL_NOT_STARTED",
	"XML_ERR_LITERAL_NOT_FINISHED",
	/* "XML_ERR_COMMENT_NOT_STARTED", <= eliminated on purpose */
	"XML_ERR_COMMENT_NOT_FINISHED",
	"XML_ERR_PI_NOT_STARTED",
	"XML_ERR_PI_NOT_FINISHED",
	"XML_ERR_NOTATION_NOT_STARTED",
	"XML_ERR_NOTATION_NOT_FINISHED",
	"XML_ERR_ATTLIST_NOT_STARTED",
	"XML_ERR_ATTLIST_NOT_FINISHED",
	"XML_ERR_MIXED_NOT_STARTED",
	"XML_ERR_MIXED_NOT_FINISHED",
	"XML_ERR_ELEMCONTENT_NOT_STARTED",
	"XML_ERR_ELEMCONTENT_NOT_FINISHED",
	"XML_ERR_XMLDECL_NOT_STARTED",
	"XML_ERR_XMLDECL_NOT_FINISHED",
	"XML_ERR_CONDSEC_NOT_STARTED",
	"XML_ERR_CONDSEC_NOT_FINISHED",
	"XML_ERR_EXT_SUBSET_NOT_FINISHED",
	"XML_ERR_DOCTYPE_NOT_FINISHED",
	"XML_ERR_MISPLACED_CDATA_END",
	"XML_ERR_CDATA_NOT_FINISHED",
	"XML_ERR_RESERVED_XML_NAME",
	"XML_ERR_SPACE_REQUIRED",
	"XML_ERR_SEPARATOR_REQUIRED",
	"XML_ERR_NMTOKEN_REQUIRED",
	"XML_ERR_NAME_REQUIRED",
	"XML_ERR_PCDATA_REQUIRED",
	"XML_ERR_URI_REQUIRED",
	"XML_ERR_PUBID_REQUIRED",
	"XML_ERR_LT_REQUIRED",
	"XML_ERR_GT_REQUIRED",
	"XML_ERR_LTSLASH_REQUIRED",
	"XML_ERR_EQUAL_REQUIRED",
	"XML_ERR_TAG_NAME_MISMATCH",
	"XML_ERR_TAG_NOT_FINISHED",
	"XML_ERR_STANDALONE_VALUE",
	"XML_ERR_ENCODING_NAME",
	"XML_ERR_HYPHEN_IN_COMMENT",
	"Invalid encoding",
	"XML_ERR_EXT_ENTITY_STANDALONE",
	"XML_ERR_CONDSEC_INVALID",
	"XML_ERR_VALUE_REQUIRED",
	"XML_ERR_NOT_WELL_BALANCED",
	"XML_ERR_EXTRA_CONTENT",
    "XML_ERR_ENTITY_CHAR_ERROR",
    "XML_ERR_ENTITY_PE_INTERNAL",
    "XML_ERR_ENTITY_LOOP",
    "XML_ERR_ENTITY_BOUNDARY",
    "Invalid URI",
    "XML_ERR_URI_FRAGMENT",
    "XML_WAR_CATALOG_PI",
    "XML_ERR_NO_DTD"
};

const XML_Char *
XML_ErrorString(int code)
{
	if (code < 0 || code >= (int)sizeof(error_mapping)) {
		return "Unknown";
	}
	return error_mapping[code];
}

int
XML_GetCurrentLineNumber(XML_Parser parser)
{
	return parser->parser->input->line;
}

int
XML_GetCurrentColumnNumber(XML_Parser parser)
{
	return parser->parser->input->col;
}

int
XML_GetCurrentByteIndex(XML_Parser parser)
{
	return parser->parser->input->consumed +
			(parser->parser->input->cur - parser->parser->input->base);
}

const XML_Char *XML_ExpatVersion(void)
{
	return "1.0";
}

static void
_free_ns_name(void *ptr, xmlChar *name)
{
	xmlFree(ptr);
}

static void
_free_ns_pointer(void *ptr, xmlChar *name)
{
	php_xml_ns *cur_ns_scope;

	/* Child scopes should already be removed, but in the event 
	of malformed xml, they may still be resident and need to be cleaned */
	cur_ns_scope = ((php_xml_ns *) ptr)->next;
	if (cur_ns_scope != NULL) {
		_free_ns_pointer(cur_ns_scope, NULL);
	}

	xmlFreeNsList(((php_xml_ns *) ptr)->nsptr);

	efree(ptr);
}

void
XML_ParserFree(XML_Parser parser)
{
	if (parser->use_namespace) {
		xmlHashFree(parser->_ns_map,         _free_ns_name);
		xmlHashFree(parser->_reverse_ns_map, _free_ns_pointer);
		if (parser->_ns_seperator) {
			xmlFree(parser->_ns_seperator);
		}
	}
	xmlFreeParserCtxt(parser->parser);
	efree(parser);
}

#endif /* LIBXML_EXPAT_COMPAT */
#endif

/**
 * Local Variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * vim600: fdm=marker
 * vim: ts=4 noet sw=4
 */
