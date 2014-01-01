# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2009-2014 Alvaro Lopez Ortega
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of version 2 of the GNU General Public
# License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#

from Table import Table
from Server import get_server
from PageCleaner import Uniq_Block
from Server import publish, post, cfg, cfg_reply_ajax_ok

HEADER = [
    '<script type="text/javascript" src="/CTK/js/jquery.tablednd_0_5.js"></script>'
]

JS_INIT = """
$("#%(id)s").tableDnD({
   onDrop: function (table, row) {
       $.ajax ({url:      "%(url)s",
                async:    true,
                type:     "POST",
                dataType: "text",
                data:     $.tableDnD.serialize_plain(),
                success:   function (data_raw) {
                  var data = eval('(' + data_raw + ')');

                  if (data['ret'] == 'ok') {
                    /* Modified: Save button */
                    var modified     = data['modified'];
                    var not_modified = data['not-modified'];

                    if (modified != undefined) {
                       $(modified).show();
                       $(modified).removeClass('saved');
                    } else if (not_modified) {
                       $(not_modified).addClass('saved');
                    }
                  }

                  /* Signal */
                  $("#%(id)s").trigger ('reordered');
                }
              });
   },
   dragHandle: "dragHandle",
   containerDiv: $("#%(container)s")
});
"""

JS_INIT_FUNC = """
jQuery.tableDnD.serialize_plain = function() {
     var result = "";
     var table  = jQuery.tableDnD.currentTable;

     /* Serialize */
     for (var i=0; i<table.rows.length; i++) {
         if (result.length > 0) {
              result += ",";
         }
         result += table.rows[i].id;
     }
     return table.id + "_order=" + result;
};
"""

def changed_handler_func (callback, key_id, **kwargs):
    return callback (key_id, **kwargs)

class SortableList (Table):
    def __init__ (self, callback, container, *args, **kwargs):
        Table.__init__ (self, *args, **kwargs)
        self.id  = "sortablelist_%d" %(self.uniq_id)
        self.url = "/sortablelist_%d"%(self.uniq_id)
        self.container = container

        # Secure submit
        srv = get_server()
        if srv.use_sec_submit:
            self.url += '?key=%s' %(srv.sec_submit)

        # Register the public URL
        publish (r"^/sortablelist_%d"%(self.uniq_id), changed_handler_func,
                 method='POST', callback=callback, key_id='%s_order'%(self.id), **kwargs)

    def Render (self):
        render = Table.Render (self)

        props = {'id': self.id, 'url': self.url, 'container': self.container}

        render.js      += JS_INIT %(props) + Uniq_Block (JS_INIT_FUNC %(props))
        render.headers += HEADER
        return render

    def set_header (self, row_num):
        # Set the row properties
        self[row_num].props['class'] = "nodrag nodrop"

        # Let Table do the rest
        Table.set_header (self, num=row_num)


def SortableList__reorder_generic (post_arg_name, pre, step=10):
    # Process new list
    order = post.pop (post_arg_name)
    tmp = order.split(',')

    # Build and alternative tree
    num = step
    for v in tmp:
        cfg.clone ('%s!%s'%(pre, v), 'tmp!reorder!%s!%d'%(pre, num))
        num += step

    # Set the new list in place
    del (cfg[pre])
    cfg.rename ('tmp!reorder!%s'%(pre), pre)

    return cfg_reply_ajax_ok()
