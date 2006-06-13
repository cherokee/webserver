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
		return array('yahoo/yahoo',
			     'event/event',
			     'dom/dom',
			     'connection/connection');
	}

	function _Render () {
		$general_js= '
		<script type="text/javascript">//<![CDATA[

		var id_being_edited = "";

		var property_clicked;
		var property_update_and_close;
		var property_check;
		var property_update_and_close_ok;
		var property_update_and_close_failed;

		property_update_and_close_ok = function (resp) {
			if (resp.responseText != "ok\r\n") {
				return property_update_and_close_failed (resp);
			}

			var div_area  = document.getElementById (id_being_edited);
			var text_area = document.getElementById (id_being_edited + "_entry");
			var value     = text_area.value;

			div_area.innerHTML = value;
			YAHOO.util.Event.addListener(id_being_edited, "click", property_clicked);

			id_being_edited = "";
		}
		property_update_and_close_failed = function (resp) {
			alert ("failed: " + resp.responseText);
		}			
	
		var property_check_cb = {
		  	success: property_update_and_close_ok, 
			failure: property_update_and_close_failed
		};

		property_update_and_close = function () {
			var div_area  = document.getElementById (id_being_edited);
			var text_area = document.getElementById (id_being_edited + "_entry");
			var value     = text_area.value;

			var post_data = "ajax=1&prop="+id_being_edited+"&value="+value;
			var request   = YAHOO.util.Connect.asyncRequest ("POST", document.location.href, property_check_cb, post_data);

			return false;
		}

		property_clicked = function (event, type) {
			var target = YAHOO.util.Event.getTarget (event, true);
			var obj    = document.getElementById (target.id);
			var old    = obj.innerHTML;

			if (id_being_edited.length != 0) {
				return false;
			}

			YAHOO.util.Event.preventDefault (event);
			YAHOO.util.Event.removeListener (target.id, "click", property_clicked);

			id_being_edited = target.id;

			obj.innerHTML = \'<form onsubmit="return property_update_and_close();"> \
	                                   <input type="textarea" id="\'+target.id+\'_entry" value="\'+old+\'" name="\'+target.id+\'" /> \
					   <input type="submit" value="OK" /> \
					 </form>\';
	    	}
		//]]></script>';

		$table_rows = '';
		$events_js  = '';

		foreach ($this->entries as $name => $entry) {
			$fname = str_replace(' ', '_', $name);

			$table_rows .= "<tr><td>$name</td><td><div id=\"$fname\">" . 
				       $this->conf->FindValue($entry['conf']) .
				       "</div></td></tr>";

			$events_js  .= "YAHOO.util.Event.addListener (\"$fname\", \"click\", ".
				       "property_clicked, \"".$entry['type']."\");" . CRLF;
		}

		return $general_js. '
	        	<table class="pretty-table">
	            		<tr><th>Setting</th><th>Value</th></tr>
				'. $table_rows .'
			</table>
			<script type="text/javascript">//<![CDATA[ 
			'. $events_js .' 
			//]]></script>';
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