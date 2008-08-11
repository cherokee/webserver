/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * This piece of code by:
 *      Miguel Angel Ajo Pelayo <ajo@godsmaze.org>
 *
 * Copyright (C) 2001-2008 Alvaro Lopez Ortega
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

#include "common-internal.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


struct cherokee_logger_private {
	CHEROKEE_MUTEX_T   (mutex);
	cherokee_boolean_t  backup_mode;
};

#define PRIV(x)  (LOGGER(x)->priv)

ret_t
cherokee_logger_init_base (cherokee_logger_t *logger, cherokee_plugin_info_t *info)
{
	CHEROKEE_NEW_TYPE(priv, struct cherokee_logger_private);

	/* Init the base class
	 */
	cherokee_module_init_base (MODULE(logger), NULL, info);

	/* Pure virtual methods
	 */
	logger->priv         = priv;
	logger->write_access = NULL;
	logger->write_error  = NULL;

	/* Private
	 */	
	logger->priv->backup_mode = false;
	CHEROKEE_MUTEX_INIT (&PRIV(logger)->mutex, NULL);

	return ret_ok;
}



/* Virtual method hiding layer
 */
ret_t
cherokee_logger_free (cherokee_logger_t *logger)
{
	ret_t ret;

	CHEROKEE_MUTEX_DESTROY (&PRIV(logger)->mutex);

	if (MODULE(logger)->free == NULL) {
		ret = ret_error;
	}

	ret = MODULE(logger)->free (logger);
	free (logger);

	return ret_ok;
}


ret_t
cherokee_logger_flush (cherokee_logger_t *logger)
{
	ret_t ret = ret_error;

	/* If the logger is on backup mode, it shouldn't write anything
	 * to the disk.  Maintenance tasks have been taking place.
	 */
	if (logger->priv->backup_mode) {
		return ret_ok;
	}

	if (logger->flush) {
		CHEROKEE_MUTEX_LOCK (&PRIV(logger)->mutex);
		ret = logger->flush(logger);
		CHEROKEE_MUTEX_UNLOCK (&PRIV(logger)->mutex);
	}

	return ret;
}


ret_t
cherokee_logger_init (cherokee_logger_t *logger)
{
	logger_func_init_t init_func;

	init_func = (logger_func_init_t) MODULE(logger)->init;

	if (init_func) {
		return init_func (logger);
	}

	return ret_error;
}


ret_t
cherokee_logger_write_access (cherokee_logger_t *logger, void *conn)
{
	ret_t ret = ret_error;

	if (logger->write_access) {
		CHEROKEE_MUTEX_LOCK (&PRIV(logger)->mutex);
		ret = logger->write_access (logger, conn);
		CHEROKEE_MUTEX_UNLOCK (&PRIV(logger)->mutex);
	}

	return ret;
}


ret_t
cherokee_logger_write_error (cherokee_logger_t *logger, void *conn)
{
	ret_t ret = ret_error;

	if (logger->write_error) {
		CHEROKEE_MUTEX_LOCK (&PRIV(logger)->mutex);
		ret = logger->write_error (logger, conn);
		CHEROKEE_MUTEX_UNLOCK (&PRIV(logger)->mutex);
	}

	return ret;
}


ret_t 
cherokee_logger_write_string (cherokee_logger_t *logger, const char *format, ...)
{
	va_list ap;

	if (logger == NULL) 
		return ret_ok;

	if (logger->write_string) {
		ret_t             ret;
		cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

		CHEROKEE_MUTEX_LOCK(&PRIV(logger)->mutex);
		va_start (ap, format);
		cherokee_buffer_add_va_list (&tmp, (char *)format, ap);
		va_end (ap);
		CHEROKEE_MUTEX_UNLOCK(&PRIV(logger)->mutex);

		ret = logger->write_string (logger, tmp.buf);

		cherokee_buffer_mrproper (&tmp);
		return ret;
	}

	return ret_error;
}


ret_t 
cherokee_logger_reopen (cherokee_logger_t *logger)
{
	ret_t ret = ret_error;

	if (logger->reopen != NULL) {
		CHEROKEE_MUTEX_LOCK (&PRIV(logger)->mutex);
		ret = logger->reopen (logger);
		CHEROKEE_MUTEX_UNLOCK (&PRIV(logger)->mutex);
	}

	return ret;
}


ret_t 
cherokee_logger_set_backup_mode (cherokee_logger_t *logger, cherokee_boolean_t active)
{
	ret_t ret;

	/* Set backup mode: ON
	 */
	if (active == true) {
		logger->priv->backup_mode = true;
		return ret_ok;
	}

	/* Set backup mode: OFF
	 */
	logger->priv->backup_mode = false;

	ret = cherokee_logger_reopen (logger);
	if (unlikely(ret != ret_ok)) return ret;

	ret = cherokee_logger_flush (logger);
	if (unlikely(ret != ret_ok)) return ret;

	return ret_ok;
}


ret_t 
cherokee_logger_get_backup_mode (cherokee_logger_t *logger, cherokee_boolean_t *active)
{
	*active = logger->priv->backup_mode;
	return ret_ok;
}
