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

HTML_JS_BLOCK = """
<script type="text/javascript">
%s
</script>
"""

JS_ON_READY = """
$(document).ready(function(){
  %s
});
"""

HTML_JS_ON_READY_BLOCK = HTML_JS_BLOCK %(JS_ON_READY)

LINK_JS_ON_CLICK = '<a href="#" OnClick="javascript:%s">%s</a>'
LINK_HREF        = '<a href="%s">%s</a>'
