#
# Authors:
#       Brian Rosner <brosner@gmail.com>
#
# Credit:   Jacob Kaplan-Moss
#            - For all his hard work on the wonderful Django documentation and
#              is the basis of the Cherokee documentation.
#

import sys
import os
import shutil

try:
    import docutils
except ImportError:
    docutils = False
else:
    from docutils import nodes
    from docutils.core import publish_parts
    from docutils.writers import html4css1

from django.template import Context
from django.template.loader import get_template

def build_document(text):
    class CherokeeHTMLWriter(html4css1.Writer):

        def __init__(self):
            html4css1.Writer.__init__(self)
            self.translator_class = CherokeeHTMLTranslator

    class CherokeeHTMLTranslator(html4css1.HTMLTranslator):

        # prevent name attributes from being generated
        named_tags = []

        def visit_table(self, node):
            self.body.append(self.starttag(node, 'table', CLASS='docutils'))

        #
        # avoid <blockquotes>s around merely indented nodes.
        # copied from Django project website written by Jacob Kaplan-Moss
        #

        _suppress_blockquote_child_nodes = (
            nodes.bullet_list, nodes.enumerated_list, nodes.definition_list,
            nodes.literal_block, nodes.doctest_block, nodes.line_block,
            nodes.table
        )

        def _bq_is_valid(self, node):
            return len(node.children) != 1 or \
                not isinstance(*(
                    node.children[0],
                    self._suppress_blockquote_child_nodes
                ))

        def visit_block_quote(self, node):
            if self._bq_is_valid(node):
                html4css1.HTMLTranslator.visit_block_quote(self, node)

        def depart_block_quote(self, node):
            if self._bq_is_valid(node):
                html4css1.HTMLTranslator.depart_block_quote(self, node)
    
    return publish_parts(text, writer=CherokeeHTMLWriter())

def main():
    output_dir = '.'
    
    #
    # Check parameters
    #
    verbose = False
    if '-v' in sys.argv:
        verbose = True
        sys.argv.remove('-v')
    

    #
    # get the language to compile
    #
    try:
        try:
            lang = sys.argv[1]
        except IndexError:
            lang = 'en'
    finally:
        lang_dir = 'locale/%s/' % lang
        if not os.path.exists(lang_dir):
            print ('language %s does not exist.' % lang)
            raise SystemExit

    #
    # configure django to make templates work
    #
    from django.conf import settings
    settings.configure(**{
        'TEMPLATE_DEBUG': False,
        'TEMPLATE_LOADERS':
            ('django.template.loaders.filesystem.load_template_source',),
        'TEMPLATE_DIRS': ('templates/',)
    })
    
    doc_detail_template = get_template('doc_detail.html')
    
    #
    # get all the documentation files for the given language
    #
    for d, dirs, files in os.walk(lang_dir):
        
        media_root = os.path.join(
            *['..' for i in xrange(len(d.split('/')) - 2) \
                if not d.endswith('/')] + ['media/'])

        for ifile in files:
            # ignore hidden files
            if ifile.startswith('.'):
                continue
            
            # ignore non text files
            if not ifile.endswith('.txt'):
                continue
            
            # Output filename
            html_file = "%s.html" % ifile.split('.')[0]

            fp = open(os.path.join(d, ifile))
            data = fp.read()
            fp.close()

            if verbose:
                sys.stdout.write("Compiling %s.. " % (html_file))
                sys.stdout.flush()

            if docutils:
                parts = build_document(data)
            else:
                parts = {'title': 'Unknown', 'body': 'No docutils found.'}
            
            write_dir = os.path.join(output_dir, d.replace(lang_dir, ''))
            
            if not os.path.exists(write_dir):
                os.mkdir(write_dir)
            
            # create the context for django template
            c = Context({
                'media_root': media_root,
                'doc': parts
            })
            
            # write the compiled html doc
            ofile = os.path.join (write_dir, html_file)

            fp = open(ofile, 'w')
            fp.write(doc_detail_template.render(c))
            fp.close()

            if verbose:
                print ("OK")

if __name__ == '__main__':
    main()
