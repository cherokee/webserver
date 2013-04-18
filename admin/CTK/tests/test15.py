# -*- coding: utf-8 -*-

import CTK

def default():
    carousel = CTK.Carousel()
    carousel += CTK.Image ({'src': "http://www.fancydressfast.com/images/R881051.jpg"})
    carousel += CTK.Image ({'src': "http://2.bp.blogspot.com/_j1nA6ZV_0EU/SoGvD5XbyaI/AAAAAAAAAbE/bjD-vBJY6Eo/s400/cherokee_nectarine.jpg"})
    carousel += CTK.Image ({'src': "http://rlv.zcache.com/cherokee_indians_middle_springfield_missouri_tshirt-p235011080071166412yboh_400.jpg"})
    carousel += CTK.Image ({'src': "http://img1.tradeget.com/globalpackersmovers/1D5JQV6B1car_transportation.jpg"})

    page = CTK.Page()
    page += CTK.RawHTML('<h1>Demo</h1>')
    page += carousel
    page += CTK.RawHTML('FIN')
    return page.Render()

CTK.publish ('', default)
CTK.run (port=8000)
