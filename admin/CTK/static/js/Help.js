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

function Help_update_group (group_prefix, active_value) {
    prefix = group_prefix.replace('!','_');

    /* Hide all the entries */
    selector = '.help #help_group_'+prefix;
    $(selector).children().each(function(){
	   $(this).hide();
    });

    /* Show the right group */
    selector = '.help #help_group_'+prefix+' #help_group_'+active_value;
    $(selector).show();
}

var help_a_size = 0;
function toggleHelp() {
    if ($("#help-a").width() == 230) {
        $("#help .help").fadeOut(200, function() {
            $("#help-a").animate({ width: help_a_size + 'px' }, 100);
        });
    } else {
        help_a_size = $("#help-a").width();
        $("#help-a").animate({ width: '230px' }, 100, function() {
            $("#help .help").fadeIn(200);
        });
    }
}

function Help_add_entries (helps) {
    var help = $('.help:first');

    /* Remove previously merged entries
     */
    $('.help_entry.merged').remove();

    /* Add new entries
     */
    for (var tmp in helps) {
	   var name = helps[tmp][0];
	   var file = helps[tmp][1];

	   if (file.search("://") == -1) {
		  url = '/help/'+file+'.html';
	   } else {
		  url = file;
	   }

	   help.append ('<div class="help_entry merged"><a href="'+url+'" target="cherokee_help">'+name+'</a></div>');
    }
}