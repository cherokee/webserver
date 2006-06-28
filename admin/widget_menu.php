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

$menu_layout = 
	array ('General'    => array ('index'    => array ('link' => '/',            'text' => 'Basic'),
				      'advanced' => array ('link' => '/advanced',    'text' => 'Advanced')),
	       'Debug Tree' => array ('debug'    => array ('link' => '/?page=debug', 'text' => 'Debug')));


class WidgetMenu extends Widget {
	var $page_name;

	function WidgetMenu ($page_name='') {
		$this->Widget (get_class($this));
		$this->page_name = $page_name;
	}
	
	function _RenderMenuGroup ($group_name, $content) {
		return "<li><strong>$group_name</strong>
			  <ul>
			    $content
			  </ul>
			</li>
		       ";
	}

	function _RenderMenuEntry ($menu_entry) {
		if ($page_name == $this->page_name) 
			return '<li><a href="'.$menu_entry['link'].'" class="selected">'.$menu_entry['text'].'</a></li>';
		else 
			return '<li><a href="'.$menu_entry['link'].'">'.$menu_entry['text'].'</a></li>';
	}

	function _Render () {
		$menu_tmp = '';

		global $menu_layout;
		foreach ($menu_layout as $group => $group_entry) {

			$menu_content = '';
			foreach ($group_entry as $entry_name => $entry) {
				$menu_content .= $this->_RenderMenuEntry ($entry);
			}

			$menu_tmp .= $this->_RenderMenuGroup ($group, $menu_content);
		}

		$menu = "
			<ul id=\"nav\">
			  $menu_tmp
			</ul>
			";

		$apply_changes = '
			<div align="center">
			   <input type="button" value="Apply Changes" onclick="return apply_changes_clicked();">
			</div>
		';

		return $menu . $apply_changes;
	}
}

?>