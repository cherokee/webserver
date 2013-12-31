# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2014 Alvaro Lopez Ortega
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

TYPE_VSERVER = 1
TYPE_RULE    = 1 << 2

LIST = [
    {'name':  'CMS',
     'title': N_('CMS'),
     'descr': N_('Content Management Systems'),
     'list':  [
            {'name':  'concrete5',
             'title': N_('Concrete 5'),
             'descr': N_('The CMS made for Marketing, but built for Geeks.'),
             'type':  TYPE_VSERVER | TYPE_RULE},
            ]
     },

    {'name':  'appservers',
     'title': N_('Application Servers'),
     'descr': N_('General Purpose Application Servers'),
     'list':  [
            {'name':  'alfresco',
             'title': N_('Alfresco'),
             'descr': N_('Enterprise web platform for building business solutions.'),
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'glassfish',
             'title': N_('Glassfish'),
             'descr': N_('The first compatible implementation of the Java EE 6 platform specification.'),
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'coldfusion',
             'title': N_('Adobe ColdFusion'),
             'descr': N_('Rapidly build Internet applications for the enterprise.'),
             'type':  TYPE_VSERVER | TYPE_RULE}
             ]
    },

    {'name':  'platforms',
     'title': N_('Platforms'),
     'descr': N_('Web Development Platforms'),
     'list':  [
            {'name':  'django',
             'title': N_('Django'),
             'descr': N_('The Web framework for perfectionists (with deadlines).'),
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'rails',
             'title': N_('Ruby on Rails'),
             'descr': N_('Open-source web framework optimized for sustainable productivity.'),
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'zend',
             'title': N_('Zend'),
             'descr': N_('Simple, straightforward, open-source software framework for PHP 5.'),
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'symfony',
             'title': N_('Symfony'),
             'descr': N_('Full-stack framework, a library of cohesive classes written in PHP.'),
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'uwsgi',
             'title': N_('uWSGI'),
             'descr': N_('Fast, self-healing, developer-friendly, language-agnostic, application container.'),
             'type':  TYPE_VSERVER | TYPE_RULE}
             ]
    },

    {'name':  'langs',
     'title': N_('Languages'),
     'descr': N_('Development Languages and Platforms'),
     'list':  [
            {'name':  'php',
             'title': N_('PHP'),
             'descr': N_('Widely-used general-purpose scripting language.'),
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'mono',
             'title': N_('.NET with Mono'),
             'descr': N_('Cross-platform, open-source .NET development framework.'),
             'type':  TYPE_VSERVER | TYPE_RULE},
             ]
    },

    {'name':  'webapps',
     'title': N_('Web Applications'),
     'descr': N_('General Purpose Applications'),
     'list':  [
            {'name':  'mailman',
             'title': N_('GNU MailMan'),
             'descr': N_('Free software for managing electronic mail discussion and e-newsletter lists.'),
             'type':  TYPE_VSERVER},
            {'name':  'rtorrent',
             'title': N_('rTorrent'),
             'descr': N_('BitTorrent client written in C++, based on the libTorrent libraries for Unix.'),
             'type':  TYPE_RULE},
             ]
    },

    {'name':  'tasks',
     'title': N_('Tasks'),
     'descr': N_('Common Maintenance Tasks'),
     'list':  [
            {'name':  'ssl_test',
             'title': N_('SSL/TLS Testing'),
             'descr': N_('Auto-configure SSL/TLS support with a self-signed certificate.'),
             'type':  TYPE_RULE},
            {'name':  'flcache',
             'title': N_('Content Caching'),
             'descr': N_('Boost your server performance by enabling content caching.'),
             'type':  TYPE_RULE},
            {'name':  'redirect',
             'title': N_('Virtual Server Redirection'),
             'descr': N_('Create a new virtual server redirecting every request to another domain host.'),
             'type':  TYPE_VSERVER},
            {'name':  'hotlinking',
             'title': N_('Anti Hot-Linking'),
             'descr': N_('Stop other domains from hot-linking your media files.'),
             'type':  TYPE_RULE},
            {'name':  'icons',
             'title': N_('Directory-listing Icons'),
             'descr': N_('Add the /cherokee_icons and /cherokee_themes directories used for directory listing.'),
             'type':  TYPE_RULE},
            {'name':  'static',
             'title': N_('Static-file support'),
             'descr': N_('Add a rule to optimally serve the most common static files.'),
             'type':  TYPE_RULE},
            {'name':  'streaming',
             'title': N_('Media file streaming'),
             'descr': N_('Add a rule to stream media files through an intelligent streaming handler.'),
             'type':  TYPE_RULE},
             ]
    },
]
