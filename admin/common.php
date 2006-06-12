<?php /* -*- Mode: PHP; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2006 Alvaro Lopez Ortega
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

/* Types
 */
define ("ret_ok",     0);
define ("ret_error", -1);

/* Constants
 */
define ("CRLF", "\r\n");

/* Configuration
 */
define ('cherokee_default_config_file', '/etc/cherokee/cherokee.conf');
define ('cherokee_default_theme_path',  'theme/');

define ('YUI_use_min', 0);
define ('YUI_www_path', '/build');

/* Functions
 */
function PRINT_ERROR ($string) 
{
	echo "<b>ERROR</b>: $string<br />\n";
}

/* Utilities
 */
function is_windows ()
{
	return ((PHP_OS == 'WINNT') or 
		(PHP_OS == 'WIN32') or 
		(PHP_OS == 'Windows'));
}

?>