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

require_once ('common.php');
require_once ('server.php');
require_once ('config_node.php');
require_once ('page_index.php');
require_once ('page_debug.php');
require_once ('page_vserver.php');


function &read_configuration () {
	$conf =& new ConfigNode();
	
	$ret = $conf->Load (cherokee_default_config_file);
	if ($ret != ret_ok) {
		PRINT_ERROR ("Couldn't read $default_config");
	}

	return $conf;
}


function &instance_page (&$theme, &$conf) {
	$name   = strtolower($_REQUEST['page']);
	$params = $_REQUEST;

	unset ($params['page']);
	$ajax = $params['ajax'];

	switch ($name) {
	case 'restart':
		$server =& new Server ($conf);
		$conf->Save (cherokee_default_config_file);
		$server->SendHUP();
		break;

	case 'debug':
		$page =& new PageDebug(&$theme, $conf, $params);
		break;

	case 'vserver':
		if (!empty($ajax))
			$page =& new PageVserverAjax ($conf, $params);
		else
			$page =& new PageVServer(&$theme, $conf, $params);
		break;

	default:
		if (!empty($ajax))
			$page =& new PageIndexAjax ($conf, $params);
		else
			$page =& new PageIndex ($theme, $conf, $params);
		break;
	}

	return $page;
}


function main() 
{
	session_start();

	if ($_SESSION["config"] == null) {
		$conf =& read_configuration ();
		$_SESSION["config"] =& $conf;
	}

	$theme =  new Theme();
	$conf  =& $_SESSION["config"];

	$page =& instance_page ($theme, $conf);
	echo $page->Render();

	session_write_close();
}

main();
?>