/* -*- Mode: c; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Copyright (C) 2001-2006 Alvaro Lopez Ortega <alvaro@alobbs.com>
 */

var id_being_edited   = "";
var conf_being_edited = "";

function Trim (sString) {
        while (sString.substring(0,1) == " ")
			 sString = sString.substring(1, sString.length);
	   while (sString.substring(sString.length-1, sString.length) == " ")
			 sString = sString.substring(0,sString.length-1);
	   return sString;
}

property_text_update_and_close_ok = function (resp) {
	   if (resp.responseText != "ok\r\n") {
			 return property_update_and_close_failed (resp);
	   }

	   var div_area  = document.getElementById (id_being_edited);
	   var text_area = document.getElementById (id_being_edited + "_entry");
	   var value     = text_area.value;
	   var params    = window[id_being_edited+"_params"];

	   div_area.innerHTML = value;
	   YAHOO.util.Event.addListener(id_being_edited, "click", property_text_clicked, params);

	   id_being_edited   = "";
	   conf_being_edited = "";
}

property_bool_update_and_close_ok = function (resp) {
	   ;
}

property_update_and_close_failed = function (resp) {
	   alert ("failed: " + resp.responseText);
}			

var property_text_check_cb = {
        success: property_text_update_and_close_ok, 
	   failure: property_update_and_close_failed
};

var property_bool_check_cb = {
        success: property_bool_update_and_close_ok, 
	   failure: property_update_and_close_failed
};

property_text_update_and_close = function () {
	   var div_area  = document.getElementById (id_being_edited);
	   var text_area = document.getElementById (id_being_edited + "_entry");
	   var value     = text_area.value;
	   
	   var post_data = "ajax=1&prop="+id_being_edited+"&value="+value+"&conf="+conf_being_edited;
	   YAHOO.util.Connect.asyncRequest ("POST", document.location.href, property_text_check_cb, post_data);

	   return false;
}

property_bool_clicked = function (event, params) {
	   var type      = params[0];
	   var conf      = params[1];
	   var fname     = params[2];
	   var checkbox  = document.getElementById (fname + "_entry");
	   var value     = checkbox.checked;
	   
	   var post_data = "ajax=1&prop="+fname+"&value="+value+"&conf="+conf;
	   YAHOO.util.Connect.asyncRequest ("POST", document.location.href, property_bool_check_cb, post_data);
	   
	   return true;
}
	   
property_text_clicked = function (event, params) {
	   if (id_being_edited.length != 0) {
		   return false;
	   }

	   var target = YAHOO.util.Event.getTarget (event, true);
	   var obj    = document.getElementById (target.id);
	   var old    = Trim(obj.innerHTML);
	   var type   = params[0];
	   var conf   = params[1];
	   
	   YAHOO.util.Event.preventDefault (event);
	   YAHOO.util.Event.removeListener (target.id, "click", property_text_clicked);
	   
	   conf_being_edited = conf;
	   id_being_edited   = target.id;
	   
	   obj.innerHTML = '<form onsubmit="return property_text_update_and_close();">                  \
	   	<input type="textarea" id="'+target.id+'_entry" value="'+old+'" name="'+target.id+'" /> \
		<input type="submit" value="OK" />	                                   		\
	   </form>';
}

