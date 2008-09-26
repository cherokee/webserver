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


function options_changed (url, options_id, parent_id)
{
	   var thisform = $("#"+parent_id).children("#"+options_id)[0];
	   var options  = thisform.options;
	   var selected = options[options.selectedIndex].value;

	   /* POST the new value and reload */
	   var post = options_id + "=" + selected;

	   jQuery.post (url, post,
              function (data, textStatus) {
   			   window.location = window.location;
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
