# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2010 Alvaro Lopez Ortega
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
     'title': _('CMS'),
     'descr': _('Content Management Systems'),
     'list':  [
            {'name':  'drupal',
             'title': 'Drupal',
             'descr': 'CMS for everything from personal weblogs to large community-driven websites.',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'wordpress',
             'title': 'Worpress',
             'descr': 'State-of-the-art publishing platform with a focus on aesthetics, web standards, and usability.',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'joomla',
             'title': 'Joomla',
             'descr': 'Build Web sites and powerful online applications',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'moinmoin',
             'title': 'MoinMoin Wiki',
             'descr': 'Advanced, easy to use and extensible WikiEngine with a large community of users.',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'trac',
             'title': 'Trac Bug Tracker System',
             'descr': 'Enhanced wiki and issue tracking system for software development projects.',
             'type':  TYPE_VSERVER},
            {'name':  'concrete5',
             'title': 'Concrete 5',
             'descr': 'The CMS made for Marketing, but built for Geeks.',
             'type':  TYPE_VSERVER | TYPE_RULE}
            ]
     },

    {'name':  'appservers',
     'title': _('Application Servers'),
     'descr': _('General Purpose Application Servers'),
     'list':  [
            {'name':  'alfresco',
             'title': 'Alfresco',
             'descr': 'Enterprise web platform for building business solutions.',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'liferay',
             'title': 'Liferay',
             'descr': 'Enterprise web platform for building business solutions.',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'glassfish',
             'title': 'Glassfish',
             'descr': 'The first compatible implementation of the Java EE 6 platform specification.',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'coldfusion',
             'title': 'Adobe ColdFusion',
             'descr': 'Rapidly build Internet applications for the enterprise.',
             'type':  TYPE_VSERVER | TYPE_RULE}
             ]
    },

    {'name':  'platforms',
     'title': _('Platforms'),
     'descr': _('Web Development Platforms'),
     'list':  [
            {'name':  'django',
             'title': 'Django',
             'descr': 'The Web framework for perfectionists (with deadlines).',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'rails',
             'title': 'Ruby on Rails',
             'descr': 'Open-source web framework optimized for sustainable productivity.',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'zend',
             'title': 'Zend',
             'descr': 'Simple, straightforward, open-source software framework for PHP 5.',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'symfony',
             'title': 'Symfony',
             'descr': 'Full-stack framework, a library of cohesive classes written in PHP.',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'uwsgi',
             'title': 'uWSGI',
             'descr': 'Fast, self-healing, developer-friendly WSGI server.',
             'type':  TYPE_VSERVER | TYPE_RULE}
             ]
    },

    {'name':  'langs',
     'title': _('Languages'),
     'descr': _('Development Languages and Platforms'),
     'list':  [
            {'name':  'php',
             'title': 'PHP',
             'descr': 'Widely-used general-purpose scripting language.',
             'type':  TYPE_RULE},
            {'name':  'mono',
             'title': '.NET with Mono',
             'descr': 'Cross-platform, open-source .NET development framework.',
             'type':  TYPE_VSERVER | TYPE_RULE},
             ]
    },

    {'name':  'webapps',
     'title': _('Web Applications'),
     'descr': _('General Purpose Applications'),
     'list':  [
            {'name':  'mailman',
             'title': 'GNU MailMan',
             'descr': 'PHP web application to handle the administration of MySQL.',
             'type':  TYPE_RULE},
            {'name':  'mailman',
             'title': 'GNU MailMan',
             'descr': 'Free software for managing electronic mail discussion and e-newsletter lists.',
             'type':  TYPE_VSERVER},
            {'name':  'moodle',
             'title': 'Moodle',
             'descr': 'Course Management System / Virtual Learning Environment.',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'phpbb',
             'title': 'PhpBB Forum',
             'descr': 'The most widely used Open Source forum solution.',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'sugar',
             'title': 'Sugar CRM',
             'descr': 'Enable organizations to efficiently organize customer relationships.',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'rtorrent',
             'title': 'rTorrent',
             'descr': 'BitTorrent client written in C++, based on the libTorrent libraries for Unix.',
             'type':  TYPE_RULE},
             ]
    },

    {'name':  'tasks',
     'title': _('Tasks'),
     'descr': _('Common Maintenance Tasks'),
     'list':  [
            {'name':  'redirect',
             'title': 'Virtual Server Redirection',
             'descr': 'Create a new virtual server redirecting every request to another domain host.',
             'type':  TYPE_VSERVER},
            {'name':  'hotlinking',
             'title': 'Anti Hot-Linking',
             'descr': 'Stop other domains from hot-linking your media files.',
             'type':  TYPE_RULE},
            {'name':  'icons',
             'title': 'Directory-listing Icons',
             'descr': 'Add the /icons and /cherokee_themes directories used for directory listing.',
             'type':  TYPE_RULE},
            {'name':  'static',
             'title': 'Static-file support',
             'descr': 'Add a rule to optimally serve the most common static files.',
             'type':  TYPE_RULE},
            {'name':  'streaming',
             'title': 'Media file streaming',
             'descr': 'Add a rule to stream media files through an intelligent streaming handler.',
             'type':  TYPE_RULE},
             ]
    },
]
