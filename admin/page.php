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
require_once ('widget.php');
require_once ('theme.php');


class Page {
	var $widgets;
	var $theme;
	var $params;

	function Page (&$theme) {
		$this->theme   =& $theme;
		$this->widgets =  array();
		$this->params  =  array();
	}

	/* Private 
	 */
	function _GetCSSHeaders () {
		$css = array();

		foreach ($this->widgets as $name => $widget) {
			$css_js = $widget->GetCSSIncludes();

			if (empty ($css_js))
				continue;

			foreach ($css_js as $i) {
				$js[$i] = $i;
			}
		}

		return $js;
	}

	function _GetJavaScriptHeaders () {
		$js = array();

		foreach ($this->widgets as $name => $widget) {
			$widget_js = $widget->GetJavaScriptIncludes();

			if (empty ($widget_js))
				continue;

			foreach ($widget_js as $i) {
				$js[$i] = $i;
			}
		}

		return $js;
	}

	function _GetHeaderText ($css, $js) {
		$header = '';

		if (!empty ($css)) {
			foreach ($css as $css_entry) {
				if (YUI_use_min)
					$css_entry .= "-min";
				$header .= '<link rel="stylesheet" type="text/css" href="/'.$css_entry.'.css" />'.CRLF;
			}
		}

		if (!empty ($js)) {
			foreach ($js as $js_entry) {
				if (YUI_use_min)
					$js_entry .= "-min";
				$header .= '<script type="text/javascript" src="/'.$js_entry.'.js"></script>'.CRLF;
			}
		}
		
		return $header;
	}
	
	/* Public methods
	 */
	function AddWidget ($name, $widget) {
		$name = strtolower($name);

		if (array_key_exists ($name, $this->widgets)) {
			PRINT_ERROR ("Widget $name already exists");
			return ret_error;
		}

		$this->widgets[$name] = &$widget;
		return ret_ok;
	}

	function Render () {
		return $this->theme->Render ($this);
	}

	/* Callbacks
	 */
	function GetHeader () {
		$js_headers  = $this->_GetJavaScriptHeaders();
		$css_headers = $this->_GetCSSHeaders();

		$header = $this->_GetHeaderText ($css_headers, $js_headers);
		return $header;
	}
}

?>