import CTK

name = "whatever"

def submit():
    global name
    name = CTK.post['field']
    if len(name) < 10:
        return {'ret': 'error', 'errors': {'field': 'Too short'}}
    return {'ret': 'ok'}


def url3():
    cont = CTK.Container()
    cont += CTK.RawHTML ('Thank you, the value is <b>%s</b>.'%(name))
    cont += CTK.DruidButtonsPanel_PrevCreate ('/url2')
    return cont.Render().toStr()


def url2():
    table = CTK.PropsTable()
    table.Add ("Field", CTK.TextField({'name': 'field', 'value': name}), "Type whatever you want in here")
    submit = CTK.Submitter ('/apply')
    submit += table

    cont = CTK.Container()
    cont += CTK.RawHTML ('Please, fill out the following form:')
    cont += submit
    cont += CTK.DruidButtonsPanel_PrevNext ('/url1', '/url3')

    return cont.Render().toStr()


def url1():
    cont = CTK.Container()
    cont += CTK.RawHTML ('Welcome, this is a test..')
    cont += CTK.DruidButtonsPanel_Next('/url2')
    return cont.Render().toStr()


class default:
    def __call__ (self):
        dialog = CTK.Dialog ({'title': "New Virtual Server", 'width': 500, 'autoOpen': True})
        dialog += CTK.Druid (CTK.RefreshableURL ('/url1'))

        page = CTK.Page()
        page += dialog
        return page.Render()


CTK.publish ('', default)
CTK.publish ('/url1', url1)
CTK.publish ('/url2', url2)
CTK.publish ('/url3', url3)
CTK.publish ('/apply', submit, method="POST")
CTK.run (port=8000)
