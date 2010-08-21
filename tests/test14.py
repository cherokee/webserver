# -*- coding: utf-8 -*-

import CTK

def commit():
    print CTK.post
    return {'ret': 'ok'}

def default():
    submit = CTK.Submitter('/commit')
    submit += CTK.RawHTML ("<h2>3rd selected</h2>")
    submit += CTK.StarRating ({'name': 'test_rate1', 'selected': '3'})
    submit += CTK.RawHTML ("<h2>None selected</h2>")
    submit += CTK.StarRating ({'name': 'test_rate2'})

    page = CTK.Page()
    page += CTK.RawHTML('<h1>Demo StarRating</h1>')
    page += submit
    return page.Render()


CTK.publish ('', default)
CTK.publish ('/commit', commit, method="POST")
CTK.run (port=8000)
