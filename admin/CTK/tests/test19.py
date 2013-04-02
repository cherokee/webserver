# -*- coding: utf-8 -*-

import re
import CTK

URL_R1 = '/r1'
URL_R2 = '/r2'
URL_R3 = '/r3'

GOTO_JS = "$('#%s').trigger({'type': 'refresh_goto', 'goto': '%s'});"


class R1:
    ID = "r1-id"

    def __call__ (self):
        box = CTK.Box({'id': self.ID})

        for n in range(10):
            item = CTK.Box (content = CTK.RawHTML ('Box1: %s'%(n)))
            item.bind ('click', GOTO_JS %(R2.ID, URL_R2+'/%s'%(n)))
            box += item

        return box.Render().toStr()

class R2:
    ID = "r2-id"

    def __call__ (self):
        box = CTK.Box({'id': self.ID})

        tmp = re.findall('%s/(.*)'%(URL_R2), CTK.request.url)
        if tmp:
            b1 = tmp[0]
        else:
            b1 = '0'

        for n in "ABCDEFGHIJ":
            txt  = 'Box1 is %s, this %s'%(b1, n)
            item = CTK.Box (content = CTK.RawHTML (txt))
            item.bind ('click', GOTO_JS %(R3.ID, URL_R3+'/%s/%s'%(b1, n)))
            box += item

        return box.Render().toStr()

class R3:
    ID = "r3-id"

    def __call__ (self):
        box = CTK.Box({'id': self.ID})

        tmp = re.findall('%s/(.*)/(.*)'%(URL_R3), CTK.request.url)
        if tmp:
            b1,b2 = tmp[0]
        else:
            b1,b2 = '0','A'

        box += CTK.RawHTML('<h1>%s %s</h1>' %(b1, b2))
        return box.Render().toStr()


def main():
    r1 = CTK.RefreshableURL (URL_R1)
    r2 = CTK.RefreshableURL (URL_R2)
    r3 = CTK.RefreshableURL (URL_R3)

    page = CTK.Page()
    page += r1
    page += r2
    page += r3
    return page.Render()


CTK.publish (URL_R1, R1)
CTK.publish (URL_R2, R2)
CTK.publish (URL_R3, R3)
CTK.publish ('', main)
CTK.run (port=8000)
