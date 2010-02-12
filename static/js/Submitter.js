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

function Submitter (id, url) {
    this.submitter_id  = id;
    this.url           = url;
    this.key_pressed   = false;
    this.check_changed = false;

    this.setup = function (self) {
	   /* When input looses focus */
	   var pre = "#submitter" + this.submitter_id;
	   $(pre+" :text, "+pre+" :password").bind ("blur",     this, this.input_blur_cb);
	   $(pre+" :text, "+pre+" :password").bind ("keypress", this, this.input_keypress_cb);
	   $("#submitter" + this.submitter_id + " :checkbox").bind ("change", this, this.input_checkbox_cb);
	   $("#submitter" + this.submitter_id + " select").bind ("change", this, this.input_combobox_cb);

	   /* Read the original values */
	   self.orig_values = {};
	   $("#submitter" + self.submitter_id).children(":text").each(function(){
		  self.orig_values[this.id] = this.value;
	   });
    }

    this.restore_orig_values = function (self) {
        for (var key in self.orig_values) {
		  $("#submitter"+ self.submitter_id +" #"+key).attr (
			 'value', self.orig_values[key]
		  );
        }
    }

    this.is_fulfilled = function (self) {
	   var full = true;
	   var pre  = "#submitter"+ self.submitter_id;

	   $(pre +" .required:text, "+ pre +" .required:password").each(function() {
		  if (! this.value) {
			 full = false;
		  }
	   });

	   return full;
    }

    this.submit_form = function (self) {
	   var pre = "#submitter" + self.submitter_id;

	   /* Block the fields */
	   $(pre +" input").attr("disabled", true);
           // XXX: Probably we need to know if it's input or select... is better to have an ID for the field...
	   $("#submitter"+ self.submitter_id +" input").after('<img class="notice" id="notice' + self.submitter_id  + '" src="/CTK/images/loading.gif" alt="Submitting..."/>');
	   /* Build the post */
	   info = {};
	   $(pre +" input:text, "+ pre +" input:password, "+ pre +" input:hidden").each(function(){
		  info[this.name] = this.value;
	   });
	   $(pre +" input:checkbox").each(function(){
		  info[this.name] = this.checked ? "1" : "0";
	   });
	   $(pre +" select").each(function(){
		  info[this.name] = this.value;
	   });

	   /* Remove error messages */
	   filter = "#submitter"+ self.submitter_id +" div.error";
	   $(filter).html("");

	   /* Async post */
	   $.ajax ({
		  type:     'POST',
		  url:       self.url,
		  async:     true,
		  dataType: 'json',
		  data:      info,
		  success:   function (data) {
			 if (data['ret'] != "ok") {
				/* Set the error messages */
				for (var key in data['errors']) {
				    filter = "#submitter"+ self.submitter_id + " div.error[key='"+ key +"']";
				    $(filter).html (data['errors'][key]);
				}

				/* Update the fields */
				for (var key in data['updates']) {
				    filter = "#submitter"+ self.submitter_id + " input[name='"+ key +"']";
				    $(filter).attr ('value', data['updates'][key]);
				}
			 }
			 if (data['redirect'] != undefined) {
				window.location.replace (data['redirect']);
			 }
		  },
		  error: function (xhr, ajaxOptions, thrownError) {
			 self.restore_orig_values (self);
			 alert ("Error: " + xhr.status +"\n"+ xhr.statusText);
		  },
		  complete:  function (XMLHttpRequest, textStatus) {
			 /* Unlock fields */
			 $("#notice"+ self.submitter_id).remove();
                         // XXX: Probably we need to know if it's input or select...
			 $("#submitter"+ self.submitter_id +" input").removeAttr("disabled");
		  }
	   });
    }

    this.input_keypress_cb = function(event) {
	   var self = event.data;

	   /* Enter -> Focus next input */
	   if (event.keyCode == 13) {
		  var inputs = $("input").not("input:hidden");
		  var n      = inputs.index(this);
		  var next   = (n < inputs.length -1) ? n+1 : 0;

		  inputs[next].blur();
		  inputs[next].focus();
		  return;
	   }

	   self.key_pressed = true;
    }

    this.input_checkbox_cb = function (event) {
	   var self = event.data;
	   self.check_changed = true;

	   if (! self.is_fulfilled(self)) {
		  return;
	   }

	   self.submit_form(self);
    }

    this.input_combobox_cb = function(event) {
	   var self = event.data;
	   self.check_changed = true;

	   if (! self.is_fulfilled(self)) {
		  return;
	   }

	   self.submit_form(self);
    }

    this.input_blur_cb = function (event) {
	   var self = event.data;
	   var pre  = "#submitter" + self.submitter_id;

	   /* Only proceed when something */
	   if (! self.key_pressed) {
		  return;
	   }

	   /* Procced on the last entry */
	   var last = $(pre +" :text, "+ pre +" :password").filter(".required:last");

	   if ((last.attr('id') != undefined) &&
		  (last.attr('id') != event.currentTarget.id))
	   {
		  return;
	   }

	   /* Check fields fulfillness */
	   if (! self.is_fulfilled(self)) {
		  return;
	   }

	   self.submit_form (self);
    }
}
