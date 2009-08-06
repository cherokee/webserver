var graphPrefix   = 'server';
var graphType     = 'traffic';
var graphVServer  = null;
var graphInterval = '1h';
var refInterval   = 60000;

function graphChangeType(type, title) {
    $('#gmtop').html("<span>" + title + "</span>");
    graphType = type;
    changeGraph(false);
}

function graphChangeInterval(i) {
    $("#g"+graphInterval).removeClass("gsel");
    $("#g"+i).addClass("gsel");
    graphInterval = i;
    changeGraph(true);
}

function changeGraph(flash)
{
    if (graphPrefix == 'vserver') {
	   imgurl = '/graphs/' + graphPrefix + '_' + graphType + '_' + graphVServer + '_' + graphInterval + '.png';
    } else {
	   imgurl = '/graphs/' + graphPrefix + '_' + graphType + '_' + graphInterval + '.png';
    }

    if (flash) {
	   $('#graphimg').fadeOut("slow", function() {
		  $('#graphimg').attr('src', imgurl).fadeIn("slow");
	   });
    } else {
	   $('#graphimg').attr('src', imgurl);
    }
}

function refreshGraph()
{
    setInterval(function(){ changeGraph(false); }, refInterval);
}
