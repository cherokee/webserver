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
    var StarRating = function (element) {
	   var obj  = this;       // Object {}
	   var self = $(element); // $(<div>)

	   // PRIVATE
	   //
	   function get_star_number (star) {
		  return $(star).find("a").text();
	   }

	   function fill_to (idx, hover) {
		  if (idx <= -1) {
			 for (var i=1; i<=5; i++) {
				var $star = obj.$parent.find('.star'+i);
				$star.removeClass('ui-stars-star-hover')
				     .removeClass('ui-stars-star-on');
			 }
			 return;
		  }

		  var addClass = hover ? 'ui-stars-star-hover' : 'ui-stars-star-on';
		  var remClass = hover ? 'ui-stars-star-on' : 'ui-stars-star-hover';

		  for (var i=1; i<=5; i++) {
			 var $star = obj.$parent.find('.star'+i);
			 if (idx >= i) {
				$star.removeClass(remClass).addClass(addClass);
			 } else {
				$star.removeClass(addClass).addClass(remClass);
			 }
		  }
	   }

	   // PUBLIC
	   //
	   this.init = function (self, options) {
		  obj.$select = self.find('select');  // <select>
		  obj.$parent = obj.$select.parent(); // <div>
		  obj.score   = options.selected;
		  obj.can_set = options.can_set;

		  /* Hide the <select> */
		  obj.$select.hide();
		  obj.$select.val (obj.score);

		  /* Add the stars */
		  var stars_html = '';
		  for (var i=1; i<=5; i++) {
			 if (obj.can_set) {
				stars_html += '<div class="star'+i +' ui-stars-star"><a>'+ i +'</a></div>';
			 } else {
				stars_html += '<div class="star'+i +' ui-stars-star ui-stars-star-disabled"><a>'+ i +'</a></div>';
			 }
            }
		  obj.$parent.append (stars_html);

		  /* Initial value */
		  fill_to (obj.score, true);

		  /* Events */
		  if (obj.can_set) {
			 obj.$parent.find ('.ui-stars-star')
			 .bind('mouseover', function() {
				var idx = get_star_number (this);
				fill_to (idx, true);
			 })
			 .bind('mouseout', function() {
				fill_to (obj.score, true);
			 })
			 .bind('click', function() {
				var idx = get_star_number (this);
				obj.score = parseInt(idx);
				fill_to (idx, true);
				obj.$select.val(idx);
				obj.$select.trigger ({type: 'change', value: idx})
			 });
		  }
	   };
    };

    $.fn.StarRating = function (options) {
	   var self = this;
	   return this.each (function() {
		  if ($(this).data('stars')) return;

		  var stars = new StarRating (this);
		  $(this).data('stars', stars);
		  stars.init (self, options);
	   });

    };
})(jQuery);
