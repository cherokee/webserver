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

(function($) {
    var Carousel = function (element) {
	   var obj  = this;       // Object {}
	   var self = $(element); // $(<div>)

	   // PRIVATE
	   //
	   function set_item (num) {
		  for (var n=0; n<obj.child_num; n++) {
			 if (n == num) {
				obj.$child.eq(n).fadeIn(500);
				obj.$pager_child.eq(n).addClass('selected');
			 } else {
				obj.$child.eq(n).hide();
				obj.$pager_child.eq(n).removeClass('selected');
			 }
		  }

		  if (num == 0) {
			 obj.$prev.hide();
		  } else {
			 obj.$prev.show();
		  }

		  if (num == obj.child_num -1) {
			 obj.$next.hide();
		  } else {
			 obj.$next.show();
		  }

		  obj.current = num;
	   }

	   function advance_step (val) {
		  var new_current = obj.current + val;

		  if ((new_current >= 0) &&
			 (new_current < obj.child_num))
		  {
			 set_item (new_current);
		  }
	   }

	   function pager_click_cb() {
		  set_item (parseInt ($(this).text()) -1);
	   }

	   // PUBLIC
	   //
	   this.init = function (self, options) {
		  obj.$pager       = self.find('.pager');
		  obj.$pager_child = obj.$pager.children();
		  obj.$prev        = self.find('.prev');
		  obj.$next        = self.find('.next');
		  obj.$child       = self.find('.overview').children();
		  obj.child_num    = obj.$child.length;
		  obj.current      = 0;

		  obj.$prev.bind ('click', function(){ advance_step (-1); })
		  obj.$next.bind ('click', function(){ advance_step (+1); })

		  obj.$pager_child.each (function() {
			 $(this).bind ('click', pager_click_cb);
		  });

		  set_item (0);
	   }
    };

    $.fn.Carousel = function (options) {
	   var self = this;
	   return this.each (function() {
		  if ($(this).data('carousel')) return;
		  var carousel = new Carousel (this);
		  $(this).data('carousel', carousel);
		  carousel.init (self, options);
	   });
    };
})(jQuery);
