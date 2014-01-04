# -*- coding: utf-8 -*-

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

import smtplib

from email.mime.text import MIMEText
from email.MIMEImage import MIMEImage
from email.mime.multipart import MIMEMultipart

MAIL_SERVER   = "localhost"
TXT_LINE_WRAP = 80

def wrap_text (txt, cols = TXT_LINE_WRAP):
    render = ''

    while len(txt) > cols:
        for n in range(min(len(txt),cols)-1, 0, -1):
            if txt[n] != ' ':
                continue
            render += txt[:n] + '\n'
            txt = txt[n+1:]
            break

    if len(txt):
        render += txt

    return render


class MailHTML:
    def __init__ (self, me, to, subject):
        self.chunks  = []

        self.me      = me
        self.to      = to
        self.subject = subject
        self.images  = []
        self.headers = {}

        self.html_header = ''
        self.html_footer = ''
        self.txt_header  = ''
        self.txt_footer  = ''

    def add (self, txt, kind='p', props={}):
        self.chunks.append ((kind, txt, props.copy()))

    def add_image (self, img_id, file_path):
        # Eg: <img src="cid:image1">, where 'img_id' is: image1
        self.images.append ((img_id, file_path))

    def RenderHTML (self):
        body = ''
        for entry in self.chunks:
            kind, text, props = entry

            if props.has_key('txt_only'):
                continue

            if kind == 'raw':
                body += text
            elif kind == 'p':
                body += '<p>%s</p>'%(text)
            elif kind == 'p,center':
                body += '<p><div align="center">%s</div></p>'%(text)
            elif kind == 'h2':
                body += '<h2>%s</h2>'%(text)
            elif kind == 'h3':
                body += '<h3>%s</h3>'%(text)
            elif kind == 'li':
                body += '<li>%s</li>'%(text)
            elif kind == 'pre':
                body += '<pre>%s</pre>'%(text)
            else:
                assert False, "Unknown type '%s'"%(kind)

        html  = self.html_header
        html += body
        html += self.html_footer
        return html

    def RenderTXT (self):
        body = ''
        for entry in self.chunks:
            kind, text, props = entry

            if props.has_key('html_only'):
                continue

            if kind == 'raw':
                body += text
            elif kind == 'p':
                body += '%s\n\n'%(wrap_text (text))
            elif kind == 'p,center':
                margin = max (0, int((TXT_LINE_WRAP - len(text)) / 2))
                body += ' '*margin
                body += '%s\n\n'%(text)
            elif kind == 'h2':
                body += '%s\n%s\n\n'%(text, '-'*len(text))
            elif kind == 'h3':
                body += '%s\n'%(text)
            elif kind == 'li':
                body += ' * %s\n'%(text)
            elif kind == 'pre':
                body += '%s\n'%(text)
            else:
                assert False, "Unknown type '%s'"%(kind)

        txt  = self.txt_header
        txt += body
        txt += self.txt_footer
        return txt

    def RenderMessage (self):
        if self.images:
            return self.RenderMessage_Images()
        else:
            return self.RenderMessage_No_Images()

    def RenderMessage_No_Images (self):
        msg = MIMEMultipart('alternative')
        msg.set_charset('utf-8')
        msg['Subject']      = self.subject
        msg['From']         = self.me
        msg['To']           = self.to

        for h in self.headers:
            msg[h] = self.headers[h]

        part1 = MIMEText (self.RenderTXT(),  'plain', _charset='utf-8')
        part2 = MIMEText (self.RenderHTML(), 'html',  _charset='utf-8')

        msg.attach (part1)
        msg.attach (part2)

        return msg

    def RenderMessage_Images (self):
        msg_root = MIMEMultipart('related')
        msg_root['Subject']      = self.subject
        msg_root['From']         = self.me
        msg_root['To']           = self.to
        msg_root.preamble        = 'This is a multi-part message in MIME format.'

        for h in self.headers:
            msg_root[h] = self.headers[h]

        msg = MIMEMultipart('alternative')
        msg['Subject'] = self.subject
        msg['From']    = self.me
        msg['To']      = self.to

        for h in self.headers:
            msg[h] = self.headers[h]

        msg_root.attach (msg)

        part1 = MIMEText (self.RenderTXT(),  'plain')
        part2 = MIMEText (self.RenderHTML(), 'html')

        msg.attach (part1)
        msg.attach (part2)

        for image_entry in self.images:
            image_id, image_path = image_entry

            fp = open(image_path, 'rb')
            msg_image = MIMEImage(fp.read())
            fp.close()

            msg_image.add_header ('Content-ID', '<%s>'%(image_id))
            msg_root.attach (msg_image)

        return msg_root

    def Send (self, mail_server=MAIL_SERVER):
        msg = self.RenderMessage()

        s = smtplib.SMTP (mail_server)
        s.sendmail (self.me, self.to, msg.as_string())
        s.quit()

    def __repr__ (self):
        return self.RenderMessage().as_string()

    def __setitem__ (self, key, value):
        self.headers[key] = value


if __name__ == "__main__":
    # Mail Headers
    mail = MailHTML ("Alvaro Lopez Ortega <alvaro@alobbs.com>",
                     "Good looking HTML mail")

    # Template
    mail.html_header = "<html><body><h1>Header</h1>"
    mail.html_footer = "<hr/></body></html>"

    mail.txt_header = "**Header**\n\n"
    mail.txt_footer = "-- fin --\n"

    # Content
    mail.add_image ('logo', "static/images/market-logo.png")
    mail.add ("<a href=\"http://www.cherokee-market.com/\"><img src=\"cid:logo\"></a>", props={'html_only': True})
    mail.add ("This is the first paragraph: " + ', '.join([str(x) for x in range(200)]))
    mail.add ("This is the second one")
    mail.add ("And this is the last one")

    print mail
