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


class WidgetMenu extends Widget {
	
	function WidgetMenu () {
		$this->Widget (get_class($this));
	}

	function _Render () {
		return '
            <ul id="nav">
                <li><strong>General</strong>
                    <ul>
                        <li><a href="#" class="selected">Config Summary</a></li>
                        <li><a href="#">Fundamentals</a></li>
                        <li><a href="#">Advanced Settings</a></li>

                    </ul>
                </li>
                <li><strong>File Handling</strong>
                    <ul>
                        <li><a href="#">MIME Types</a></li>
                        <li><a href="#">Content Negotiation</a></li>
                        <li><a href="#">Content Compression</a></li>

                        <li><a href="#">File Upload</a></li>
                    </ul>
                </li>
                <li><strong>URL Handling</strong>
                    <ul>
                        <li><a href="#">URL Mappings</a></li>
                        <li><a href="#">Gateway</a></li>

                        <li><a href="#">Home Directories</a></li>
                        <li><a href="#">Spelling Correction</a></li>
                    </ul>
                </li>
                <li><strong>API Support</strong>
                    <ul>
                        <li><a href="#">SSI</a></li>

                        <li><a href="#">CGI</a></li>
                        <li><a href="#">FastCGI</a></li>
                        <li><a href="#">PHP</a></li>
                        <li><a href="#">Perl</a></li>
                    </ul>
                </li>
            </ul>';
	}
}

?>