function get_by_id (id)
{
    if(document.layers)
	  return document.layers[id];

    if(document.getElementById)
        return document.getElementById(id);

    if(document.all)
	   return document.all[id];
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

function options_active_prop (options_id, props_prefix)
{
	   var form     = get_by_id (options_id);	   
	   var options  = form.options;
	   var selected = options[options.selectedIndex].value;

	   for (i=0; i<options.length; i++) {
			 var i_str = props_prefix + options[i].value;
			 var s_str = props_prefix + selected;

			 if (i_str == s_str) {
				    comment_out (i_str, 1);
			 } else {
				    comment_out (i_str, 0);
			 }
	   }
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
                  window.location.reload();
              }
	   );
}

function post_add_entry_key (url, entry_name, cfg_key)
{
	   var obj  = document.getElementById(entry_name);
	   var post = cfg_key + "=" + obj.value;

	   jQuery.post (url, post, 
	         function (data, textStatus) {
                window.location.reload();
              }
        );
}