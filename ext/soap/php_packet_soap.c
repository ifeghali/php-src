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
  | Authors: Brad Lafountain <rodif_bl@yahoo.com>                        |
  |          Shane Caraveo <shane@caraveo.com>                           |
  |          Dmitry Stogov <dmitry@zend.com>                             |
  +----------------------------------------------------------------------+
*/
/* $Id$ */

#include "php_soap.h"

/* SOAP client calls this function to parse response from SOAP server */
int parse_packet_soap(zval *this_ptr, char *buffer, int buffer_size, sdlFunctionPtr fn, char *fn_name, zval *return_value, zval *soap_headers TSRMLS_DC)
{
	char* envelope_ns = NULL;
	xmlDocPtr response;
	xmlNodePtr trav, env, head, body, resp, cur, fault;
	xmlAttrPtr attr;
	int param_count = 0;
	int old_error_reporting;
	int soap_version;

	ZVAL_NULL(return_value);

	old_error_reporting = EG(error_reporting);
	EG(error_reporting) &= ~(E_WARNING|E_NOTICE|E_USER_WARNING|E_USER_NOTICE);

	/* Parse XML packet */
	response = xmlParseMemory(buffer, buffer_size);
	xmlCleanupParser();

	EG(error_reporting) = old_error_reporting;

	if (!response) {
		add_soap_fault(this_ptr, "Client", "looks like we got no XML document", NULL, NULL TSRMLS_CC);
		return FALSE;
	}
	if (xmlGetIntSubset(response) != NULL) {
		add_soap_fault(this_ptr, "Client", "DTD are not supported by SOAP", NULL, NULL TSRMLS_CC);
		xmlFreeDoc(response);
		return FALSE;
	}

	/* Get <Envelope> element */
	env = NULL;
	trav = response->children;
	while (trav != NULL) {
		if (trav->type == XML_ELEMENT_NODE) {
			if (env == NULL && node_is_equal_ex(trav,"Envelope",SOAP_1_1_ENV_NAMESPACE)) {
				env = trav;
				envelope_ns = SOAP_1_1_ENV_NAMESPACE;
				soap_version = SOAP_1_1;
			} else if (env == NULL && node_is_equal_ex(trav,"Envelope",SOAP_1_2_ENV_NAMESPACE)) {
				env = trav;
				envelope_ns = SOAP_1_2_ENV_NAMESPACE;
				soap_version = SOAP_1_2;
			} else {
				add_soap_fault(this_ptr, "VersionMismatch", "Wrong Version", NULL, NULL TSRMLS_CC);
				xmlFreeDoc(response);
				return FALSE;
			}
		}
		trav = trav->next;
	}
	if (env == NULL) {
		add_soap_fault(this_ptr, "Client", "looks like we got XML without \"Envelope\" element", NULL, NULL TSRMLS_CC);
		xmlFreeDoc(response);
		return FALSE;
	}

	attr = env->properties;
	while (attr != NULL) {
		if (attr->ns == NULL) {
			add_soap_fault(this_ptr, "Client", "A SOAP Envelope element cannot have non Namespace qualified attributes", NULL, NULL TSRMLS_CC);
			xmlFreeDoc(response);
			return FALSE;
		} else if (attr_is_equal_ex(attr,"encodingStyle",SOAP_1_2_ENV_NAMESPACE)) {
			if (soap_version == SOAP_1_2) {
				add_soap_fault(this_ptr, "Client", "encodingStyle cannot be specified on the Envelope", NULL, NULL TSRMLS_CC);
				xmlFreeDoc(response);
				return FALSE;
			} else if (strcmp(attr->children->content,SOAP_1_1_ENC_NAMESPACE) != 0) {
				add_soap_fault(this_ptr, "Client", "Unknown data encoding style", NULL, NULL TSRMLS_CC);
				xmlFreeDoc(response);
				return FALSE;
			}
		}
		attr = attr->next;
	}

	/* Get <Header> element */
	head = NULL;
	trav = env->children;
	while (trav != NULL && trav->type != XML_ELEMENT_NODE) {
		trav = trav->next;
	}
	if (trav != NULL && node_is_equal_ex(trav,"Header",envelope_ns)) {
		head = trav;
		trav = trav->next;
	}

	/* Get <Body> element */
	body = NULL;
	while (trav != NULL && trav->type != XML_ELEMENT_NODE) {
		trav = trav->next;
	}
	if (trav != NULL && node_is_equal_ex(trav,"Body",envelope_ns)) {
		body = trav;
		trav = trav->next;
	}
	while (trav != NULL && trav->type != XML_ELEMENT_NODE) {
		trav = trav->next;
	}
	if (body == NULL) {
		add_soap_fault(this_ptr, "Client", "Body must be present in a SOAP envelope", NULL, NULL TSRMLS_CC);
		xmlFreeDoc(response);
		return FALSE;
	}
	attr = body->properties;
	while (attr != NULL) {
		if (attr->ns == NULL) {
			if (soap_version == SOAP_1_2) {
				add_soap_fault(this_ptr, "Client", "A SOAP Body element cannot have non Namespace qualified attributes", NULL, NULL TSRMLS_CC);
				xmlFreeDoc(response);
				return FALSE;
			}
		} else if (attr_is_equal_ex(attr,"encodingStyle",SOAP_1_2_ENV_NAMESPACE)) {
			if (soap_version == SOAP_1_2) {
				add_soap_fault(this_ptr, "Client", "encodingStyle cannot be specified on the Body", NULL, NULL TSRMLS_CC);
				xmlFreeDoc(response);
				return FALSE;
			} else if (strcmp(attr->children->content,SOAP_1_1_ENC_NAMESPACE) != 0) {
				add_soap_fault(this_ptr, "Client", "Unknown data encoding style", NULL, NULL TSRMLS_CC);
				xmlFreeDoc(response);
				return FALSE;
			}
		}
		attr = attr->next;
	}
	if (trav != NULL && soap_version == SOAP_1_2) {
		add_soap_fault(this_ptr, "Client", "A SOAP 1.2 envelope can contain only Header and Body", NULL, NULL TSRMLS_CC);
		xmlFreeDoc(response);
		return FALSE;
	}

	if (head != NULL) {
		attr = head->properties;
		while (attr != NULL) {
			if (attr->ns == NULL) {
				add_soap_fault(this_ptr, "Client", "A SOAP Header element cannot have non Namespace qualified attributes", NULL, NULL TSRMLS_CC);
				xmlFreeDoc(response);
				return FALSE;
			} else if (attr_is_equal_ex(attr,"encodingStyle",SOAP_1_2_ENV_NAMESPACE)) {
				if (soap_version == SOAP_1_2) {
					add_soap_fault(this_ptr, "Client", "encodingStyle cannot be specified on the Header", NULL, NULL TSRMLS_CC);
					xmlFreeDoc(response);
					return FALSE;
				} else if (strcmp(attr->children->content,SOAP_1_1_ENC_NAMESPACE) != 0) {
					add_soap_fault(this_ptr, "Client", "Unknown data encoding style", NULL, NULL TSRMLS_CC);
					xmlFreeDoc(response);
					return FALSE;
				}
			}
			attr = attr->next;
		}
	}

	if (soap_headers && head) {
		trav = head->children;
		while (trav != NULL) {
			if (trav->type == XML_ELEMENT_NODE) {
				zval *val = master_to_zval(NULL, trav);
				add_assoc_zval(soap_headers, (char*)trav->name, val);
			}
			trav = trav->next;
		}
	}

	/* Check if <Body> contains <Fault> element */
	fault = get_node_ex(body->children,"Fault",envelope_ns);
	if (fault != NULL) {
		char *faultcode = NULL, *faultstring = NULL, *faultactor = NULL;
		zval *details = NULL;
		xmlNodePtr tmp;

		if (soap_version == SOAP_1_1) {
			tmp = get_node(fault->children,"faultcode");
			if (tmp != NULL && tmp->children != NULL) {
				faultcode = tmp->children->content;
			}

			tmp = get_node(fault->children,"faultstring");
			if (tmp != NULL && tmp->children != NULL) {
				faultstring = tmp->children->content;
			}

			tmp = get_node(fault->children,"faultactor");
			if (tmp != NULL && tmp->children != NULL) {
				faultactor = tmp->children->content;
			}

			tmp = get_node(fault->children,"detail");
			if (tmp != NULL) {
				details = master_to_zval(NULL, tmp);
			}
		} else {
			tmp = get_node(fault->children,"Code");
			if (tmp != NULL && tmp->children != NULL) {
				tmp = get_node(tmp->children,"Value");
				if (tmp != NULL && tmp->children != NULL) {
					faultcode = tmp->children->content;
				}
			}

			tmp = get_node(fault->children,"Reason");
			if (tmp != NULL && tmp->children != NULL) {
				/* TODO: lang attribute */
				tmp = get_node(tmp->children,"Text");
				if (tmp != NULL && tmp->children != NULL) {
					faultstring = tmp->children->content;
				}
			}

			tmp = get_node(fault->children,"Detail");
			if (tmp != NULL) {
				details = master_to_zval(NULL, tmp);
			}
		}
		add_soap_fault(this_ptr, faultcode, faultstring, faultactor, details TSRMLS_CC);
		xmlFreeDoc(response);
		return FALSE;
	}

	/* Parse content of <Body> element */
	array_init(return_value);
	resp = body->children;
	while (resp != NULL && resp->type != XML_ELEMENT_NODE) {
		resp = resp->next;
	}
	if (resp != NULL) {
		if (fn != NULL && fn->binding && fn->binding->bindingType == BINDING_SOAP) {
		  /* Function has WSDL description */
			sdlParamPtr *h_param, param = NULL;
			xmlNodePtr val = NULL;
			char *name, *ns = NULL;
			zval* tmp;
			sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)fn->bindingAttributes;
			int res_count = zend_hash_num_elements(fn->responseParameters);

			zend_hash_internal_pointer_reset(fn->responseParameters);
			while (zend_hash_get_current_data(fn->responseParameters, (void **)&h_param) == SUCCESS) {
				param = (*h_param);
				if (fnb->style == SOAP_DOCUMENT) {
					name = param->encode->details.type_str;
					ns = param->encode->details.ns;
				} else {
					name = fn->responseName;
					/* ns = ? */
				}

				/* Get value of parameter */
				cur = get_node_ex(resp, name, ns);
				if (!cur) {
					cur = get_node(resp, name);
					/* TODO: produce warning invalid ns */
				}
				if (cur) {
					if (fnb->style == SOAP_DOCUMENT) {
						val = cur;
					} else {
						val = get_node(cur->children, param->paramName);
						if (val == NULL && res_count == 1) {
							val = get_node(cur->children, "return");
						}
						if (val == NULL && res_count == 1) {
							val = get_node(cur->children, "result");
						}
					}
				}

				if (!val) {
					/* TODO: may be "nil" is not OK? */
					MAKE_STD_ZVAL(tmp);
					ZVAL_NULL(tmp);
/*
					add_soap_fault(this_ptr, "Client", "Can't find response data", NULL, NULL TSRMLS_CC);
					xmlFreeDoc(response);
					return FALSE;
*/
				} else {
					/* Decoding value of parameter */
					if (param != NULL) {
						tmp = master_to_zval(param->encode, val);
					} else {
						tmp = master_to_zval(NULL, val);
					}
				}
				add_assoc_zval(return_value, param->paramName, tmp);

				param_count++;

				zend_hash_move_forward(fn->responseParameters);
			}
		} else {
		  /* Function hasn't WSDL description */
			xmlNodePtr val;
			val = resp->children;
			while (val != NULL) {
				while (val && val->type != XML_ELEMENT_NODE) {
					val = val->next;
				}
				if (val != NULL) {
					if (!node_is_equal_ex(val,"result",RPC_SOAP12_NAMESPACE)) {
						zval *tmp;

						tmp = master_to_zval(NULL, val);
						if (val->name) {
							add_assoc_zval(return_value, (char*)val->name, tmp);
						} else {
							add_next_index_zval(return_value, tmp);
						}
						++param_count;
					}
					val = val->next;
				}
			}
		}
	}

	if (Z_TYPE_P(return_value) == IS_ARRAY) {
		if (param_count == 0) {
			zval_dtor(return_value);
			ZVAL_NULL(return_value);
		} else if (param_count == 1) {
			zval *tmp = *(zval**)Z_ARRVAL_P(return_value)->pListHead->pData;
			tmp->refcount++;
			zval_dtor(return_value);
			*return_value = *tmp;
			FREE_ZVAL(tmp);
		}
	}

	xmlFreeDoc(response);
	return TRUE;
}
