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

function options_active_prop (options_id, props_prefix)
{
	   var form     = get_by_id (options_id);	   
	   var options  = form.options;
	   var selected = options[options.selectedIndex].value;

	   for (i=0; i<options.length; i++) {
			 var i_str = props_prefix + options[i].value;
			 var s_str = props_prefix + selected;

			 if (i_str == s_str) {
				    make_visible (i_str, 1);
			 } else {
				    make_visible (i_str, 0);
			 }
	   }
}