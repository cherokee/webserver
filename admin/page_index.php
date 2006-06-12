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

require_once ('menu_page.php');
require_once ('check.php');


$entries = array ('Port'      => array ('conf' => 'server!port',      'check' => 'check_is_int'),
		  'Keepalive' => array ('conf' => 'server!keepalive', 'check' => 'check_is_bool'),
		  'Listen'    => array ('conf' => 'server!listen',    'check' => 'check_is_ip'),
		  'PID file'  => array ('conf' => 'server!pid_file',  'check' => 'check_is_path')
	);


class PageIndexAjax {
	var $conf;
	var $params;

	function PageIndexAjax ($conf, $params) {
		$this->conf   = $conf;
		$this->params = $params;
	}

	function Render () {
		global $entries;

		$prop  = $this->params['prop'];
		$value = $this->params['value'];

		foreach ($entries as $name => $entry) {
			$fname = str_replace (' ', '_', $name);

			if ($fname == $prop) {
				$check = $entry['check'];

				$re = $check ($value);
				if ($re == NULL) 
					return 'ok'.CRLF;
				else 
					return $re.CRLF;
			}
		}

		return 'Property not found'.CRLF;
	}
}


class PageIndex extends MenuPage {

	function PageIndex ($theme, $conf, $params) {
		$this->MenuPage ($theme, $params);

		$this->conf = $conf;
		$this->body = $theme->LoadFile('pag_index.inc');
	}

	function GetPageTitle () {
		return NULL;
	}
	function GetBody () {
		return $this->body;
	}

	function _GetJavaScriptHeaders () {
		$js1 = array('yahoo/yahoo',
			     'event/event',
			     'dom/dom',
			     'connection/connection');
		$js2 = Page::_GetJavaScriptHeaders();

		return array_merge ($js1, $js2);
	}

	function GetServerInfoPanel () {
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

		property_clicked = function (event) {
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

		global $entries;

		$table_rows = '';
		$events_js  = '';

		foreach ($entries as $name => $entry) {
			$fname = str_replace (' ', '_', $name);

			$table_rows .= "<tr><td>$name</td><td><div id=\"$fname\">" . 
				$this->conf->FindValue($entry['conf']) .
				"</div></td></tr>";
			
			$events_js .= "YAHOO.util.Event.addListener(\"$fname\", \"click\", property_clicked);" . CRLF;
		}

		return $general_js. '
	        	<table class="pretty-table">
	            		<tr><th>Setting</th><th>Value</th></tr>'
				. $table_rows .
			'</table>
			<script type="text/javascript">//<![CDATA[ 
			'. $events_js .' 
			//]]></script>';
	}

	function GetVirtualServersPanel () {
		return "<br/><br/><br/>TODO: Virtual Server Table";
	}
}

?>