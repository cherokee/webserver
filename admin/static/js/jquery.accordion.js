/*
 * File redistributed by the Cherokee-Project making use of its
 * MIT License. Original license text follows.
 */

/*
 * Accordion 1.5 - jQuery menu widget
 *
 * Copyright (c) 2007 Jörn Zaefferer, Frank Marcia
 *
 * http://bassistance.de/jquery-plugins/jquery-plugin-accordion/
 *
 * Dual licensed under the MIT and GPL licenses:
 *   http://www.opensource.org/licenses/mit-license.php
 *   http://www.gnu.org/licenses/gpl.html
 *
 * Revision: $Id: jquery.accordion.js 2880 2007-08-24 21:44:37Z joern.zaefferer $
 *
 */

/**
 * Make the selected elements Accordion widgets.
 *
 * Semantic requirements:
 *
 * If the structure of your container is flat with unique
 * tags for header and content elements, eg. a definition list
 * (dl > dt + dd), you don't have to specify any options at
 * all.
 *
 * If your structure uses the same elements for header and
 * content or uses some kind of nested structure, you have to
 * specify the header elements, eg. via class, see the second example.
 *
 * Use activate(Number) to change the active content programmatically.
 *
 * A change event is triggered everytime the accordion changes. Apart from
 * the event object, all arguments are jQuery objects.
 * Arguments: event, newHeader, oldHeader, newContent, oldContent
 *
 * @example jQuery('#nav').Accordion();
 * @before <dl id="nav">
 *   <dt>Header 1</dt>
 *   <dd>Content 1</dd>
 *   <dt>Header 2</dt>
 *   <dd>Content 2</dd>
 * </dl>
 * @desc Creates an Accordion from the given definition list
 *
 * @example jQuery('#nav').Accordion({
 *   header: '.title'
 * });
 * @before <div id="nav">
 *  <div>
 *    <div class="title">Header 1</div>
 *    <div>Content 1</div>
 *  </div>
 *  <div>
 *    <div class="title">Header 2</div>
 *    <div>Content 2</div>
 *  </div>
 * </div>
 * @desc Creates an Accordion from the given div structure
 *
 * @example jQuery('#nav').Accordion({
 *   header: '.head',
 * 	 navigation: true
 * });
 * @before <ul id="nav">
 *   <li>
 *     <a class="head" href="books/">Books</a>
 *     <ul>
 *       <li><a href="books/fantasy/">Fantasy</a></li>
 *       <li><a href="books/programming/">Programming</a></li>
 *     </ul>
 *   </li>
 *   <li>
 *     <a class="head" href="movies/">Movies</a>
 *     <ul>
 *       <li><a href="movies/fantasy/">Fantasy</a></li>
 *       <li><a href="movies/programming/">Programming</a></li>
 *     </ul>
 *   </li>
 * </ul>
 * @after <ul id="nav">
 *   <li>
 *     <a class="head" href="">Books</a>
 *     <ul style="display: none">
 *       <li><a href="books/fantasy/">Fantasy</a></li>
 *       <li><a href="books/programming/">Programming</a></li>
 *     </ul>
 *   </li>
 *   <li>
 *     <a class="head" href="">Movies</a>
 *     <ul>
 *       <li><a class="current" href="movies/fantasy/">Fantasy</a></li>
 *       <li><a href="movies/programming/">Programming</a></li>
 *     </ul>
 *   </li>
 * </ul>
 * @desc Creates an Accordion from the given navigation list, activating those accordion parts
 * that match the current location.href. Assuming the user clicked on "Fantasy" in the "Movies" section,
 * the accordion displayed after loading the page with the "Movies" section open and the "Fantasy" link highlighted
 * with a class "current".
 *
 * @example jQuery('#accordion').Accordion().change(function(event, newHeader, oldHeader, newContent, oldContent) {
 *   jQuery('#status').html(newHeader.text());
 * });
 * @desc Updates the element with id status with the text of the selected header every time the accordion changes
 *
 * @param Map options key/value pairs of optional settings.
 * @option String|Element|jQuery|Boolean|Number active Selector for the active element. Set to false to display none at start. Default: first child
 * @option String|Element|jQuery header Selector for the header element, eg. 'div.title', 'a.head'. Default: first child's tagname
 * @option String|Number speed 
 * @option String selectedClass Class for active header elements. Default: 'selected'
 * @option Boolean alwaysOpen Whether there must be one content element open. Default: true
 * @option Boolean|String animated Choose your favorite animation, or disable them (set to false). In addition to the default, "bounceslide" and "easeslide" are supported (both require the easing plugin). Default: 'slide'
 * @option String event The event on which to trigger the accordion, eg. "mouseover". Default: "click"
 * @option Boolean navigation If set, looks for the anchor that matches location.href and activates it. Great for href-based pseudo-state-saving. Default: false
 * @option Boolean autoheight If set, the highest content part is used as height reference for all other parts. Provides more consistent animations. Default: false
 *
 * @type jQuery
 * @see activate(Number)
 * @name Accordion
 * @cat Plugins/Accordion
 */

/**
 * Activate a content part of the Accordion programmatically.
 *
 * The index can be a zero-indexed number to match the position of the header to close
 * or a string expression matching an element. Pass -1 to close all (only possible with alwaysOpen:false).
 *
 * @example jQuery('#accordion').activate(1);
 * @desc Activate the second content of the Accordion contained in <div id="accordion">.
 *
 * @example jQuery('#accordion').activate("a:first");
 * @desc Activate the first element matching the given expression.
 *
 * @example jQuery('#nav').activate(false);
 * @desc Close all content parts of the accordion.
 *
 * @param String|Element|jQuery|Boolean|Number index An Integer specifying the zero-based index of the content to be
 *				 activated or an expression specifying the element, or an element/jQuery object, or a boolean false to close all.
 *
 * @type jQuery
 * @name activate
 * @cat Plugins/Accordion
 */

(function($) {

$.Accordion = {};
$.extend($.Accordion, {
	defaults: {
		selectedClass: "selected",
		alwaysOpen: true,
		animated: 'slide',
		event: "click"
	},
	Animations: {
		slide: function(settings, additions) {
			settings = $.extend({
				easing: "swing",
				duration: 300
			}, settings, additions);
			if ( !settings.toHide.size() ) {
				settings.toShow.animate({height: "show"}, {
					duration: settings.duration,
					easing: settings.easing,
					complete: settings.finished
				});
				return;
			}
			var height = settings.toHide.height();
			settings.toShow.css({ height: 0, overflow: 'hidden' }).show();
			settings.toHide.filter(":hidden").each(settings.finished).end().filter(":visible").animate({height:"hide"},{
				step: function(n){
					settings.toShow.height(Math.ceil(height - ($.fn.stop ? n * height : n)));
				},
				duration: settings.duration,
				easing: settings.easing,
				complete: settings.finished
			});
		},
		bounceslide: function(settings) {
			this.slide(settings, {
				easing: settings.down ? "bounceout" : "swing",
				duration: settings.down ? 1000 : 200
			});
		},
		easeslide: function(settings) {
			this.slide(settings, {
				easing: "easeinout",
				duration: 700
			})
		}
	}
});

$.fn.extend({
	nextUntil: function(expr) {
	    var match = [];
	
	    // We need to figure out which elements to push onto the array
	    this.each(function(){
	        // Traverse through the sibling nodes
	        for( var i = this.nextSibling; i; i = i.nextSibling ) {
	            // Make sure that we're only dealing with elements
	            if ( i.nodeType != 1 ) continue;
	
	            // If we find a match then we need to stop
	            if ( $.filter( expr, [i] ).r.length ) break;
	
	            // Otherwise, add it on to the stack
	            match.push( i );
	        }
	    });
	
	    return this.pushStack( match );
	},
	// the plugin method itself
	Accordion: function(settings) {
		if ( !this.length )
			return this;
	
		// setup configuration
		settings = $.extend({}, $.Accordion.defaults, {
			// define context defaults
			header: $(':first-child', this)[0].tagName // take first childs tagName as header
		}, settings);
		
		if ( settings.navigation ) {
			var current = this.find("a").filter(function() { return this.href == location.href; });
			if ( current.length ) {
				if ( current.filter(settings.header).length ) {
					settings.active = current;
				} else {
					settings.active = current.parent().parent().prev();
					current.addClass("current");
				}
			}
		}
		
		// calculate active if not specified, using the first header
		var container = this,
			headers = container.find(settings.header),
			active = findActive(settings.active),
			running = 0;

		if ( settings.autoheight ) {
			var maxHeight = 0;
			headers.nextUntil(settings.header).each(function() {
				maxHeight = Math.max(maxHeight, $(this).height());
			}).height(maxHeight);
		}

		headers
			.not(active || "")
			.nextUntil(settings.header)
			.hide();
		active.addClass(settings.selectedClass);
		
		
		function findActive(selector) {
			return selector != undefined
				? typeof selector == "number"
					? headers.eq(selector)
					: headers.not(headers.not(selector))
				: selector === false
					? $("<div>")
					: headers.eq(0)
		}
		
		function toggle(toShow, toHide, data, clickedActive, down) {
			var finished = function(cancel) {
				running = cancel ? 0 : --running;
				if ( running )
					return;
				// trigger custom change event
				container.trigger("change", data);
			};
			
			// count elements to animate
			running = toHide.size() == 0 ? toShow.size() : toHide.size();
			
			if ( settings.animated ) {
				if ( !settings.alwaysOpen && clickedActive ) {
					toShow.slideToggle(settings.animated);
					finished(true);
				} else {
					$.Accordion.Animations[settings.animated]({
						toShow: toShow,
						toHide: toHide,
						finished: finished,
						down: down
					});
				}
			} else {
				if ( !settings.alwaysOpen && clickedActive ) {
					toShow.toggle();
				} else {
					toHide.hide();
					toShow.show();
				}
				finished(true);
			}
		}
		
		function clickHandler(event) {
			// called only when using activate(false) to close all parts programmatically
			if ( !event.target && !settings.alwaysOpen ) {
				active.toggleClass(settings.selectedClass);
				var toHide = active.nextUntil(settings.header);
				var toShow = active = $([]);
				toggle( toShow, toHide );
				return;
			}
			// get the click target
			var clicked = $(event.target);
			
			// due to the event delegation model, we have to check if one
			// of the parent elements is our actual header, and find that
			if ( clicked.parents(settings.header).length )
				while ( !clicked.is(settings.header) )
					clicked = clicked.parent();
			
			var clickedActive = clicked[0] == active[0];
			
			// if animations are still active, or the active header is the target, ignore click
			if(running || (settings.alwaysOpen && clickedActive) || !clicked.is(settings.header))
				return;

			// switch classes
			active.toggleClass(settings.selectedClass);
			if ( !clickedActive ) {
				clicked.addClass(settings.selectedClass);
			}

			// find elements to show and hide
			var toShow = clicked.nextUntil(settings.header),
				toHide = active.nextUntil(settings.header),
				data = [clicked, active, toShow, toHide],
				down = headers.index( active[0] ) > headers.index( clicked[0] );
			
			active = clickedActive ? $([]) : clicked;
			toggle( toShow, toHide, data, clickedActive, down );

			return !toShow.length;
		};
		function activateHandler(event, index) {
			// IE manages to call activateHandler on normal clicks
			if ( arguments.length == 1 )
				return;
			// call clickHandler with custom event
			clickHandler({
				target: findActive(index)[0]
			});
		};

		return container
			.bind(settings.event, clickHandler)
			.bind("activate", activateHandler);
	},
	activate: function(index) {
		return this.trigger('activate', [index]);
	}
});

})(jQuery);