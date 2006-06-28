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
require_once ('widget_prop_table.php');


$entries_vservers = 
	array ('Document Root'   => array ('type' => 'text', 'conf' => 'vserver!%name%!document_root',   'check' => 'check_is_path'),
	       'Aliases'         => array ('type' => 'text', 'conf' => 'vserver!%name%!alias',           'check' => 'check_file_list'),
	       'Directory Index' => array ('type' => 'text', 'conf' => 'vserver!%name%!directory_index', 'check' => 'check_nil'));


class PageVserverAjax {
	var $tab;

	function PageVserverAjax (&$conf, $params) {
		$this->tab =& new WidgetPropTableAjax ($conf, $params);
	}

	function Render () {
		global $entries_vservers;
		return $this->tab->Render ($entries_vservers);
	}
}

class PageVServer extends MenuPage {
	function PageVServer (&$theme, &$conf, $params) {
		$this->MenuPage ($theme, $params);
		
		$this->conf    =& $conf;
		$this->body    =  $theme->LoadFile('pag_vserver.inc');		
		$this->vserver =  $params['vserver'];

		$this->tab1    =& new WidgetPropTable (&$conf, 'vserver');
		$this->AddWidget ('tab1', &$this->tab1);

		global $entries_vservers;
		foreach ($entries_vservers as $name => $e) {
			$conf_entry = str_replace('%name%', $this->vserver, $e['conf']);
			$this->tab1->AddEntry ($name, $e['type'], $conf_entry, $e['check']);
		}
	}

	function GetPageTitle () {
		return 'Virtual Server: ' . $this->vserver;
	}

	function GetBody () {
		return $this->tab1->Render();
	}
}

?>