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


class WidgetVSrvsTable extends Widget {
	var $conf;

	function WidgetVSrvsTable (&$conf) {
		$this->Widget (get_class($this));

		$this->conf = $conf;
	}

	function _GetJavaScriptIncludes () {
		return array('yui/yahoo/yahoo',
			     'yui/event/event',
			     'yui/dom/dom',
			     'yui/connection/connection',
			     'widget_vsrvs_table');
	}

	function _Render () {
		$table_rows = '';
		$events_js  = '';

		$vserv = $this->conf->Find('vserver');
		foreach ($vserv->child as $name => $subconf) {
			$label_name  = 'srv_label_'.$name;
			$checkbox    = '<input type="checkbox">';

			$table_rows .= "<tr><td>$checkbox</td><td><div id=\"$label_name\">$name</div></td></tr>" .CRLF;
			$events_js  .= "YAHOO.util.Event.addListener ('$label_name', 'click', vsrv_label_clicked, '$name');" .CRLF;
		}		

		return '<form method="POST" id="vsrvs_hidden_form">
				   <input type="hidden" name="page" value="vserver">
				   <input type="hidden" name="vserver" value="default">
			</form>

			<table class="pretty-table" width="100%">
	            		<tr><th></th><th>Domain</th></tr>
				'. $table_rows .'
			</table>
			<input type="button" value="Delete">

			<script type="text/javascript">//<![CDATA[ 
			        '. $events_js .' 
			//]]></script>
		       ';
	}
}
