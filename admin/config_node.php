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

class ConfigNode {
	var $child;
	var $value;

	function ConfigNode() {
		$this->child = array();
		$this->value = NULL; 
	}

	function ParseLine ($line) {
		/* Split left and right sides
		 */
		list($left, $right) = explode(' = ', $line);

		/* Get the first element
		 */
		$sep = strchr ($left, '!');

		/* Is it a final value?
		 */
		if (empty($sep)) {
			$this->value = $right;
			return ret_ok;
		} 
		
		/* Intermediate node
		 */
		$first = substr ($left, 0, strlen($left) - strlen($sep));
		$rest  = substr ($left, strlen($first) + 1, strlen($left)-(strlen($first)+1));
		
		if (! array_key_exists ($first, $this->child)) {
			$this->child[$first] = new ConfigNode;
		}
		$subconf = $this->child[$first];
		$subconf->ParseLine ($rest);

		return ret_ok;
	}

	function ParseString ($string) {	
		$lines = preg_split ('/[\n\r]+/', $string);

 		foreach ($lines as $l) {
			/* Clean it up
			 */
			if (empty($l)) 
				continue;
			$line = trim ($l);

			/* Ignore comments
			 */
			if ($line[0] == '#')
				continue;
			if (strlen($line) <= 4)
				continue;

			$ret = $this->ParseLine($line);
			if ($ret != ret_ok) return $ret;
 		}
	}

	function _ReadFile ($path) {
		/* Read the content
		 */
		if (!is_readable ($path))
			return ret_error;

		/* Lock and read the file
		 */
		$f = fopen ($path, 'r');
		if (!$f) return ret_error;
		
		if (!flock($f, LOCK_EX)) {
			PRINT_ERROR ("Couldn't lock ".$path);
			return ret_error;
		}
		
		$content = fread ($f, filesize($path));

		/* Unlock it and check the content
		 */
		flock($f, LOCK_UN);
		fclose ($f);

		if (empty($content))
			return ret_error;

		/* Parse it
		 */
		$ret = $this->ParseString ($content);
		if ($ret != ret_ok) return $ret;

		return ret_ok;
	}

	function Load ($path) {
		if (is_dir($path)) {
			$files = scandir ($path);
			foreach ($f as $files) {
				$file = $path.'/'.$f;

				$ret  = $this->_readFile ($file);
				if ($ret != ret_ok) return $ret;
			}
			return ret_ok;
		}

		return $this->_readFile ($path);
	}
}

?>