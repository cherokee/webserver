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
require_once ('widget_prop_table.php');
require_once ('widget_vsrvs_table.php');


$entries_index = 
	array ('Port'      => array ('type' => 'int',  'conf' => 'server!port',      'check' => 'check_is_int'),
	       'Keepalive' => array ('type' => 'bool', 'conf' => 'server!keepalive', 'check' => 'check_is_bool', 'fix' => 'fix_bool'),
	       'Listen'    => array ('type' => 'text', 'conf' => 'server!listen',    'check' => 'check_is_ip'),
	       'PID file'  => array ('type' => 'text', 'conf' => 'server!pid_file',  'check' => 'check_is_path'));


class PageIndexAjax {
	var $tab;

	function PageIndexAjax (&$conf, $params) {
		$this->tab =& new WidgetPropTableAjax ($conf, $params);
	}

	function Render () {
		global $entries_index;
		return $this->tab->Render ($entries_index);
	}
}

class PageIndex extends MenuPage {
	var $tab;

	function PageIndex (&$theme, &$conf, $params) {
		$this->MenuPage ($theme, $params);

		$this->conf =& $conf;
		$this->body =  $theme->LoadFile('pag_index.inc');
		
		/* Property table
		 */
		$this->tab = new WidgetPropTable ($conf, 'index');
		$this->AddWidget ('tab', &$this->tab);

		global $entries_index;
		foreach ($entries_index as $name => $e) {
			$this->tab->AddEntry ($name, $e['type'], $e['conf'], $e['check']);
		}

		/* Virtual servers table
		 */
		$this->vsrvs = new WidgetVSrvsTable ($conf);
		$this->AddWidget ('vsrvs', &$this->vsrvs);
	}

	function GetPageTitle () {
		return NULL;
	}
	function GetBody () {
		return $this->body;
	}

	function GetServerInfoPanel () {
		return $this->tab->Render();
	}

	function GetVirtualServersPanel () {
		return $this->vsrvs->Render();
	}

	function GetApplyButton () {
		$js   = '
	        <script type="text/javascript">//<![CDATA[ 

			restarted_ok = function (resp) {
				;
			}			
			restarted_failed = function (resp) {
		   		alert ("failed: " + resp.responseText);
			}			

			var restart_cb = {
			  	success: restarted_ok, 
	                        failure: restarted_failed
                        };

			function apply_changes_clicked (event) {
				var post_data = "page=restart";
		   		YAHOO.util.Connect.asyncRequest ("POST", document.location.href, restart_cb, post_data);
			}
		//]]></script>
		';

		$html = '<div align="right">
			   <input type="button" value="Apply Changes" onclick="return apply_changes_clicked();">
			 </div>';

		return $js . $html;
	}
}

?>