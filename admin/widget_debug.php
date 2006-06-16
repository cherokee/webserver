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


class WidgetDebug extends Widget {
	var $conf;
	var $id_num;
	
	function WidgetDebug ($conf) {
		$this->Widget (get_class($this));
		$this->conf   = $conf;
		$this->id_num = 1;
	}

	function _GetCSSIncludes () {
		return array ('yui/treeview/assets/tree');
	}

	function _GetJavaScriptIncludes () {
		return array ('yui/yahoo/yahoo',
			      'yui/event/event',
			      'yui/connection/connection',
			      'yui/animation/animation',
			      'yui/dom/dom',
			      'yui/dragdrop/dragdrop',
			      'yui/treeview/treeview');
	}

	function _GetNextID () {
		$id = $this->id_num;
		$this->id_num++;
		return $id;
	}

	function _AddNode ($subconf, $prev_num) {
		$branch = '';

		foreach ($subconf->child as $entry => $subconf2) {
			$new_id  = $this->_GetNextID();

			$txt = $entry;
			if (!empty ($subconf2->value)) {
				$txt .= ' = '. $subconf2->value;
			}

			$branch .= "var tmpNode_$new_id = new YAHOO.widget.TextNode(\"$txt\", tmpNode_$prev_num, false);" .CRLF;
			$branch .= $this->_AddNode ($subconf2, $new_id);
		}

		return $branch;
	}

	function _GenerateJavaScriptTree () {
		$id = $this->GetID();
		$js = 'var tree;
		       var nodes = new Array();
		       tree = new YAHOO.widget.TreeView("'.$id.'");' .CRLF;

		foreach ($this->conf->child as $entry => $subconf) {
			$id = $this->_GetNextID();

			$js .= "var tmpNode_$id = new YAHOO.widget.TextNode(\"$entry\", tree.getRoot(), false);" .CRLF;
			$js .= $this->_AddNode ($subconf, $id);
		}

		$js .= 'tree.draw();';
		return $js;
	}

	function _Render () {
		$id = $this->GetID();

		$html  = '<a href="javascript:tree.expandAll()">Expand all</a> ';
		$html .= '<a href="javascript:tree.collapseAll()">Collapse all</a>';
		$html .= '<div id="'.$id.'"></div>';

		$js = '<script type="text/javascript">//<![CDATA[ ' .CRLF. 
		      $this->_GenerateJavaScriptTree()              .CRLF. 
		      ' //]]></script>';

		return $html . CRLF . $js;
	}
}

?>