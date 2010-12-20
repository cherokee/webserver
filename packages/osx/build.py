#!/usr/bin/env python

import os
import re
from utils import *

MAKE_PARAMS = "-j4"
TMP         = "/var/tmp"
PKG_MAKER   = "/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker"


def _figure_version (path):
    info  = open (os.path.join (path, "Info.plist"), 'r').read()
    regex = r"<key>CFBundleShortVersionString</key>[ \n\r]*<string>(.+)</string>"
    return re.findall(regex, info, re.MULTILINE)[0]


# Globals
osx_dir      = os.path.abspath (os.path.dirname(__file__))
src_topdir   = os.path.abspath ("%s/../.."%(osx_dir))
version      = _figure_version (osx_dir)
dmg_dir      = "%s/Cherokee-dmg"    % (osx_dir)
dmg_rw_path  = "%s/Cherokee-%s-RW.dmg" % (osx_dir, version)
dmg_fullpath = "%s/Cherokee-%s.dmg" % (osx_dir, version)


def _clean_up():
    cherokee_destdir = "%s/cherokee-destdir" %(TMP)

    exe ("sudo rm -rfv %s"    % (cherokee_destdir), colorer=red)
    exe ("sudo rm -rfv %s %s" % (dmg_dir, dmg_fullpath), colorer=red)
    exe ("mkdir -p %s %s"     % (dmg_dir, cherokee_destdir))


def _build_cherokee():
    destdir      = "%s/cherokee-destdir" %(TMP)
    root_dir     = "%s/cherokee-destdir/root" %(TMP)
    resource_dir = "%s/cherokee-destdir/resources" %(TMP)

    # Ensure it's compiled
    chdir (src_topdir)
    exe ("make %s" % (MAKE_PARAMS))

    # Install it
    exe ("sudo make install DESTDIR=%s" % (root_dir))
    exe ("sudo mv %s/usr/local/etc/cherokee/cherokee.conf %s/usr/local/etc/cherokee/cherokee.conf.example" % (root_dir, root_dir), colorer=red)

    # Fix permissions (TODO)
    None

    # Copy resources
    exe ("mkdir -p %s" %(resource_dir), colorer=blue)

    exe ("cp -v %s/Info.plist        %s" %(osx_dir, resource_dir), colorer=green)
    exe ("cp -v %s/Description.plist %s" %(osx_dir, resource_dir), colorer=green)
    exe ("cp -v %s/License.rtf       %s" %(osx_dir, resource_dir), colorer=green)
    exe ("gunzip --stdout %s/background.tiff.gz > %s/background.tiff" % (osx_dir, resource_dir), colorer=green)

    # Build pkg
    exe ("rm -rfv %s/cherokee.pkg" %(dmg_dir), colorer=red)

    chdir ('%s/usr/local' %(root_dir))

    exe ("%s --verbose " % (PKG_MAKER) +
         "--id org.cherokee.webserver " +
         "--root %s/usr/local " % (root_dir) +
         "--domain system " +
         "--root-volume-only " +
         "--info %s/Info.plist " %(resource_dir) +
         "--resources %s " %(resource_dir) +
         "--version %s " %(version) +
         "--out %s/cherokee.pkg" %(dmg_dir))

def _copy_cherokee_app():
    exe ("cp -rv %s/Cherokee-admin.app '%s/Cherokee Admin.app'" %(osx_dir, dmg_dir))
    exe ("cp -rv %s/dmg-background.png '%s/background.png'"     %(osx_dir, dmg_dir))
    exe ("ln -s /Applications %s/Applications" %(dmg_dir))

## http://efreedom.com/Question/1-96882/Create-Nice-Looking-DMG-Mac-OS-Using-Command-Line-Tools
## http://clanmills.com/articles/macinstallers/

def _build_dmg():
    chdir (TMP)

    # RW version
    exe ("hdiutil create -verbose -format UDRW " +
         "-volname Cherokee-%s " %(version) +
         "-srcfolder %s " %(dmg_dir) +
         "%s" %(dmg_fullpath), colorer=green)

    # Attach
    path = exe_output ("hdiutil attach %s | sed -n 's/.*\(\/Volumes\/.*\)/\1/p'" %(dmg_fullpath))
    print "path", path

    exe ("ls -l %s" % (dmg_fullpath))


def perform():
    # Change the current directory
    prev_dir = chdir (TMP)

    # Perform
    try:
        _clean_up()
        _build_cherokee()
        _copy_cherokee_app()
        _build_dmg()
    except:
        chdir (prev_dir)
        raise

    # Done and dusted
    chdir (prev_dir)
    print yellow("Done and dusted!")


def check_preconditions():
    conf = os.path.join (src_topdir, "cherokee.conf.sample")
    assert "server!user = www" in open(conf).read(), "Bad user"
    assert "server!group = www" in open(conf).read(), "Bad group"
    assert "prefix = /usr/local" in open("Makefile").read(), "Wrong configuration"
    assert os.access (TMP, os.W_OK), "Cannot compile in: %s" %(TMP)
    assert which("hdiutil"), "hdiutil is required"
    assert which("gunzip"), "gunzip is required"
    assert which("svn"), "SVN is required"


if __name__ == "__main__":
    check_preconditions()
    perform()
