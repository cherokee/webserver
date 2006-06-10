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


class Page {
	var $widgets;

	function Page () {
		$this->widgets = array();
	}

	/* Private methods
	 */
	function _GetCSSHeaders () {
		$css = array();

		foreach ($this->widgets as $name => $widget) {
			$css_js = $widget->GetCSSIncludes();

			if (empty ($css_js))
				return NULL;

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
				return NULL;

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
				$header .= '<link rel="stylesheet" type="text/css" href="'.YUI_www_path.'/'.$css_entry.'.css" />'.CRLF;
			}
		}

		if (!empty ($js)) {
			foreach ($js as $js_entry) {
				if (YUI_use_min)
					$js_entry .= "-min";
				$header .= '<script type="text/javascript" src="'.YUI_www_path.'/'.$js_entry.'.js"></script>'.CRLF;
			}
		}
		
		return $header;
	}

	function _GetBodyText () {
		$body = '';

		foreach ($this->widgets as $name => $widget) {
			$body .= "<!-- Widget: $name -->" . CRLF;
			$body .= $widget->Render() . CRLF;
		}

		return $body;
	}
	
	/* Public methods
	 */
	function AddWidget ($name, $widget) {
		if (array_key_exists ($name, $this->widgets)) {
			PRINT_ERROR ("Widget $name already exists");
			return ret_error;
		}

		$this->widgets[$name] = &$widget;
		return ret_ok;
	}
	
	function Render () {
		$js_headers  = $this->_GetJavaScriptHeaders();
		$css_headers = $this->_GetCSSHeaders();

		$header = $this->_GetHeaderText ($css_headers, $js_headers);
		$body   = $this->_GetBodyText ();

 		$page  = '<!doctype html public "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">'.CRLF;
		$page .= '<html>'.CRLF;
		$page .= '<head>'.CRLF;
		$page .= $header;
		$page .= '</head><body>'.CRLF;
		$page .= $body;
		$page .= '</body>'.CRLF;
		$page .= '</html>';

		return $page;
	}
}

?>