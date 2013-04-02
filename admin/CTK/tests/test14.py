# -*- coding: utf-8 -*-

import CTK

def commit():
    print CTK.post
    return {'ret': 'ok'}

def default():
    submit = CTK.Submitter('/commit')

    submit += CTK.RawHTML ("<h2>Can set, without initial value</h2>")
    submit += CTK.StarRating ({'name': 'test_rate1', 'can_set': True})

    submit += CTK.RawHTML ("<h2>Can set, with initial value</h2>")
    submit += CTK.StarRating ({'name': 'test_rate2', 'selected': '3', 'can_set': True})

    submit += CTK.RawHTML ("<h2>Cannot edit value</h2>")
    submit += CTK.StarRating ({'name': 'test_rate3', 'selected': '4'})

    submit += CTK.RawHTML ("<h2>No auto-submit</h2>")
    submit += CTK.StarRating ({'name': 'test_rate4', 'can_set': True, 'class': 'noauto'})

    page = CTK.Page()
    page += CTK.RawHTML('<h1>Demo StarRating</h1>')
    page += submit
    return page.Render()


CTK.publish ('', default)
CTK.publish ('/commit', commit, method="POST")
CTK.run (port=8000)
