#!/usr/bin/env python2

# Cherokee CLI configuration conversion
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2014 Alvaro Lopez Ortega
# This file is distributed under the GPL2 license.

import os
import sys

# Import CTK
sys.path.append (os.path.abspath (os.path.realpath(__file__) + '/../CTK/CTK'))
from Config import *

from configured import *
from config_version import *


def main():
    # Read the command line parameter
    try:
        cfg_file = sys.argv[1]
    except:
        print "Incorrect parameters: %s CONFIG_FILE" % (sys.argv[0])
        raise SystemExit

    # Parse the configuration file
    cfg = Config(cfg_file)

    # Update the configuration file if needed
    ver_config  = int (cfg.get_val('config!version', '000099028'))
    ver_release = int (config_version_get_current())

    print "Upgrading '%s' from %d to %d.." % (cfg_file, ver_config, ver_release),

    # Convert it
    updated = config_version_update_cfg (cfg)
    print ["Not upgraded.", "Upgraded."][updated]

    # Save it
    if updated:
        print "Saving new configuration..",
        cfg.save()
        print "OK"

if __name__ == '__main__':
    main()
