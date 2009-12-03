import configured

# Converts from 0.99.30 to 0.99.31
def upgrade_to_0_99_31 (cfg):
    # verver!_!logger!error is vserver!_!error_writer now.
    # Must be relocated on each virtual server.
    for v in cfg.keys('vserver'):
        pre = 'vserver!%s' % (v)
        if cfg['vserver!%s!logger!error' %(v)]:
            cfg.clone ('vserver!%s!logger!error' %(v),
                       'vserver!%s!error_writer' %(v))
            del(cfg['vserver!%s!logger!error' %(v)])


def config_version_get_current():
    ver = configured.VERSION.split ('b')[0]
    v1,v2,v3 = ver.split (".")

    major = int(v1)
    minor = int(v2)
    micro = int(v3)

    return "%03d%03d%03d" %(major, minor, micro)


def config_version_cfg_is_up_to_date (cfg):
    # Cherokee's version
    ver_cherokee = config_version_get_current()

    # Configuration file version
    ver_config = cfg.get_val("config!version")
    if not ver_config:
        cfg["config!version"] = "000099028"
        return False

    # Cherokee 0.99.26 bug: 990250 is actually 99025
    if int(ver_config) == 990250:
        ver_config = "000099025"
        cfg['config!version'] = ver_config
        return False

    # Compare both of them
    if int(ver_config) > int(ver_cherokee):
        print "WARNING!! Running a new configuration file (version %d)"  % int(ver_config)
        print "          with an older version of Cherokee (version %d)" % int(ver_cherokee)
        return True

    elif int(ver_config) == int(ver_cherokee):
        return True
    
    else:
        return False


def config_version_update_cfg (cfg):
    # Do not proceed if it's empty
    if not cfg.has_tree():
        return False

    # Update only when it's outdated
    if config_version_cfg_is_up_to_date (cfg):
        return False

    ver_release_s = config_version_get_current()
    ver_config_s  = cfg.get_val("config!version")
    ver_config_i  = int(ver_config_s)

    # Update to.. 0.99.31
    if ver_config_i < 99031:
        upgrade_to_0_99_31 (cfg)

    # Update to.. 0.99.xx    

    cfg["config!version"] = ver_release_s
    return True
