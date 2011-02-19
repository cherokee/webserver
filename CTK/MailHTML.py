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

            if kind == 'p':
                body += '<p>%s</p>'%(text)
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

            if kind == 'p':
                body += '%s\n\n'%(wrap_text (text))
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
        msg['Subject'] = self.subject
        msg['From']    = self.me
        msg['To']      = self.to

        part1 = MIMEText (self.RenderTXT(),  'plain')
        part2 = MIMEText (self.RenderHTML(), 'html')

        msg.attach (part1)
        msg.attach (part2)

        return msg

    def RenderMessage_Images (self):
        msg_root = MIMEMultipart('related')
        msg_root['Subject'] = self.subject
        msg_root['From']    = self.me
        msg_root['To']      = self.to
        msg_root.preamble   = 'This is a multi-part message in MIME format.'

        msg = MIMEMultipart('alternative')
        msg['Subject'] = self.subject
        msg['From']    = self.me
        msg['To']      = self.to

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

    def Send (self):
        msg = self.RenderMessage()

        s = smtplib.SMTP (MAIL_SERVER)
        s.sendmail (self.me, self.to, msg.as_string())
        s.quit()

    def __repr__ (self):
        return self.RenderMessage().as_string()


if __name__ == "__main__":
    # Mail Headers
    mail = MailHTML ("Alvaro Lopez Ortega <alvaro@alobbs.com>",
                     "Alvaro Lopez Ortega <alvaro@octality.com>",
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
