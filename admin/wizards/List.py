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
             'descr': 'Drupal is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'wordpress',
             'title': 'Worpress',
             'descr': 'Wordpress is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'joomla',
             'title': 'Joomla',
             'descr': 'Joomla is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'moinmoin',
             'title': 'MoinMoin Wiki',
             'descr': 'MoinMoin is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'trac',
             'title': 'Trac Bug Tracker System',
             'descr': 'Trac is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'concrete5',
             'title': 'Concrete 5',
             'descr': 'Concrete5 is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE}
            ]
     },

    {'name':  'appservers',
     'title': _('Application Servers'),
     'descr': _('General Purpose Application Servers'),
     'list':  [
            {'name':  'alfresco',
             'title': 'Alfresco',
             'descr': 'Alfresco is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'liferay',
             'title': 'Liferay',
             'descr': 'Liferay is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'glassfish',
             'title': 'Glassfish',
             'descr': 'Glassfish is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'coldfusion',
             'title': 'Adobe ColdFusion',
             'descr': 'ColdFussion is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE}
             ]
    },

    {'name':  'platforms',
     'title': _('Platforms'),
     'descr': _('Web Development Platforms'),
     'list':  [
            {'name':  'django',
             'title': 'Django',
             'descr': 'Django is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'rails',
             'title': 'Ruby on Rails',
             'descr': 'Ruby on Rails is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'zend',
             'title': 'Zend',
             'descr': 'Zend is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'symfony',
             'title': 'Symfony',
             'descr': 'Symfony is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'uWSGI',
             'title': 'uWSGI',
             'descr': 'uWSGI is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE}
             ]
    },

    {'name':  'langs',
     'title': _('Languages'),
     'descr': _('Development Languages and Platforms'),
     'list':  [
            {'name':  'php',
             'title': 'PHP',
             'descr': 'PHP is bla, bla, bla..',
             'type':  TYPE_RULE},
            {'name':  'mono',
             'title': '.NET with Mono',
             'descr': 'Mono.NET is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE},
             ]
    },

    {'name':  'webapps',
     'title': _('Web Applications'),
     'descr': _('General Purpose Applications'),
     'list':  [
            {'name':  'mailman',
             'title': 'GNU MailMan',
             'descr': 'Mailman is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'moodle',
             'title': 'Moodle',
             'descr': 'Moodle is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'phpbb',
             'title': 'PhpBB Forum',
             'descr': 'phpBB is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE},
            {'name':  'sugar',
             'title': 'Sugar CRM',
             'descr': 'Sugar CRM is bla, bla, bla..',
             'type':  TYPE_VSERVER | TYPE_RULE},
             ]
    },

    {'name':  'tasks',
     'title': _('Tasks'),
     'descr': _('Common Maintenance Tasks'),
     'list':  [
            {'name':  'redirect',
             'title': 'Virtual Server redirection',
             'descr': 'Redirection is bla, bla, bla..',
             'type':  TYPE_VSERVER},
             ]
    },
]
