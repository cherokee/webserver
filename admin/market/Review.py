# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2011 Alvaro Lopez Ortega
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

import CTK
import OWS_Login

from consts import *
from ows_consts import *
from configured import *
from XMLServerDigest import XmlRpcServer

URL_REVIEW_FAIL  = '%s/fail'  %(URL_REVIEW)
URL_REVIEW_APPLY = '%s/apply' %(URL_REVIEW)


def Review_Apply():
    rate   = CTK.post.get_val('rate')
    title  = CTK.post.get_val('title')
    review = CTK.post.get_val('review')
    app_id = CTK.post.get_val('application_id')

    # OWS auth
    xmlrpc = XmlRpcServer(OWS_APPS_AUTH, OWS_Login.login_user, OWS_Login.login_password)
    try:
        ok = xmlrpc.set_review (app_id, rate,
                                CTK.util.to_unicode(title),
                                CTK.util.to_unicode(review))
    except:
        ok = False

    return {'ret': ('error','ok')[ok]}


class Review_Fail:
    def __call__ (self):
        cont = CTK.Container()
        cont += CTK.RawHTML ("<h1>%s</h1>"%(_('Could not send the review')))
        cont += CTK.RawHTML ("<p>%s</p>"%(_('We are sorry, but the review could not be sent.')))
        cont += CTK.RawHTML ("<p>%s</p>"%(_('Make sure you are logged in and there are no connectivity issues. If the problem persists, please contact our support team.')))

        buttons = CTK.DruidButtonsPanel()
        buttons += CTK.DruidButton_Close(_('Close'))
        cont += buttons

        return cont.Render().toStr()


class Review:
    def __call__ (self):
        application_id   = CTK.cfg.get_val('tmp!market!review!app_id')
        application_name = CTK.cfg.get_val('tmp!market!review!app_name')

        cont = CTK.Container()

        submit = CTK.Submitter (URL_REVIEW_APPLY)
        submit += CTK.Hidden ('application_id', application_id)
        submit += CTK.RawHTML (_("Please, describe your experience with %(application_name)s:")%(locals()))
        submit += CTK.Box ({'class': 'title-text'}, CTK.RawHTML (_('Title')))
        submit += CTK.TextField  ({'name': 'title',  'class': 'noauto druid-review-title'})
        submit += CTK.Box ({'class': 'review-text'}, CTK.RawHTML (_('Review')))
        submit += CTK.TextArea   ({'name': 'review', 'class': 'noauto druid-review-review'})
        submit += CTK.Box ({'class': 'rating-text'}, CTK.RawHTML (_('How do you rate %(application_name)s?')%(locals())))
        submit += CTK.StarRating ({'name': 'rate',   'class': 'noauto', 'can_set': True})
        submit.bind ('submit_fail', CTK.DruidContent__JS_to_goto (submit.id, URL_REVIEW_FAIL))
        cont += submit

        buttons = CTK.DruidButtonsPanel()
        buttons += CTK.DruidButton_Submit (_("Review"))
        buttons += CTK.DruidButton_Close (_("Cancel"))
        cont += buttons

        return cont.Render().toStr()


CTK.publish ('^%s$'%(URL_REVIEW),       Review)
CTK.publish ('^%s$'%(URL_REVIEW_FAIL),  Review_Fail)
CTK.publish ('^%s$'%(URL_REVIEW_APPLY), Review_Apply, method="POST")
