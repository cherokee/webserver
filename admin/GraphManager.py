import os
import time
from configured import *

def graphs_are_active (cfg):
    return cfg.get_val("server!collector") != None


def graphs_get_images (cfg, filter):
    rrd_dir = cfg.get_val("server!collector!database_dir", CHEROKEE_RRD_DIR)
    img_dir = os.path.join (rrd_dir, "images")

    files = []
    if os.path.exists (img_dir):
        for f in os.listdir(img_dir):
            if not f.startswith(filter):
                continue

            interval = f.split('.')[-2].split('_')[-1]
            files.append((interval, f))

    def cmp_tuple(x,y):
        vals = {'h': 60*60,
                'd': 60*60*24,
                'w': 60*60*24*7,
                'm': 60*60*24*31,
                'y': 60*60*24*366}
        n1,c1 = int(x[0][:-1]), vals[x[0][-1]]
        n2,c2 = int(y[0][:-1]), vals[y[0][-1]]
        return cmp(n1*c1, n2*c2)

    files.sort(cmp_tuple)
    return files
