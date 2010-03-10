/* CTK: Cherokee Toolkit
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2010 Alvaro Lopez Ortega
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

;(function($) {
    var Submitter = function (element, url) {
	   var key_pressed = false;
	   var orig_values = {};
	   var obj         = this;       //  Object {}
	   var self        = $(element); // .submitter

	   // PRIVATE callbacks
	   //
	   function input_keypress_cb (event) {
		  key_pressed = true;
	   };

	   function input_blur_cb (event) {
		  /* Only proceed when something */
		  if (! key_pressed) {
			 return;
		  }

		  /* Procced on the last entry */
		  var last = self.find(":text,:password,textarea").filter(".required:last");
		  if ((last.attr('id') != undefined) &&
			 (last.attr('id') != event.currentTarget.id))
		  {
			 return;
		  }

		  /* Check fields fulfillness */
		  if (! is_fulfilled()) {
			 return;
		  }
		  submit_form();
	   };

	   function input_checkbox_cb (event) {
		  //var self = event.data;

		  if (! is_fulfilled()) {
			 return;
		  }
		  submit_form();
	   }

	   function input_combobox_cb (event) {
		  if (! is_fulfilled()) {
			 return;
		  }
		  submit_form();
	   }

	   // PRIVATE
	   //
	   function is_fulfilled () {
		  var full = true;
		  self.find (".required:text, .required:password, textarea.required").each(function() {
			 if (! this.value) {
				full = false;
			 }
		  });
		  return full;
	   }

	   function restore_orig_values () {
		  for (var key in orig_values) {
			 self.find("#"+key).attr ('value', self.orig_values[key]);
	       }
	   }

	   function submit_form() {
		  /* Block the fields */
		  $("#activity").show();
		  self.find("input,select,textarea").attr("disabled", true);

		  /* Build the post */
		  info = {};
		  self.find ("input:text, input:password, input:hidden").each(function(){
			 info[this.name] = this.value;
		  });
		  self.find ("input:checkbox").each(function(){
			 info[this.name] = this.checked ? "1" : "0";
		  });
		  self.find ("select").each(function(){
			 info[this.name] = $(this).val();
		  });
		  self.find ("textarea").each(function(){
			 info[this.name] = $(this).val();
		  });

		  /* Remove error messages */
		  self.find('div.error').html('');

		  /* Async POST */
		  $.ajax ({
			 type:     'POST',
			 url:       url,
			 async:     true,
			 dataType: 'json',
			 data:      info,
			 success:   function (data) {
				if (data['ret'] != "ok") {
				    /* Set the error messages */
				    for (var key in data['errors']) {
					   self.find ("div.error[key='"+ key +"']").html(data['errors'][key]);
					   had_errors = 1;
				    }

				    /* Update the fields */
				    for (var key in data['updates']) {
					   self.find ("input[name='"+ key +"']").attr('value', data['updates'][key]);
				    }
				}
				if (data['redirect'] != undefined) {
				    window.location.replace (data['redirect']);
				}

				/* Trigger events */
				if (data['ret'] == "ok") {
				    self.trigger('submit_success');
				} else {
				    self.trigger('submit_fail');
				}
			 },
			 error: function (xhr, ajaxOptions, thrownError) {
				restore_orig_values();
				// alert ("Error: " + xhr.status +"\n"+ xhr.statusText);
				self.trigger('submit_fail');
			 },
			 complete:  function (XMLHttpRequest, textStatus) {
				/* Unlock fields */
				$("#activity").fadeOut('fast');
				self.find("input,select,textarea").removeAttr("disabled");
			 }
		  });
	   }

	   // PUBLIC
	   //
	   this.submit_form = function() {
		  submit_form (obj);
		  return obj;
	   };

	   this.init = function (self) {
		  /* Events */
		  self.find(":text, :password, textarea").not('.noauto').bind ('keypress', self, input_keypress_cb);
		  self.find(":text, :password, textarea").not('.noauto').bind ("blur", self, input_blur_cb);
		  self.find(":checkbox").not('.required,.noauto').bind ("change", self, input_checkbox_cb);
		  self.find("select").not('.required,.noauto').bind ("change", self, input_combobox_cb);

		  /* Original values */
		  self.find(":text,textarea").each(function(){
			 orig_values[this.id] = this.value;
		  });

		  return obj;
	   }
    };

    $.fn.Submitter = function (url) {
	   var self = this;
	   return this.each(function() {
		  if ($(this).data('submitter')) return;
		  var submitter = new Submitter(this, url);
		  $(this).data('submitter', submitter);
		  submitter.init(self);
	   });
    };

})(jQuery);

// REF: http://www.virgentech.com/code/view/id/3
