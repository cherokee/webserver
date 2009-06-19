var wizCur = '';
var wizwCur = '';

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
                    $(this).addClass("wizSelected");
                    if (wizCur != '') {
                        $("#" + wizCur).removeClass("wizSelected");
                    }
                    wizCur = $(this).attr("id");
                    wizwCur = '';
                    getWizards(event.data.w);
                })
            .appendTo("#wizGroup");
        x++;
    }
}

function getWizards(w)
{
    $("#wizL").empty();
    $("#wizIinner").empty();

    $("<ul/>")
        .attr("id", "wizList")
        .appendTo("#wizL");


    x = 1;
    for (var i in cherokeeWizards[w]) {
        if (x % 2 == 0) {
            odd = "";
        } else {
            odd = "wizOdd";
        }
        $("<li/>")
            .html(i)
            .attr("id", "wizw" + x)
            .addClass(odd)
            .attr("value", i)
            .bind(
                'click',
                { "g": w, "w": i },
                function (event) { 
                    $(this).addClass("wizSelected");
                    if (wizwCur != '') {
                        $("#" + wizwCur).removeClass("wizSelected");
                    }
                    wizwCur = $(this).attr("id");
                    getWizInfo(event.data.g, event.data.w);
                })
            .appendTo("#wizList");
        x++;
    }

}

function getWizInfo(g,w)
{
    $("#wizIinner").empty();
    i = cherokeeWizards[g][w];
    $("<img/>")
        .attr("src", i.img)
        .attr("id", "wizImg")
        .appendTo("#wizIinner");

    $("<div/>")
        .attr("id", "wizTitle")
        .html(i.title)
        .appendTo("#wizIinner");

    $("<div/>")
        .attr("id", "wizDesc")
        .html(i.desc)
        .appendTo("#wizIinner");

    if (i.show) {
        $('<div id="wizBut"><a class="button" href="'+i.url+'"><span>Run Wizard</span></a></div>').appendTo("#wizIinner");
    } else {
        $('<div/>')
            .addClass('dialog-warning')
            .html(i.no_show)
            .appendTo("#wizIinner");
    }
}
