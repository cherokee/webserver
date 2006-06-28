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

	function &_CreatePath ($path) {
		$subconf =& $this;

		for (;;) {
			/* Get the first element
			 */
			$sep = strchr ($path, '!');

			/* Intermediate node
			 */
			if (empty ($sep)) {
				$first = $path;
				$rest  = '';
			} else {
				$first = substr ($path, 0, strlen($path) - strlen($sep));
				$rest  = substr ($path, strlen($first) + 1, strlen($path)-(strlen($first)+1));
			}
			
			/* Create a new node
			 */
			if (! array_key_exists ($first, $subconf->child)) {
				$subconf->child[$first] =& new ConfigNode;
			}
			
			$child   =& $subconf->child;
			$subconf =& $child[$first];

			if (empty($rest)) 
				return $subconf;
			
			$path = $rest;
		}
	}

	function ParseLine ($line) {
		/* Split left and right sides
		 */
		list($left, $right) = explode(' = ', $line);

		$subconf =& $this->_CreatePath ($left);
		if ($subconf == NULL) return ret_error;

		/* Assign the value
		 */
		if (!empty ($right)) {
			$subconf->value = $right;
		}

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

	function _Serialize ($path="") {
		$content = '';

		if ($this->value != NULL) {
			$line = $path . ' = ' . $this->value . CRLF;
			$content .= $line;
		}

		if ($this->child == NULL)
			return $content;

		foreach ($this->child as $name => $child) {
			if (! empty ($path)) 
				$new_path = $path . '!' . $name;
			else 
				$new_path = $name;
			
			$content .= $child->_Serialize ($new_path);
		}
		
		return $content;
	}

	function Save ($file) {
		/* Serialize the config
		 */
		$content = $this->_Serialize();

		/* Check that the file is writable
		 */
		if (!is_writable($file)) {
			PRINT_ERROR ("$file is not writable");
			return ret_error;
		}

		/* Write down the file
		 */
		$f = fopen ($file, 'w+');

		$re = fwrite ($f, $content);
		if ($re === FALSE) {
			PRINT_ERROR ("couldn't write content in $file");
			return ret_error;
		}

		fclose ($f);
		return ret_ok;
	}

	function &Find ($path) 
	{
		$subconf = &$this;
		
		for (;;) {
			$sep = strchr ($path, '!');
			
			/* If it isn't a final object, keep searching..
			 */
			if (! empty ($sep)) {
				$key = substr ($path, 0, strlen($path) - strlen($sep));

				if (! array_key_exists ($key, $subconf->child))
					return NULL;
				
				$child   =& $subconf->child;
				$subconf =& $child[$key];
				$path    =  substr ($sep, 1, strlen($sep) - 1);

				continue;
			}
			
			/* Check that the key actually exists
			 */
			$child =& $subconf->child; 

			if (! array_key_exists ($path, $child))
				return NULL;
			
			/* Found		
			 */
			$re =& $child[$path];
			return $re;
		}
	}

	function FindValue ($path) 
	{
		$re =& $this->Find ($path);
		if ($re == NULL) 
			return NULL;

		return $re->value;
	}

	function AddPath ($path) {
		$this->_CreatePath ($path);
	}

	function SetValue ($path, $value, $create=false) 
	{
		/* Look for the entry
		 */
		$entry =& $this->Find ($path);
		if ($entry == NULL) {
			/* There is no such element 
			 */
			if (! $create) 
				return "Node not found: ".$path;

			/* Add a new config node
			 */
			$entry = $this->_CreatePath ($path);

			if ($entry == NULL)
				return "Could not create: ".$path;
		}
		
		/* Fill it out
		 */
		$entry->value = $value;
		return NULL;
	}
}

?>