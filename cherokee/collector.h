/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2014 Alvaro Lopez Ortega
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_COLLECTOR_H
#define CHEROKEE_COLLECTOR_H

#include <cherokee/common.h>
#include <cherokee/buffer.h>
#include <cherokee/module.h>
#include <cherokee/connection.h>

CHEROKEE_BEGIN_DECLS

/* Virtual method prototypes
 */
typedef ret_t (* collector_func_new_t)      (void                   **collector,
                                             cherokee_plugin_info_t  *info,
                                             cherokee_config_node_t  *config);
typedef ret_t (* collector_func_free_t)     (void                    *collector);
typedef ret_t (* collector_func_init_t)     (void                    *collector);

typedef ret_t (* collector_func_new_vsrv_t) (void                    *collector,
                                             cherokee_config_node_t  *config,
                                             void                   **collector_vsrv);

typedef ret_t (* collector_vsrv_func_init_t) (void                    *collector_vsrv,
                                              void                    *vsrv);


typedef struct {
	cherokee_module_t         module;
	void                     *priv;

	/* Virtual Methods
	 */
	collector_func_free_t     free;

	/* Properties
	 */
	off_t                     rx;
	off_t                     rx_partial;
	off_t                     tx;
	off_t                     tx_partial;
} cherokee_collector_base_t;

typedef struct {
	cherokee_collector_base_t collector;

	/* Virtual Methods
	 */
	collector_func_new_vsrv_t new_vsrv;
	collector_func_init_t     init;

	/* Properties
	 */
	cullong_t                 accepts;
	cullong_t                 accepts_partial;
	cullong_t                 requests;
	cullong_t                 requests_partial;
	cullong_t                 timeouts;
	cullong_t                 timeouts_partial;
} cherokee_collector_t;

typedef struct {
	cherokee_collector_base_t  collector;
	cherokee_collector_t      *srv_collector;

	/* Virtual Methods
	 */
	collector_vsrv_func_init_t init;

	/* Properties
	 */
	time_t                     srv_next_update;
	off_t                      srv_rx_partial;
	off_t                      srv_tx_partial;
} cherokee_collector_vsrv_t;

#define COLLECTOR_BASE(c)       ((cherokee_collector_base_t *)(c))
#define COLLECTOR(c)            ((cherokee_collector_t *)(c))
#define COLLECTOR_VSRV(c)       ((cherokee_collector_vsrv_t *)(c))

#define COLLECTOR_RX(c)         (COLLECTOR_BASE(c)->rx)
#define COLLECTOR_RX_PARTIAL(c) (COLLECTOR_BASE(c)->rx_partial)
#define COLLECTOR_TX(c)         (COLLECTOR_BASE(c)->tx)
#define COLLECTOR_TX_PARTIAL(c) (COLLECTOR_BASE(c)->tx_partial)


/* Easy initialization
 */
#define PLUGIN_INFO_COLLECTOR_EASY_INIT(name)                          \
	PLUGIN_INFO_INIT(name, cherokee_collector,                     \
	                 (void *)cherokee_collector_ ## name ## _new,  \
	                 (void *)NULL)

#define PLUGIN_INFO_COLLECTOR_EASIEST_INIT(name) \
	PLUGIN_EMPTY_INIT_FUNCTION(name)         \
	PLUGIN_INFO_COLLECTOR_EASY_INIT(name)

/* Internal
 */
ret_t cherokee_collector_init_base      (cherokee_collector_t      *collector,
                                         cherokee_plugin_info_t    *info,
                                         cherokee_config_node_t    *config);

ret_t cherokee_collector_vsrv_init_base (cherokee_collector_vsrv_t  *collector_vsrv,
                                         cherokee_config_node_t     *config);


/* Collector virtual methods
 */
ret_t cherokee_collector_init        (cherokee_collector_t      *collector);
ret_t cherokee_collector_free        (cherokee_collector_t      *collector);

ret_t cherokee_collector_log_accept  (cherokee_collector_t      *collector);
ret_t cherokee_collector_log_request (cherokee_collector_t      *collector);
ret_t cherokee_collector_log_timeout (cherokee_collector_t      *collector);

/* Collector virtual methods
 */
ret_t cherokee_collector_vsrv_new    (cherokee_collector_t       *collector,
                                      cherokee_config_node_t     *config,
                                      cherokee_collector_vsrv_t **collector_vsrv);
ret_t cherokee_collector_vsrv_free   (cherokee_collector_vsrv_t  *collector_vsrv);
ret_t cherokee_collector_vsrv_init   (cherokee_collector_vsrv_t  *collector_vsrv,
                                      void                       *vsrv);

ret_t cherokee_collector_vsrv_count  (cherokee_collector_vsrv_t  *collector_vsrv,
                                      off_t                       rx,
                                      off_t                       tx);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_COLLECTOR_H */
