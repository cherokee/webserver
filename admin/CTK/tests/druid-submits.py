# -*- coding: utf-8 -*-

import CTK


def submit_ok():
    return CTK.cfg_reply_ajax_ok()

def submit_fail():
    if CTK.post.get_val('3') != '0':
        return {'ret': 'fail', 'errors': {'3': 'Must be 0'}}
    return CTK.cfg_reply_ajax_ok()

def fin():
    box = CTK.Box()
    box += CTK.RawHTML('<h1>FIN</h1>')
    return box.Render().toStr()

def content():
    box = CTK.Box()

    # Submit 1
    submit = CTK.Submitter ('/submit_ok')
    submit += CTK.TextField ({'name': '1', 'value': '1'})
    box += CTK.RawHTML("<h2>Submit 1</h2>")
    box += submit

    # Submit 2
    submit = CTK.Submitter ('/submit_ok')
    submit += CTK.TextField ({'name': '2', 'value': '2'})
    box += CTK.RawHTML("<h2>Submit 2</h2>")
    box += submit

    # Submit 3
    submit = CTK.Submitter ('/submit_fail')
    submit += CTK.TextField ({'name': '3', 'value': '3'})
    box += CTK.RawHTML("<h2>Submit 3</h2>")
    box += submit

    buttons = CTK.DruidButtonsPanel()
    buttons += CTK.DruidButton_Goto (_('Next'), '/fin', do_submit=True)
    box += buttons

    return box.Render().toStr()

def main():
    page = CTK.Page()

    refresh = CTK.RefreshableURL ('/content', {'id': "test-refresh"})
    druid   = CTK.Druid (refresh)

    dialog = CTK.Dialog({'title': "Test", 'autoOpen': True})
    dialog += druid

    page += dialog
    return page.Render()


CTK.publish ('',             main)
CTK.publish ('/fin',         fin)
CTK.publish ('/content',     content)
CTK.publish ('/submit_ok',   submit_ok,   method="POST")
CTK.publish ('/submit_fail', submit_fail, method="POST")

CTK.run (port=8000)
