/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *      Stefan de Konink <stefan@konink.de>
 *
 * Copyright (C) 2001-2011 Alvaro Lopez Ortega
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

/* Make sure common-internal.h is always the first include!
 * Otherwise you will get bad things on 32bit platforms.
 */
#include "common-internal.h"

/* The header of our handler */
#include "handler_xslt.h"


/* The general Cherokee include */

#include "util.h"
#include "connection.h"
#include "connection-protected.h"
#include "plugin_loader.h"

#include <libxml/xmlmemory.h>
#include <libxml/xmlIO.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/uri.h>

#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxslt/extensions.h>
#include <libxslt/security.h>
#include <libexslt/exsltconfig.h>
#include <libexslt/exslt.h>

#ifdef LIBXML_XINCLUDE_ENABLED
#include <libxml/xinclude.h>
#endif
#ifdef LIBXML_CATALOG_ENABLED
#include <libxml/catalog.h>
#endif


/* Plug-in initialization
 *
 * In this function you can use any of these:
 * http_delete | http_get | http_post | http_put
 *
 * For a full list: cherokee_http_method_t
 *
 * It is what your handler to be implements.
 *
 */
PLUGIN_INFO_HANDLER_EASIEST_INIT (xslt, http_get);

/* Methods implementation
 */

/* Props_free is registered to free our properties
 * after the handler is removed by the webserver.
 * If you put any variables in your props that need
 * to be freed, here is the place. Be smart and check
 * for NULL :)
 */
static ret_t 
props_free (cherokee_handler_xslt_props_t *props)
{
	if (props->global)
		xsltFreeStylesheet(props->global);

	if (props->globalDoc)
		xmlFreeDoc(props->globalDoc);

//	xsltFreeSecurityPrefs(props->sec);
	xsltCleanupGlobals();
	xmlCleanupParser();

	cherokee_buffer_mrproper(&props->content_type);
	cherokee_buffer_mrproper(&props->stylesheet);
	return cherokee_module_props_free_base (MODULE_PROPS(props));
}


/* A _configure function will be executed by the webserver
 * at the moment of 'first' initialisation. Here you do the
 * start of memory management. The processing of the configuration
 * parameters.
 */
ret_t 
cherokee_handler_xslt_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	cherokee_list_t               *i;
	cherokee_handler_xslt_props_t *props;
	ret_t                          ret;

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_xslt_props);

		cherokee_module_props_init_base (MODULE_PROPS(n), 
						 MODULE_PROPS_FREE(props_free));		
        
		/* Look at handler_xslt.h
		 * This is an xslt of configuration.
		 */
		xmlInitMemory();
		
		LIBXML_TEST_VERSION

		/*
		 * Register the EXSLT extensions and the test module
		 */
		exsltRegisterAll();
		xsltRegisterTestModule();

		cherokee_buffer_init (&n->content_type);
		cherokee_buffer_init (&n->stylesheet);
		n->global = NULL;
		n->globalDoc = NULL;
		n->xinclude = false;
		n->xincludestyle = false;
//		n->sec = xsltNewSecurityPrefs();
//		xsltSetDefaultSecurityPrefs(n->sec);
		n->defaultEntityLoader = xmlGetExternalEntityLoader();
//		xmlSetExternalEntityLoader(xsltprocExternalEntityLoader);

		*_props = MODULE_PROPS(n);
	}

	props = PROP_XSLT(*_props);

	/* processing of the parameters, we only know about xslt_config
	 * it might be a good thing to abort if we found a wrong key, understand
	 * the consequences: your handler will prevent the server startup upon
	 * a wrong configuration.
         *
         * You see the for each method is rather creative. Read more code
         * to see with what nice things Cherokee extended the C syntax.
	 */
	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		/* Cherokee has a nifty string equal function that allows
                 * lazy programmers to do strcmp without the == 0.
                 */
		/*if (equal_buf_str (&subconf->key, "maxdepth")) {
			cherokee_atoi(subconf->val.buf, &props->xsltMaxDepth);
		} else if (equal_buf_str (&subconf->key, "maxparserdepth")) {
			cherokee_atoi(subconf->val.buf, &props->xsltParserMaxDepth);
		} else */
		if (equal_buf_str (&subconf->key, "options")) {
			if (equal_buf_str (&subconf->val, "novalid")) {
				props->options |= XML_PARSE_NOENT | XML_PARSE_NOCDATA;
			} else if (equal_buf_str (&subconf->val, "nodtdattr")) {
				props->options |= XML_PARSE_NOENT | XML_PARSE_DTDLOAD | XML_PARSE_NOCDATA;
			} else {
				PRINT_MSG ("ERROR: Handler xslt: Unknown value: '%s'\n", subconf->val.buf);
				return ret_error;
			}
		} else if (equal_buf_str (&subconf->key, "nodict")) {
			cherokee_boolean_t nodict;
		        ret = cherokee_atob (subconf->val.buf, &nodict);
			if (ret != ret_ok) return ret;
			if (nodict) {
				props->options |= XML_PARSE_NODICT;
			}
#ifdef LIBXML_CATALOG_ENABLED
		} else if (equal_buf_str (&subconf->key, "catalogs")) {
			cherokee_boolean_t catalogs;
		        ret = cherokee_atob (subconf->val.buf, &catalogs);
			if (ret != ret_ok) return ret;
			if (catalogs) {
				const char *catalogs = getenv("SGML_CATALOG_FILES");
				if (catalogs != NULL) {
					xmlLoadCatalogs(catalogs);
				}
			}
#endif
#ifdef LIBXML_XINCLUDE_ENABLED
		} else if (equal_buf_str (&subconf->key, "xinclude")) {
		        ret = cherokee_atob (subconf->val.buf, &props->xinclude);
		} else if (equal_buf_str (&subconf->key, "xincludestyle")) {
		        ret = cherokee_atob (subconf->val.buf, &props->xincludestyle);
			if (props->xincludestyle) {
				xsltSetXIncludeDefault(1);
			}
#endif
		} else if (equal_buf_str (&subconf->key, "contenttype")) {
			cherokee_buffer_add(&props->content_type, subconf->val.buf, subconf->val.len);
		} else if (equal_buf_str (&subconf->key, "stylesheet")) {
			cherokee_buffer_add(&props->stylesheet, subconf->val.buf, subconf->val.len);
		} else {
			PRINT_MSG ("ERROR: Handler xslt: Unknown key: '%s'\n", subconf->key.buf);
			return ret_error;
		}
	}

	if (props->stylesheet.len == 0) {
		props->globalDoc = xmlReadFile((const char *) props->stylesheet.buf, NULL, props->options);
		if (props->globalDoc == NULL) {
			PRINT_MSG ("ERROR: Handler xslt: Can't parse stylesheet: '%s'\n", props->stylesheet.buf);
			return ret_error;
		}
		props->global = xsltParseStylesheetDoc(props->globalDoc);
	}
	
	if (props->content_type.len == 0) {
		cherokee_buffer_add_str (&props->content_type, "text/html");
	}

	return ret_ok;
}

/* The new method is called when something connects to the handler.
 * It is basically the place where the webserver is going to meet your
 * functions, and you will take care that the webserver remembers where
 * you implemented your methods.
 * You can see that there is an abstract class, MODULE(n) and HANDLER(n) these
 * allow you to 'implement' a specific function within your own code. Cherokee
 * will just know where to find a pointer to it the init, free, step, add_headers
 * in this xslt.
 */
ret_t
cherokee_handler_xslt_new  (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_module_props_t *props)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, handler_xslt);
	
	/* Init the base class object
	 */
	cherokee_handler_init_base(HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(xslt));
	   
	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_xslt_init;
	MODULE(n)->free         = (module_func_free_t) cherokee_handler_xslt_free;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_xslt_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_xslt_add_headers;

	HANDLER(n)->support = hsupport_length;

	/* Initialisation of our output buffer
	 */
	ret = cherokee_buffer_init (&n->buffer);
	if (unlikely(ret != ret_ok)) 
		return ret;

	/* We make sure we can fit at least 4k in this buffer
         * overkill, yes we know.
         */
	ret = cherokee_buffer_ensure_size (&n->buffer, 4*1024);
	if (unlikely(ret != ret_ok)) 
		return ret;

	*hdl = HANDLER(n);
	return ret_ok;
}


/* When your handler is thanked for his efforts after a request
 * we must clean up the mess we have made. In our case you might
 * remember the output buffer of 4k. We need to get rid of it
 */
ret_t 
cherokee_handler_xslt_free (cherokee_handler_xslt_t *hdl)
{
//	if (hdl->cur != NULL)
//		xsltFreeStylesheet(hdl->cur);

	/* remember it by: make-REAL-proper */
	cherokee_buffer_mrproper (&hdl->buffer);
	return ret_ok;
}

static ret_t
xsltProcess(cherokee_handler_xslt_t *hdl, xmlDocPtr doc, xsltStylesheetPtr cur) {
        int ret;
	xsltTransformContextPtr ctxt;
	xmlOutputBufferPtr IObuf = xmlAllocOutputBuffer(NULL);

#ifdef LIBXML_XINCLUDE_ENABLED
	if (HDL_XSLT_PROPS(hdl)->xinclude) {
#if LIBXML_VERSION >= 20603
	        xmlXIncludeProcessFlags(doc, XSLT_PARSE_OPTIONS);
#else
	        xmlXIncludeProcess(doc);
#endif
	}
#endif

        ctxt = xsltNewTransformContext(cur, doc);
        if (ctxt == NULL)
            return ret_error;
        ret = xsltRunStylesheetUser(cur, doc, NULL, NULL, NULL, IObuf, NULL, ctxt);
        
	xsltFreeTransformContext(ctxt);
        xmlFreeDoc(doc);

	cherokee_buffer_add (&hdl->buffer, IObuf->buffer->content, IObuf->buffer->use);
	xmlOutputBufferClose(IObuf);

        if (ret == -1) // || ctxt->state == XSLT_STATE_ERROR || ctxt->state == XSLT_STATE_STOPPED)
		return ret_error;
}

ret_t 
cherokee_handler_xslt_init (cherokee_handler_xslt_t *hdl)
{
	cherokee_connection_t *conn = HANDLER_CONN(hdl);
	cherokee_buffer_t buffer = CHEROKEE_BUF_INIT;
	xmlDocPtr doc;

	/* Build the local file path
	 */
	cherokee_buffer_add_buffer (&buffer, &CONN_VSRV(conn)->root);
	cherokee_buffer_add_buffer (&buffer, &conn->request);

	doc = xmlReadFile((const char *) buffer.buf, NULL, HDL_XSLT_PROPS(hdl)->options);
	cherokee_buffer_mrproper (&buffer);

	if (doc != NULL) {
		if (HDL_XSLT_PROPS(hdl)->xincludestyle) {
#if LIBXML_VERSION >= 20603
	                    xmlXIncludeProcessFlags(doc, XSLT_PARSE_OPTIONS);
#else
	                    xmlXIncludeProcess(doc);
#endif		
		}

		if (HDL_XSLT_PROPS(hdl)->global) {
			xsltProcess(hdl, doc, HDL_XSLT_PROPS(hdl)->global);
			return ret_ok;
		} else {
			xsltStylesheetPtr cur = xsltLoadStylesheetPI(doc);
			if (cur != NULL) {
				/* it is an embedded stylesheet */
				xsltProcess(hdl, doc, cur);
				xsltFreeStylesheet(cur);
				return ret_ok;
			}
		}
		xmlFreeDoc(doc);
	} else {
		/* no stylesheet, no nothing
		 * just send it out as an 'normal' file
		 * todo...
		 */
	}

	return ret_error;
}

/* The _step function is called by the webserver when your handler
 * should provide content. Important to realise is that this method
 * can be called multiple times, and not all data has to be present
 * when this function is first called.
 * You could argue that this code is overkill for the current purpose
 * and it is :)
 */
ret_t 
cherokee_handler_xslt_step (cherokee_handler_xslt_t *hdl, cherokee_buffer_t *buffer)
{
        cuint_t tosend;

	/* Check if we have actually something to send,
	 * if there is nothing, we have reached the end of
         * the contents and thus should mention an end-of-file.
	 *
         * We advise you to always use *double* buffering in
         * the build_page function for things that are known to
         * grow and take more time. If directly is written
         * into the buffer, the handler might kill the request
         * while still data is comming in.
         */
        if (cherokee_buffer_is_empty (&hdl->buffer))
                return ret_eof;
	
	/* We will send up to 1k chunks each time the _step function is called */
        tosend = (hdl->buffer.len > 1024 ? 1024 : hdl->buffer.len);

	/* To the send buffer, we copy 'tosend' amount of bytes */
        cherokee_buffer_add (buffer, hdl->buffer.buf, tosend);

	/* We don't want to send this the next time, therefore we
         * remove what we have send from the output buffer.
         */
        cherokee_buffer_move_to_begin (&hdl->buffer, tosend);

	/* A minor optimalisation is to check if the buffer were
         * processing is now empty, if it is, we can inform the
         * webserver that the data is all outputed to the send buffer
         */
        if (cherokee_buffer_is_empty (&hdl->buffer))
                return ret_eof_have_data;

	/* Otherwise we will inform that we are not done _stepping.
         * As you can see, if we didn't implement the previous
         * ret_eof_have_data, the next time the _step method is called
         * it would ask the same question. Hence we save one time of
         * stepping. (Remember Cherokee is a High Performance webserver
         * your handler should be HP too ;)
         */
        return ret_ok;
}

/* The _add_headers function is typically a moment in your code that
 * everything has been done. You know something about your buffers,
 * and want to make the webserver happy.
 */
ret_t 
cherokee_handler_xslt_add_headers (cherokee_handler_xslt_t *hdl, cherokee_buffer_t *buffer)
{
	/* This makes sending and receiving faster, because we provide the length
         * thus the client knows exactly what to expect.
         */
	cherokee_buffer_add_va (buffer, "Content-Length: %d"CRLF, hdl->buffer.len);
	cherokee_buffer_add_va (buffer, "Content-Type: %s"CRLF, HDL_XSLT_PROPS(hdl)->content_type);

	return ret_ok;
}
