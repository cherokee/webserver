function get_by_id (id)
{
    if(document.layers)
	  return document.layers[id];

    if(document.getElementById)
        return document.getElementById(id);

    if(document.all)
	   return document.all[id];
}

function get_by_id_and_class (id, klass)
{
	   var ret;

        $(klass).each(function(i) {
		  if (this.id == id) {
				ret = this;
				return false;
            }
	   });

	   return ret;
}

function make_visible (DivID, visible)
{
    if(document.layers) {
	  get_by_id(DivID).visibility = visible ? "show" : "hide";
    } else {
       get_by_id(DivID).style.visibility = visible ? "visible" : "hidden";
    }
}

function comment_out (DivID, visible)
{
    var o = get_by_id(DivID);
    if (o == null) return;

    if (visible) {
		  o.style.display = 'block';
    } else {
		  o.style.display = 'none';
    }
}

function comment_out_class (id, klass, visible)
{
    var o = get_by_id_and_class (id, klass);
    if (o == null) return;

    if (visible) {
		  o.style.display = 'block';
    } else {
		  o.style.display = 'none';
    }
}


function options_active_prop (options_id, props_prefix)
{
	   var thisselect = get_by_id_and_class (options_id, 'select');
	   var options    = thisselect.options;
	   var selected   = options[options.selectedIndex].value;

	   /* Show the righ entry */
	   for (i=0; i<options.length; i++) {
			 var i_str = props_prefix + '_' + options[i].value;
			 var s_str = props_prefix + '_' + selected;

			 if (i_str == s_str) {
				    comment_out_class (i_str, 'div', 1);
			 } else {
				    comment_out_class (i_str, 'div', 0);
			 }
	   }
}


function options_changed (url, obj)
{
	var selected = jQuery(obj).val();

	/* POST the new value and reload */
	var post = obj.name + "=" + selected;

	jQuery.post (url, post,
		function (data, textStatus) {
			window.location = window.location;
		}
	);
}

function switch_changed (url, obj)
{
	var value = jQuery(obj).attr("checked");

	if (value == 'on' || value == '1' || value == 'true') {
		value = '1';
	} else {
		value = '0';
	}

	/* POST the new value and reload */
	var post = obj.name + "=" + value;

	jQuery.post (url, post);
}

function save_config()
{
	var post = "restart=" + $("#restart").val();

	jQuery.post("/apply_ajax", post,
		function (data, textStatus) {
			$("#save_changes_msg").html(data);
			$("#save_changes_msg").fadeIn("slow");
			setTimeout('$("#save_changes_msg").fadeOut("slow")', 5000);
		}
	);
}

function toggle_help()
{
    if ($("#help-contents").is(":hidden")) {
        $("#help").html('Close');
        $("#help-contents").fadeIn("fast");
    } else {
        $("#help").html('Help');
        $("#help-contents").fadeOut("fast", function() { $("#help").fadeIn() });
    }
}


function post_del_key (url, cfg_key)
{
	   var post = cfg_key + "=";

	   jQuery.post (url, post, 
              function (data, textStatus) {
			   window.location = window.location;
              }
	   );
}

function post_add_entry_key (url, entry_name, cfg_key)
{
	   var obj  = get_by_id(entry_name);
	   var post = cfg_key + "=" + obj.value;

	   jQuery.post (url, post, 
	         function (data, textStatus) {
			   window.location = window.location;
              }
        );
}


function get_cookie (key)
{
  var i = document.cookie.indexOf (key+'=');
  if (i < 0) return;

  i += key.length + 1;

  var e = document.cookie.indexOf(';', i);
  if (e < 0) e = document.cookie.length;

  return unescape (document.cookie.substring (i, e));
}

function do_autosubmit(obj)
{
	if (check_all_or_none('required')) {
		setConfirmUnload(false);
		$(obj.form).submit();
	}
}

/* Auto submission of some forms */
function autosubmit() {
	$(".auto input:not(.noautosubmit), select:not(.noautosubmit)").change(function() {
		do_autosubmit(this);
	});
}

/* Returns true either if every field of
   class=klass is set, or none of them is
*/
function check_all_or_none (klass)
{
  var none=true;
  var valid=true;
  $('.'+klass).each(function(i, o){
    if (o.value.length > 0)
      none=false;
    else  valid=false;
  });
  return (none || valid);
}

/* Prevent accidentally navigating away */
function protectChanges()
{
	//alert(document.location.host.toString());

  $('a').click(function() {
    setConfirmUnload(false);
  });

  $('img').click(function() {
    setConfirmUnload(false);
  });

  $('select').change(function() {
    setConfirmUnload(false);
  });

  $('form').submit(function() {
    setConfirmUnload(false);
  });

  $('.rulestable').bind("mouseup", function() {
    setConfirmUnload(false);
  });

  $('form').bind("change submit", function() {
    document.cookie = "changed=true";
  });

  changed = get_cookie('changed');
  if (changed) {
    setConfirmUnload(true);
  }
}

function setConfirmUnload(on)
{
  window.onbeforeunload = (on) ? unloadMessage : null;
}

function unloadMessage()
{
  return 'Settings were modified but not saved.\n'+
         'They will be lost if you leave Cherokee-Admin.';
}


/* RULES
 */

function rule_do_not (prefix)
{
    post = 'prefix='+prefix;
    jQuery.post("/rule/not", post,
		function (data, textStatus) {
		    setConfirmUnload(false);
		    window.location = window.location;
		}
    );
}

function rule_do_and (prefix)
{
    post = 'prefix='+prefix;
    jQuery.post("/rule/and", post,
		function (data, textStatus) {
		    setConfirmUnload(false);
		    window.location = window.location;
		}
    );
}

function rule_do_or (prefix)
{
    post = 'prefix='+prefix;
    jQuery.post("/rule/or", post,
		function (data, textStatus) {
		    setConfirmUnload(false);
		    window.location = window.location;
		}
    );
}

function rule_delete (prefix)
{
    post = 'prefix='+prefix;
    jQuery.post("/rule/del", post,
		function (data, textStatus) {
		    setConfirmUnload(false);
		    window.location = window.location;
		}
    );
}

function openSection(cookie_name, section)
{
	document.cookie = cookie_name + "=" + section;
	if (prevSection != '') {
		$("#"+prevSection).hide();
		$("#"+prevSection+"_b").attr("style", "font-weight: normal;");
	}

	$("#"+section+"_b").attr("style", "font-weight: bold;");
	$("#"+section).show();
	prevSection = section;
}

function PageVServer_disable_rule (obj)
{
	checked = $(obj).is(":checked");
	$(obj)
		.closest('tr')
		.toggleClass("trdisabled", checked)
		.children('td:not(.switch)')
		.toggleClass("rulesdisabled", checked);
}

/* INITIALIZATION FUNCTIONS
 */

/* Common initialization code for PageVServer and PageVServers
 */
function init_PageVServer_common (table_name, cookie_name)
{
	$("table.rulestable tr:odd").addClass("odd");
	$(document).mouseup(function(){
		$("table.rulestable tr:even").removeClass("odd");
		$("table.rulestable tr:odd").addClass("odd");
	});

	$("#" + table_name + " tr:not(.nodrag, nodrop)").hover(
		function() {
			$(this.cells[0]).addClass('dragHandleH');
		},
		function() {
			$(this.cells[0]).removeClass('dragHandleH');
		}
	);

	$("#" + table_name + " input:checkbox.switch").iButton("invert").change(function() {
		switch_changed('/ajax/update', this);
		PageVServer_disable_rule(this);
	});

	$("tr.trdisabled td:not(.switch)").addClass("rulesdisabled");

	prevSection = '';
	$("#newsection_b").click(function() { openSection(cookie_name, 'newsection')});
	$("#clonesection_b").click(function() { openSection(cookie_name, 'clonesection')});
	$("#wizardsection_b").click(function() { openSection(cookie_name, 'wizardsection')});

	open_section  = get_cookie(cookie_name);
	if (open_section && document.referrer == window.location) {
		openSection(cookie_name, open_section);
	}
}

function init_PageVServers (table_name, ajax_url)
{
	init_PageVServer_common(table_name, 'open_vssec');

	$('#' + table_name).tableDnD({
		onDrop: function(table, row) {
			var rows = table.tBodies[0].rows;
			var post = 'update_prio=1&prios=';
			for (var i=1; i<rows.length; i++) {
				post += rows[i].id + ',';
			}

			jQuery.post (ajax_url, post,
				function (data, textStatus) {
					window.location.reload();
				}
			);
		},
		dragHandle: "dragHandle"
	});
}

function init_PageVServer (table_name, ajax_url, prefix)
{
	init_PageVServer_common(table_name, 'open_vsec');

	$('#' + table_name).tableDnD({
		onDrop: function(table, row) {
			var rows = table.tBodies[0].rows;
			var post = 'update_prefix=' + prefix + '&';
			for (var i=1; i<rows.length; i++) {
				var prio = (rows.length - i) * 100;
				post += 'update_prio=' + rows[i].id + ',' + prio + '&';
			}

			jQuery.post (ajax_url, post,
				function (data, textStatus) {
					window.location = window.location;
				}
			);
		},
		dragHandle: "dragHandle"
	});
}