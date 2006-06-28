/* -*- Mode: c; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Copyright (C) 2001-2006 Alvaro Lopez Ortega <alvaro@alobbs.com>
 */

function vsrv_label_clicked (event, name) 
{
	document.forms.vsrvs_hidden_form.vserver.value = name;
	document.forms.vsrvs_hidden_form.submit();
}
