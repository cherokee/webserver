# -*- coding: utf-8 -*-

import CTK

def default():
    page = CTK.Page()
    page += CTK.RadioText("option 1", {'name': 'test16', 'value': 'a'})
    page += CTK.RadioText("option 2", {'name': 'test16', 'value': 'b'})
    page += CTK.RadioText("option 3", {'name': 'test16', 'value': 'c'})
    return page.Render()

CTK.publish ('', default)
CTK.run (port=8000)
