#!/usr/bin/env python3

# Cherokee CLI configuration conversion
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2014 Alvaro Lopez Ortega
# This file is distributed under the GPL2 license.

import os
import sys

# Import CTK as a package, rather than putting its implementation directory
# directly on sys.path.
sys.path.append (os.path.abspath (os.path.join(os.path.dirname(__file__), 'CTK')))
from CTK.Config import Config

from configured import *
from config_version import *


def main():
    # Read the command line parameter
    try:
        cfg_file = sys.argv[1]
    except:
        print(f"Incorrect parameters: {sys.argv[0]} CONFIG_FILE")
        raise SystemExit

    # Parse the configuration file
    cfg = Config(cfg_file)

    # Update the configuration file if needed
    ver_config  = int (cfg.get_val('config!version', '000099028'))
    ver_release = int (config_version_get_current())

    print(f"Upgrading '{cfg_file}' from {int(ver_config)} to {int(ver_release)}..", end=' ')

    # Convert it
    updated = config_version_update_cfg (cfg)
    print(["Not upgraded.", "Upgraded."][updated])

    # Save it
    if updated:
        print("Saving new configuration..", end=' ')
        cfg.save()
        print("OK")

if __name__ == '__main__':
    main()
