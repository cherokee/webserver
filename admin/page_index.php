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


$entries = array ('Port'      => array ('type' => 'int',  'conf' => 'server!port',      'check' => 'check_is_int'),
		  'Keepalive' => array ('type' => 'bool', 'conf' => 'server!keepalive', 'check' => 'check_is_bool', 'fix' => 'fix_bool'),
		  'Listen'    => array ('type' => 'text', 'conf' => 'server!listen',    'check' => 'check_is_ip'),
		  'PID file'  => array ('type' => 'text', 'conf' => 'server!pid_file',  'check' => 'check_is_path')
	);


class PageIndexAjax {
	var $conf;
	var $params;

	function PageIndexAjax (&$conf, $params) {
		$this->conf   =& $conf;
		$this->params =  $params;
	}

	function Render () {
		global $entries;
				
		$prop  = $this->params['prop'];
		$value = $this->params['value'];
		$confp = $this->params['conf'];

		if (empty ($prop))
			return "No property".CRLF;

		/* Update properties
		 */
		$found = false;

		foreach ($entries as $name => $entry) {
			$fname = str_replace (' ', '_', $name);
			
			if ($fname == $prop) {
				/* Validate the value
				 */
				$check = $entry['check'];
				
				$re = $check ($value);
				if ($re != NULL) 
					return $re.CRLF;
				
				/* Fix it up
				 */
				$fix = $entry['fix'];

				if (!empty($fix)) {
					$v2 = $fix ($value);
					$value = $v2;				
				}
				
				/* Update the configuration	
				 */
				$conf =& $this->conf;
				
				$re = $conf->SetValue ($confp, $value);
				if ($re != NULL) return $re.CRLF;
				
				/* Double check it
				 */
				$v =& $conf->FindValue ($confp);
				if ($v != $value)
					return "Didn't match: $v and .$value".CRLF;
				
				$found = true;
				break;
			}
		}


		if (! $found) 
			return 'Property not found'.CRLF;

		return 'ok'.CRLF;
	}
}


class PageIndex extends MenuPage {
	var $tab;

	function PageIndex (&$theme, &$conf, $params) {
		$this->MenuPage ($theme, $params);

		$this->conf =& $conf;
		$this->body =  $theme->LoadFile('pag_index.inc');

		$this->tab  = new WidgetPropTable (&$conf);
		$this->AddWidget ('tab', &$this->tab);

		global $entries;
		foreach ($entries as $name => $e) {
			$this->tab->AddEntry ($name, $e['type'], $e['conf'], $e['check']);
		}
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
		return "<br/><br/><br/>TODO: Virtual Server Table";
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