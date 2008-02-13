option_icons_update = function (cfg_key) {
	   option   = document.getElementById (cfg_key);
	   imagediv = document.getElementById ("image_"+cfg_key);
	   new_icon = option.options[option.selectedIndex].text; 

	   /* Update the icon image
	    */
	   imagediv.innerHTML = '<img src="/icons_local/'+new_icon+'" />';

	   /* Set the value
	    */
	   post = cfg_key+'='+new_icon;
	   jQuery.post("/icons/update", post);
}