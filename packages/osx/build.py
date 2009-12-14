#!/usr/bin/env python

import os
import re
from utils import *

MAKE_PARAMS       = "-j4"
TMP               = "/var/tmp"
DESTDIR           = "%s/cherokee-destdir" % (TMP)
DESTDIR_ROOT      = "%s/cherokee-destdir/root" % (TMP)
DESTDIR_RESOURCES = "%s/cherokee-destdir/resouces" % (TMP)
PKG_MAKER         = "/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker"


def _figure_version (path):
    info  = open (os.path.join (path, "Info.plist"), 'r').read()
    regex = r"<key>CFBundleShortVersionString</key>[ \n\r]*<string>(.+)</string>"
    return re.findall(regex, info, re.MULTILINE)[0]


def _perform():
    dmg_fullpath = "%s/Cherokee-%s.dmg" % (osx_dir, version)

    # Clean up
    exe ("sudo rm -rfv %s %s" % (DESTDIR, dmg_fullpath), colorer=red)
    exe ("mkdir -p %s %s" % (DESTDIR_ROOT, DESTDIR_RESOURCES), colorer=blue)

    # Ensure it's compiled
    chdir (src_topdir)
    exe ("make %s" % (MAKE_PARAMS))

    # Install it
    exe ("sudo make install DESTDIR=%s" % (DESTDIR_ROOT))
    exe ("sudo mv %s/usr/local/etc/cherokee/cherokee.conf %s/usr/local/etc/cherokee/cherokee.conf.example" % (DESTDIR_ROOT, DESTDIR_ROOT), colorer=red)

    # Fix permissions (TODO)
    None

    # Copy resources
    exe ("cp -v %s/Info.plist %s" % (osx_dir, DESTDIR_RESOURCES), colorer=green)
    exe ("cp -v %s/Description.plist %s" % (osx_dir, DESTDIR_RESOURCES), colorer=green)
    exe ("cp -v %s/License.rtf %s" % (osx_dir, DESTDIR_RESOURCES), colorer=green)
    exe ("gunzip --stdout %s/background.tiff.gz > %s/background.tiff" % (osx_dir, DESTDIR_RESOURCES), colorer=green)

    # Clean up
    exe ("rm -rfv %s/cherokee-%s.pkg" %(TMP, version), colorer=red)
    exe ("rm -rfv %s/Cherokee-%s.dmg" %(TMP, version), colorer=red)

    # Build package and image
    chdir ('%s/usr/local' %(DESTDIR_ROOT))
    exe ("%s -build -v " % (PKG_MAKER) +
         "-p %s/cherokee-%s.pkg " % (TMP, version) +
         "-f %s/usr/local " % (DESTDIR_ROOT) +
         "-r %s " % (DESTDIR_RESOURCES) +
         "-i %s/Info.plist " % (DESTDIR_RESOURCES) +
         "-d %s/Description.plist " % (DESTDIR_RESOURCES))

    chdir (TMP)
    exe ("hdiutil create -volname Cherokee-%s -srcfolder cherokee-%s.pkg %s" % (version, version, dmg_fullpath), colorer=green)
    exe ("ls -l %s" % (dmg_fullpath))


def perform():
    # Change the current directory
    prev_dir = chdir (TMP)

    # Perform
    try:
        _perform()
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


# Globals
osx_dir    = os.path.abspath (os.path.dirname(__file__))
src_topdir = os.path.abspath ("%s/../.."%(osx_dir))
version    = _figure_version (osx_dir)

if __name__ == "__main__":
    check_preconditions()
    perform()
