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

class Server {
	var $config;
	var $pid;

	function Server ($config) {
		$this->config = &$config;
		$this->pid    = NULL;
		
		$this->_FindServerPid();
	}

	function _FindServerPid () {
		/* Get the path to the PID file
		 */
		$pid_path = $this->config->FindValue('server!pid_file');

		if (empty($pid_path)) {
			PRINT_ERROR ("server!pid undefined");
			return -1;
		}

		/* Read its content
		 */
		if (!is_readable ($pid_path)) {
			PRINT_ERROR ("Can't access PID file: $pid_path");
			return -1;
		}

		$f = fopen($pid_path, 'r');
		if (!$f) {
			PRINT_ERROR ("Couldn't read PID file: $pid_path");
			return -1;
		}

		$c = fread ($f, filesize($pid_path));
		fclose($f);

		$this->pid = intval($c);
		return $this->pid;
	}

	function SendHUP () {
		if (empty ($this->pid)) {
			PRINT_ERROR ("Can't send HUP: PID unknown");
			return ret_error;
		}

		$cmd = "kill -HUP ". $this->pid .' 2>/dev/null >&- >/dev/null';
		exec ($cmd, $re);

		return ret_ok;
	}
}

?>