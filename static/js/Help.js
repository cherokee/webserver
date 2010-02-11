/* CTK: Cherokee Toolkit
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2009 Alvaro Lopez Ortega
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

/*
<div class="help">
   <div class="help_entry"><a href="/help/General Configuration.html">config_general</a></div>
   <div class="help_entry"><a href="/help/Configuration Quickstart.html">config_quickstart</a></div>
   <div class="help_group" id="help_group_help_server!collector">
      <div class="help_group" id="help_group_rrd">
          <div class="help_entry"><a href="/help/prueba.html">prueeeeeeeba</a></div>
      </div>
      <div class="help_group" id="help_group_post_track">
          <div class="help_entry"><a href="/help/modules_handlers_post_report.html">POST Report</a></div>
      </div>
   </div>
   <div class="help_group" id="help_group_help_server!post_track">
      <div class="help_group" id="help_group_post_track">
          <div class="help_entry"><a href="/help/modules_handlers_post_report.html">POST Report</a></div>
      </div>
   </div>
</div>
*/

function Help_update_group (group_prefix, active_value) {
    prefix = group_prefix.replace('!','_');

    /* Hide all the entries */
    selector = '.help #help_group_'+prefix;
    $(selector).children().each(function(){
	   $(this).hide();
	   //console.log ('Hides #'+this.id);
    });

    /* Show the right group */
    selector = '.help #help_group_'+prefix+' #help_group_'+active_value;
    $(selector).show();

    //console.log ("Show: #"+$(selector).attr('id'));
}
