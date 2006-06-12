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


function check_is_int ($i) {
	if (!is_numeric ($i))
		return 'Non numeric';

	$d = intval($i);

	if ($d <= 0)
		return 'Negative value';
	if ($d > 65534) 
		return 'Out of range';

	return NULL;
}

function check_is_bool ($b) {
	if (is_bool ($b))
		return NULL;

	return "Wrong value";
}

function check_is_ipv4 ($ip) {
        $oct = explode('.', $ip);
        if (count($oct) != 4) {
            return "Bad formated";
        }

        for ($i = 0; $i < 4; $i++) {
            if (!is_numeric($oct[$i])) {
                return "Invalid value";
            }

            if ($oct[$i] < 0 || $oct[$i] > 255) {
                return "Our of range value";
            }
        }

	return NULL;
}

function check_is_ipv6 ($ip) {
	return NULL;
}

function check_is_ip ($ip) {
	$colon = strchr ($ip, ':');
	if ($colon != FALSE)
		return check_is_ipv6($ip);
	
	return check_is_ipv4($ip);
}

function check_is_path ($path) {
	if (is_windows()) {
		// TODO
		return "Windows: unsupported";
	}

	if ($path[0] != '/')
		return "Wrong beginning";

	return NULL;
}

?>