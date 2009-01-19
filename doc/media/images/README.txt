Since the size of the help screens is fixed to 725px, screenshots
should not exceed it. Until 0.98, no image was wider than 569px in
width. After that, 724 px is used as standard (as it is the width left
by Gimp's auto-crop feature).

Please resize them if it is bigger than that.

Also, maximum recompression is recommended:

optipng -o7 (loosles)
pngnq       (lossy, but it's the one currently used).
