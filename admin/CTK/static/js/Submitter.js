/* CTK: Cherokee Toolkit
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2010-2014 Alvaro Lopez Ortega
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

// Workaround to prevent double inclussion
if ((typeof submitter_loaded) == 'undefined') {
    submitter_loaded = true;

 ;(function($) {
    var Submitter = function (element, url, optional) {
	   var optional_str = optional;
	   var key_pressed  = false;
	   var orig_values  = {};
	   var obj          = this;       //  Object {}
	   var self         = $(element); // .submitter

	   // PRIVATE callbacks
	   //
	   function input_keypress_cb (event) {
	       if (event.keyCode == 13) {
			 submit_form (true);
			 return;
	       }

		  key_pressed = true;
	   };

	   function input_keypress_noauto_cb (event) {
	       if (event.keyCode == 13) {
			 submit_form (true);
	       }
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
		  submit_form (true);
	   };

	   function input_checkbox_cb (event) {
		  if (! is_fulfilled()) {
			 return;
		  }
		  submit_form (true);
	   }

	   function input_combobox_cb (event) {
		  if (! is_fulfilled()) {
			 return;
		  }
		  submit_form (true);
	   }

	   // PRIVATE
	   //
	   function is_fulfilled () {
		  var full = true;
		  self.find (".required:text, .required:password, textarea.required").not('.optional').each(function() {
			 if (! this.value) {
				full = false;
				return false; /* stops iteration */
			 }
		  });
		  return full;
	   }

	   function restore_orig_values () {
		  for (var key in orig_values) {
			 self.find("#"+key).attr ('value', orig_values[key]);
	       }
	   }

	   function submit_in() {
		  $("#activity").show();
		  self.find("input,select,textarea").attr("disabled", true);
	   }

	   function submit_out() {
		  $("#activity").fadeOut('fast');
		  self.find("input,select,textarea").removeAttr("disabled");
	   }

	   function submit_form (async) {
		  /* Block the fields */
		  submit_in();

		  /* Build the post */
		  info = {};
		  self.find ("input:text, input:password, input:hidden, input:radio:checked").each(function(){
			 if ((!$(this).hasClass('optional')) || (this.value != optional_str)) {
				info[this.name] = this.value;
			 }
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

		  if (info.__count__ == 0) {
			 submit_out();
			 return;
		  }

		  /* Remove error messages */
		  self.find('div.error').html('');

		  /* Async POST */
		  $.ajax ({
			 'type':     'POST',
			 'url':       url,
			 'async':     async,
			 'dataType': 'json',
			 'data':      info,
			 'success':   function (data) {
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
				var event = {'url': url, 'ret': data['ret'], 'ret_data': data};

				if (data['ret'] == "ok") {
				    event['type'] = 'submit_success';
				} else {
				    event['type'] = 'submit_fail';
				}
				self.trigger (event);

				/* Modified: Save button */
				var modified     = data['modified'];
				var not_modified = data['not-modified'];

				if (modified != undefined) {
				    $(modified).show();
				    $(modified).removeClass('saved');
				} else if (not_modified) {
				    $(not_modified).addClass('saved');
				}
			 },
			 error: function (xhr, ajaxOptions, thrownError) {
				restore_orig_values();
				// alert ("Error: " + xhr.status +"\n"+ xhr.statusText);
				self.trigger({type: 'submit_fail', url: url, status: xhr.status});
			 },
			 complete: function (XMLHttpRequest, textStatus) {
				/* Unlock fields */
				submit_out();

				/* Update Optional fields */
				self.find('.optional').each(function() {
				    $(this).trigger('update');
				});
			 }
		  });
	   }

	   // PUBLIC
	   //
	   this.submit_form = function() {
		  submit_form (true);
	   };

	   this.submit_form_sync = function() {
		  return submit_form (false);
	   };

	   this.init = function (self) {
		  /* Events */
		  self.find(":text, :password, textarea").not('.noauto').bind ('keypress', self, input_keypress_cb);
		  self.find(":text, :password").filter('.noauto').bind ('keypress', self, input_keypress_noauto_cb);
		  self.find(":text, :password, textarea").not('.noauto').bind ("blur", self, input_blur_cb);
		  self.find(":checkbox, :radio").not('.required,.noauto').bind ("change", self, input_checkbox_cb);
		  self.find("select").not('.required,.noauto').bind ("change", self, input_combobox_cb);

		  /* Original values */
		  self.find(":text,textarea").each(function(){
			 orig_values[this.id] = this.value;
		  });

		  return obj;
	   }
    };

    $.fn.Submitter = function (url, optional) {
	   var self = this;
	   return this.each(function() {
		  if ($(this).data('submitter')) return;
		  var submitter = new Submitter(this, url, optional);
		  $(this).data('submitter', submitter);
		  submitter.init(self);
	   });
    };

  })(jQuery);

} // Double inclusion

// REF: http://www.virgentech.com/code/view/id/3
