var graphPrefix   = 'server';
var graphType     = 'traffic';
var graphVServer  = null;
var graphInterval = '1h';
var refInterval   = 60000;

function graphChangeType(type, title) {
    graphType = type;
    $('#gmtop').html("<span>" + title + "</span>");
    refreshGraph();
}

function graphChangeInterval(i) {
    $("#g"+graphInterval).removeClass("gsel");
    $("#g"+i).addClass("gsel");
    graphInterval = i;
    changeGraph();
}

function changeGraph()
{
    if (graphPrefix == 'vserver') {
	   imgurl = '/graphs/' + graphPrefix + '_' + graphType + '_' + graphVServer + '_' + graphInterval + '.png';
    } else {
	   imgurl = '/graphs/' + graphPrefix + '_' + graphType + '_' + graphInterval + '.png';
    }
    $('#graphimg').attr('src', imgurl);
}

function refreshGraph()
{
    changeGraph();
    setTimeout(refreshGraph, refInterval);
}
