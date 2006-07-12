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


class Theme {
	var $name;
	var $base;

	function Theme ($name = 'default') {
		$this->name = $name;
		$this->base = $this->LoadFile('base.inc');
	}

	function LoadFile ($file) {
		$file_path = cherokee_default_theme_path . $file;

		if (! is_readable($file_path)) {
			PRINT_ERROR ("Couldn't access ".$file_path);
			return NULL;
		}

		$f = fopen ($file_path, 'r');
		if (!$f) return NULL;

		$content = fread ($f, filesize($file_path));
		fclose ($f);

		return $content;
	}

	function _FindReplacements ($page) {
		$txt = $this->base;

		for (;;) {
			/* Look for the next macro
			 */
			$re = preg_match ('/%%([^%]+?)%%/', $txt, $matches);
			if (!$re) break;

			/* Resolve it with Page
			 */
			$target = $matches[1];
			$method = 'Get'.ucwords($target);

			$found = false;
			foreach (get_class_methods(get_class($page)) as $n => $key) {
				if (strtolower($method) == strtolower($key)) {
					$found = true;
					break;
				}
			}
			
			if (! $found) {
				PRINT_ERROR ("Theme problem: Unimplemented Page method: $method");
				break;
			}
			
			$result = $page->$method ();
			$txt = str_replace ("%%$target%%", $result, $txt);
		}

		return $txt;
	}

	function Render ($page) {
		return $this->_FindReplacements ($page);
	}
}

?>