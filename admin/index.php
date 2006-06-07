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
require_once ('config_node.php');

function headers() 
{
	$scripts  = CRLF;
	$scripts .= '<script type="text/javascript" src="build/yahoo/yahoo.js"></script>'.CRLF;

	echo '<!doctype html public "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">'.CRLF;
	echo '<html>'.CRLF;
	echo "<head>$scripts</head>".CRLF;
}

function body()
{
	echo "<body>".CRLF;
}

function foot()
{
	echo "</body>".CRLF;
	echo "</html>";
}

function main() 
{
	session_start();
	headers();
	body();

//	echo "The session is: " . $_SESSION["config"] . "<br />";

	if ($_SESSION["config"] == null) 
	{
		$conf = new ConfigNode();

		$ret = $conf->Load (cherokee_default_config_file);
		if ($ret != ret_ok) {
			PRINT_ERROR ("Couldn't read $default_config");
		}

		$_SESSION["config"] = $conf;
	}

	foot();
	session_write_close();
}

main();
?>
