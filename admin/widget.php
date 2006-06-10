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


class Widget {
	var $id;
   
	function Widget ($name) {
		$this->id = uniqid ($name.'_'.time());
	}

	/* Headers
	 */
	function GetCSSIncludes () {
		if (! method_exists ($this, '_GetCSSIncludes'))
			return NULL;
		
		return $this->_GetCSSIncludes();
	}
	function GetJavaScriptIncludes () {
		if (! method_exists ($this, '_GetJavaScriptIncludes'))
			return NULL;
		
		return $this->_GetJavaScriptIncludes();
	}
	
	/* Body
	 */
	function Render () {
		if (! method_exists ($this, '_Render'))
			return NULL;
		
		return $this->_Render();
	}

	/* Methods
	 */
	function GetID () {
		return $this->id;
	}
}

?>