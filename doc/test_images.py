#!/usr/bin/env python3

# Cherokee Doc: Image checker
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2014 Alvaro Lopez Ortega
# This file is distributed under the GPL2 license.

import os
import sys

def get_img_refs():
    img_refs = {}

    for file in [x for x in os.listdir('.') if x.endswith('.txt')]:
        for img_line in [x for x in open (file, 'r').readlines() if 'image:' in x]:
            filename = img_line.replace('::',':').strip()[6:]
            fin = filename.rfind('[')
            if fin > 0:
                filename = filename[:fin]
            img_refs[filename] = None

    return list(img_refs.keys())

def get_img_files():
    def is_image(file):
        return file.endswith('.jpg') or file.endswith('.jpeg') or file.endswith('.png')

    tmp = [f'media/images/{x}' for x in os.listdir('media/images')]
    return list(filter (is_image, tmp))

def check_images():
    error = False

    img_refs  = get_img_refs()
    img_files = get_img_files()

    for ref in img_refs:
        if not ref in img_files:
            print(f"ERROR: {ref}: File not found")
            error = True

    for img in img_files:
        if '-CENSORED' in img:
            uc_img = img.replace("-CENSORED", "")
            if not uc_img in img_files:
                print(f"ERROR: {uc_img}: uncensored variant not found")
                error = True
            else:
                img = uc_img
        if not img in img_refs:
            print(f"ERROR: {img}: No longer used")
            error = True

    automake_am = open("Makefile.am", 'r').read()
    for img in img_refs:
        if not img in automake_am:
            print(f"ERROR: {img} isn't covered in Makefile.am ")
            error = True

    return error

if __name__ == "__main__":
    # Exit if .txt files are not included
    if not [x for x in os.listdir('.') if x.endswith('.txt')]:
        print("Nothing to check..")
        raise SystemExit

    # Test
    error = check_images()
    sys.exit(int(error))
