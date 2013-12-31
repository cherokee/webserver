/* CTK: Cherokee Toolkit
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2014 Alvaro Lopez Ortega
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
    var SelectionPanel = function (element, table_id, content_id, cookie_name, cookie_domain, url_empty) {
	   var obj  = this;       //  Object {}
	   var self = $(element); // .submitter

	   function deselect_row (row) {
            $('#'+row.attr('pid')).removeClass('panel-selected');
	   }

	   function select_row (row) {
		  var url      = '';
		  var activity = $("#activity");
		  var content  = $('#'+content_id);

		  // Ensure there's a row to be selected
		  if (row.length == 0) {
			 url = url_empty;

		  } else {
			 url = row.attr('url');

			 $('#'+row.attr('pid')).each (function() {
				// Highlight
				$(this).addClass('panel-selected');

				// Cookie
				$.cookie (cookie_name, $(this).attr('id'), {path: cookie_domain});
			 });
		  }

		  // Update box
            activity.show();
		  $.ajax ({type:     'GET',
				 dataType: 'json',
				 async:    true,
				 url:      url,
				 success: function(data) {
					// New content: HTML + JS
					txt = data.html;
					txt += '<script type="text/javascript"> $(document).ready(function(){'
					txt += data.js
					txt += '}); </script>'

					// Update the Help
					Help_add_entries (data.helps);

					// Transition
					content.hide();
					content.html (txt);
   					resize_cherokee_panels();
					content.show();
                         activity.fadeOut('fast');
				 }
				});
	   }

	   function auto_select_row (row) {
		  var row_id     = row.attr('id');
		  var did_select = false;

		  self.find('.row_content').each (function() {
			 if ($(this).attr('id') == row_id) {
				select_row ($(this));
				did_select = true;
			 } else {
				deselect_row ($(this));
			 }
		  });

		  return did_select;
	   }

	   function filter_entries (text) {
		  var lower_text = text.toLowerCase();

		  self.find('.row_content').each (function() {
			 if ($(this).text().toLowerCase().search (lower_text) == -1) {
                    self.find('#'+$(this).attr('pid')).hide();
			 } else {
                    self.find('#'+$(this).attr('pid')).show();
			 }
		  });
	   }

	   this.get_selected = function() {
		  var selected = self.find('.panel-selected:first');
		  return $(selected);
	   }

	   this.set_selected_cookie = function (pid) {
		  var row = self.find ('.row_content[pid='+ pid +']');
		  $.cookie (cookie_name, row.attr('id'), {path: cookie_domain});
	   }

	   this.select_last = function() {
		  auto_select_row (self.find('.row_content:last'));
	   }

	   this.select_first = function() {
		  auto_select_row (self.find('.row_content:first'));
	   }

	   this.init = function (self) {
		  var cookie_selected = $.cookie(cookie_name);

		  /* Initial Selection */
		  if (cookie_selected == undefined) {
			 var first_row = self.find('.row_content:first');
			 select_row (first_row);

		  } else {
			 var did_select = auto_select_row ($('#'+cookie_selected).find('.row_content'));

			 if (! did_select) {
				var first_row = self.find('.row_content:first');
				select_row (first_row);
			 }
		  }

		  /* Events */
		  self.find('.row_content').bind('click', function() {
			 auto_select_row ($(this));
		  });

		  /* Filtering */
		  var filter_input = self.parents('.panel').find('input:text.filter');
		  filter_input.bind ("keyup", function(event) {
			 filter_entries ($(this).val());
		  });

		  if (! filter_input.hasClass('optional_msg')) {
			 filter_entries (filter_input.val());
		  }

		  /* Show it */
		  self.show();

		  return obj;
	   }
    };

    $.fn.SelectionPanel = function (table_id, content_id, cookie, cookie_domain, url_empty) {
	   var self = this;
	   return this.each(function() {
		  if ($(this).data('selectionpanel')) return;
		  var submitter = new SelectionPanel(this, table_id, content_id, cookie, cookie_domain, url_empty);
		  $(this).data('selectionpanel', submitter);
		  submitter.init(self);
	   });
    };

})(jQuery);



function resize_cherokee_panels() {
   if ($("#status_panel").length > 0) {
       $('#status_panel').height($(window).height() - 130);
       $('.status_content .ui-tabs .ui-tabs-panel').height($(window).height() - 140);
   }

   if ($("#vservers_panel").length > 0) {
       if ($('.selection-panel table').height() > $('#vservers_panel').height()) {
           $(".selection-panel table").addClass('hasScroll');
       } else {
           $(".selection-panel table").removeClass('hasScroll');
       }
       $('#vservers_panel').height($(window).height() - 130);
       $('.vserver_content .ui-tabs .ui-tabs-panel').height($(window).height() - 140);
   }

   if ($("#rules_panel").length > 0) {
       if ($('.selection-panel table').height() > $('#rules_panel').height()) {
           $(".selection-panel table").addClass('hasScroll');
       } else {
           $(".selection-panel table").removeClass('hasScroll');
       }
       $('#rules_panel').height($(window).height() - 130);
       $('.rules_content .ui-tabs .ui-tabs-panel').height($(window).height() - 140);
   }

   if ($("#source_panel").length > 0) {
       if ($('.selection-panel table').height() > $('#source_panel').height()) {
           $(".selection-panel table").addClass('hasScroll');
       } else {
           $(".selection-panel table").removeClass('hasScroll');
       }
       $('#source_panel').height($(window).height() - 130);
       $('#source-workarea').height($(window).height() - 92);
   }
}

$(document).ready(function() {
   resize_cherokee_panels();
   $(window).resize(function(){
       resize_cherokee_panels();
   });
});
