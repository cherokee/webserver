/* CTK: Cherokee Toolkit
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2009-2014 Alvaro Lopez Ortega
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

function get_cookie (key) {
    var i = document.cookie.indexOf (key+'=');
    if (i < 0) {
	   return;
    }

    i += key.length + 1;

    var e = document.cookie.indexOf(';', i);
    if (e < 0) {
	   e = document.cookie.length;
    }

    return unescape (document.cookie.substring (i, e));
}

function focus_next_input (input) {
    var inputs  = $("input,textarea").not("input:hidden");
    var n       = inputs.index(input);
    var next    = (n < inputs.length -1) ? n+1 : 0;
    var n_input = $(inputs[next]);

    n_input.blur();
    n_input.focus();
}
