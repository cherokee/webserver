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

require_once ('widget.php');


class WidgetPropTable extends Widget {
	var $entries;
	var $conf;
	
	function WidgetPropTable ($conf) {
		$this->Widget (get_class($this));
	
		$this->conf    = $conf;
		$this->entries = array();
	}

	function _GetJavaScriptIncludes () {
		return array('yui/yahoo/yahoo',
			     'yui/event/event',
			     'yui/dom/dom',
			     'yui/connection/connection',
			     'widget_prop_table');
	}

	function _Render () {
		$table_rows = '';
		$events_js  = '';

		foreach ($this->entries as $name => $entry) {
			$fname = str_replace(' ', '_', $name);
			
			$events_js .= "var $fname".'_params = new Array("'.$entry['type'].'", "'.$entry['conf'].'", "'.$fname.'");' .CRLF;

			if ($entry['type'] == 'bool') {
				if ($this->conf->FindValue($entry['conf']) == '1') 
					$checked = 'checked';

				$table_rows .= "<tr><td>$name</td><td><div id=\"$fname\">" . 
					"<input type=\"checkbox\" id=\"$fname"."_entry\" $checked />" .
					"</div></td></tr>".CRLF;					

				$events_js .= "YAHOO.util.Event.addListener ('$fname', 'click', property_bool_clicked, $fname".'_params);'.CRLF;
			} else {
				$table_rows .= "<tr><td>$name</td><td><div id=\"$fname\">" . 
					$this->conf->FindValue($entry['conf']) .
					"</div></td></tr>".CRLF;

				$events_js .= "YAHOO.util.Event.addListener ('$fname', 'click', property_text_clicked, $fname".'_params);' . CRLF;
			}
		}

		return '<table class="pretty-table">
	            		<tr><th>Setting</th><th>Value</th></tr>
				'. $table_rows .'
			</table>

			<script type="text/javascript">//<![CDATA[ 
			        '. $events_js .' 
			//]]></script>
		       ';
	}

	/* Public methods
	 */
	function AddEntry ($name, $type, $conf_key, $check_func) {
		$this->entries[$name] = array ('conf'  => $conf_key,
					       'type'  => $type,
					       'check' => $check_func);
		return ret_ok;
	}
}

?>