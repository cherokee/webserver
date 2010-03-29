/* CTK'fied version
 */

jQuery.fn.DefaultValue = function(klass, text){
	return this.each(function(){
		//Make sure we're dealing with text-based form fields
		if (this.type != 'text' && this.type != 'password' && this.type != 'textarea')
			return;

		//Store field reference
		var fld_current = this;

		//Set value initially if none are specified
		if($(this).val() == '') {
			$(this).val(text);
		} else {
			//Other value exists - ignore
			return;
		}

		//Remove values on focus
		$(this).focus(function() {
			if($(this).val() == text || $(this).val() == '') {
				$(this).removeClass(klass);
				$(this).val('');
			}
		});

		//Place values back on blur
		$(this).blur(function() {
			if($(this).val() == text || $(this).val() == '') {
				$(this).addClass(klass);
				$(this).val(text);
			}
		});
    });
};