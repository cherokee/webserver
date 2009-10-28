var wizCur = '';

function wizardRender() 
{
    $("<ul/>")
        .attr("id", "wizGroup")
        .appendTo("#wizG");

    x = 1;

    for (var i in cherokeeWizards) {
        if (x % 2 == 0) {
            odd = "";
        } else {
            odd = "wizOdd";
        }
        $("<li/>")
            .html(i)
            .attr("id", "wiz" + x)
            .addClass(odd)
            .attr("value", i)
            .bind(
                'click',
                { "w": i },
                function (event) { 
                    if (wizCur != '') {
                        $("#" + wizCur).removeClass("wizSelected");
                    }
                    $(this).addClass("wizSelected");
                    wizCur = $(this).attr("id");
                    getWizards(event.data.w);
                })
            .appendTo("#wizGroup");
        x++;
    }

    $("#wizL").empty();
    $("<div/>")
	.attr("id", "wizSelCat")
        .html("Select a category")
        .appendTo("#wizL");

}

function getWizards(c)
{
    $("#wizL").empty();

    x = 1;
    for (var i in cherokeeWizards[c]) {
	w = cherokeeWizards[c][i];

        $("<div/>")
            .attr("id", "wizw" + x)
            .addClass("wizInfo")
            .attr("style", "background-image:url('"+w.img+"');")
            .appendTo("#wizL");

        $("<div/>")
            .addClass("wizTitle")
            .html(w.title)
            .appendTo("#wizw"+x);

        $("<div/>")
            .addClass("wizDesc")
            .html(w.desc)
            .appendTo("#wizw"+x);

        if (w.show) {
            $('<div class="wizBut"><a href="'+w.url+'">Run Wizard</a></div>').appendTo("#wizw"+x);
        } else {
            $("<div/>")
                .addClass("wizBut")
                .attr("id", "wizBut"+x)
                .appendTo("#wizw"+x);

            $("<a/>")
                .attr("href", "#")
                .html("Run Wizard")
   		      .bind('click', {"reason" : w.no_show}, function(event) {
				alert (event.data.reason);
			 })
                .appendTo("#wizBut"+x);
        }


        x++;
    }

}
