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

#include "common-internal.h"
#include "collector.h"
#include "bogotime.h"


/* Priv structure
 */

typedef struct {
	CHEROKEE_MUTEX_T (mutex);
	int               foo;
} priv_t;


static ret_t
priv_new (priv_t **priv)
{
	priv_t *n;

	n = malloc (sizeof (priv_t));
	if (n == NULL) {
		return ret_nomem;
	}

	CHEROKEE_MUTEX_INIT (&n->mutex, CHEROKEE_MUTEX_FAST);

	*priv = n;
	return ret_ok;
}

static void
priv_free (priv_t *priv)
{
	if (priv == NULL)
		return;

	CHEROKEE_MUTEX_DESTROY (&priv->mutex);
	free (priv);
}

#define LOCK(c)   CHEROKEE_MUTEX_LOCK   (&((priv_t *)(COLLECTOR_BASE(c)->priv))->mutex)
#define UNLOCK(c) CHEROKEE_MUTEX_UNLOCK (&((priv_t *)(COLLECTOR_BASE(c)->priv))->mutex)


/* Collection base
 */

static ret_t
base_init (cherokee_collector_base_t *collector,
           cherokee_plugin_info_t    *info,
           cherokee_config_node_t    *config)
{
	ret_t ret;

	/* Init the base class
	 */
	cherokee_module_init_base (MODULE(collector), NULL, info);

	/* Pure virtual methods
	 */
	collector->free       = NULL;

	/* Properties
	 */
	collector->rx         = 0;
	collector->rx_partial = 0;
	collector->tx         = 0;
	collector->tx_partial = 0;

	/* Private
	 */
	ret = priv_new ((priv_t **) &collector->priv);
	if (ret != ret_ok) {
		return ret;
	}

	/* Read configuration
	 */
	UNUSED (config);

	return ret_ok;
}

static ret_t
base_free (cherokee_collector_base_t *collector)
{
	priv_free (collector->priv);
	free (collector);
	return ret_ok;
}

static ret_t
base_count (cherokee_collector_base_t *collector,
            off_t                      rx,
            off_t                      tx)
{
	collector->rx         += rx;
	collector->rx_partial += rx;
	collector->tx         += tx;
	collector->tx_partial += tx;

	return ret_ok;
}


/* Collection Server
 */

ret_t
cherokee_collector_init_base (cherokee_collector_t  *collector,
                              cherokee_plugin_info_t *info,
                              cherokee_config_node_t *config)
{
	/* Init the base class
	 */
	base_init (COLLECTOR_BASE(collector), info, config);

	/* Pure virtual methods
	 */
	collector->new_vsrv = NULL;

	/* Properties
	 */
	collector->accepts          = 0;
	collector->accepts_partial  = 0;

	collector->requests         = 0;
	collector->requests_partial = 0;

	collector->timeouts         = 0;
	collector->timeouts_partial = 0;

	return ret_ok;
}

ret_t
cherokee_collector_free (cherokee_collector_t *collector)
{
	if (COLLECTOR_BASE(collector)->free) {
		COLLECTOR_BASE(collector)->free (COLLECTOR_BASE(collector));
	}

	return base_free (COLLECTOR_BASE(collector));
}

ret_t
cherokee_collector_init (cherokee_collector_t *collector)
{
	if (collector->init == NULL) {
		return ret_error;
	}

	return collector->init (collector);
}


ret_t
cherokee_collector_log_accept (cherokee_collector_t *collector)
{
	LOCK(collector);

	collector->accepts++;
	collector->accepts_partial++;

	UNLOCK(collector);
	return ret_ok;
}

ret_t
cherokee_collector_log_request (cherokee_collector_t *collector)
{
	LOCK(collector);

	collector->requests++;
	collector->requests_partial++;

	UNLOCK(collector);
	return ret_ok;
}

ret_t
cherokee_collector_log_timeout (cherokee_collector_t *collector)
{
	LOCK(collector);

	collector->timeouts++;
	collector->timeouts_partial++;

	UNLOCK(collector);
	return ret_ok;
}


/* Collection Virtual Server
 */

ret_t
cherokee_collector_vsrv_new (cherokee_collector_t       *collector,
                             cherokee_config_node_t     *config,
                             cherokee_collector_vsrv_t **collector_vsrv)
{
	ret_t ret;

	if (unlikely (collector->new_vsrv == NULL)) {
		return ret_error;
	}

	/* Instance
	 */
	ret = collector->new_vsrv (collector, config, (void **)collector_vsrv);
	if (ret != ret_ok) {
		return ret_error;
	}

	/* Store a ref to the server collector
	 */
	(*collector_vsrv)->srv_collector = collector;
	return ret_ok;
}

ret_t
cherokee_collector_vsrv_init_base (cherokee_collector_vsrv_t  *collector_vsrv,
                                   cherokee_config_node_t     *config)
{
	/* Init the base class
	 */
	base_init (COLLECTOR_BASE(collector_vsrv), NULL, config);

	/* Properties
	 */
	collector_vsrv->srv_next_update = 0;
	collector_vsrv->srv_rx_partial  = 0;
	collector_vsrv->srv_tx_partial  = 0;

	return ret_ok;
}

ret_t
cherokee_collector_vsrv_free (cherokee_collector_vsrv_t *collector_vsrv)
{
	if (COLLECTOR_BASE(collector_vsrv)->free) {
		COLLECTOR_BASE(collector_vsrv)->free (COLLECTOR_BASE(collector_vsrv));
	}

	return base_free (COLLECTOR_BASE(collector_vsrv));
}

ret_t
cherokee_collector_vsrv_count (cherokee_collector_vsrv_t  *collector_vsrv,
                               off_t                       rx,
                               off_t                       tx)
{
	/* Add it to the virtual server collector
	 */
	LOCK(collector_vsrv);
	base_count (COLLECTOR_BASE(collector_vsrv), rx, tx);

	/* Partial counting (vsrv to srv)
	 */
	collector_vsrv->srv_rx_partial += rx;
	collector_vsrv->srv_tx_partial += tx;

	if (cherokee_bogonow_now >= collector_vsrv->srv_next_update) {
		LOCK(collector_vsrv->srv_collector);

		base_count (COLLECTOR_BASE(collector_vsrv->srv_collector),
		            collector_vsrv->srv_rx_partial,
		            collector_vsrv->srv_tx_partial);

		collector_vsrv->srv_rx_partial  = 0;
		collector_vsrv->srv_tx_partial  = 0;
		collector_vsrv->srv_next_update = cherokee_bogonow_now + 10;

		UNLOCK(collector_vsrv->srv_collector);
	}

	UNLOCK(collector_vsrv);
	return ret_ok;
}


ret_t
cherokee_collector_vsrv_init (cherokee_collector_vsrv_t *collector,
                              void                      *vsrv)
{
	if (collector->init == NULL) {
		return ret_error;
	}

	return collector->init (collector, vsrv);
}
