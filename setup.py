#!/usr/bin/env python

from distutils.core import setup
from distutils.extension import Extension

setup (name         = "cherokee_pyscgi",
       version      = "1.16.1",
       description  = "Portable SCGI implementation",
       author       = "Alvaro Lopez Ortega",
       author_email = "alvaro@alobbs.com",
       url          = "http://www.cherokee-project.com/",
       license      = "BSD. Read LICENSE for details.",
       packages     = ['pyscgi'])
