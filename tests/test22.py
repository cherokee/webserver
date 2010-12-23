# -*- coding: utf-8 -*-

import CTK

nums = ['uno', 'dos', 'tres', 'cuatro']

class default:
    def __call__ (self):
        page = CTK.Page()

        # 4 Tabs
        for n in range(1,5):
            tab_id = 'tab_' + nums[n-1]
            tab = CTK.Tab({'id': tab_id})
            page += tab

            # Number of entries in it
            for tab_n in range (1,n+1):
                tab.Add ('%s - %s'%(tab_id, tab_n), CTK.RawHTML ())

        return page.Render()

CTK.publish ('', default)
CTK.run (port=8000)
