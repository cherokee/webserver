import configured

# Converts from 0.99.30 to 0.99.31
def upgrade_to_0_99_31 (cfg):
    # verver!_!logger!error is vserver!_!error_writer now.
    # Must be relocated on each virtual server.
    #
    for v in cfg.keys('vserver'):
        pre = 'vserver!%s' % (v)
        if cfg['vserver!%s!logger!error' %(v)]:
            cfg.clone ('vserver!%s!logger!error' %(v),
                       'vserver!%s!error_writer' %(v))
            del(cfg['vserver!%s!logger!error' %(v)])

# Converts from 0.99.31-39 to 0.99.40
def upgrade_to_0_99_40 (cfg):
    # The encoder related configuration changed. What used to be
    # vserver!10!rule!600!encoder!gzip = 1  is now
    # vserver!10!rule!600!encoder!gzip = allow
    #
    # There are three possible options: "allow", "deny" and empty.
    # The previous "1" turns "allow", "0" is default so those entries
    # are removed and the brand new "deny" key is not assigned.
    #
    to_del = []
    for v in cfg.keys('vserver'):
        for r in cfg.keys('vserver!%s!rule'%(v)):
            if cfg['vserver!%s!rule!%s!encoder' %(v,r)]:
                for e in cfg['vserver!%s!rule!%s!encoder' %(v,r)]:
                    pre = 'vserver!%s!rule!%s!encoder!%s' %(v,r,e)
                    if cfg.get_val(pre) == "1":
                        cfg[pre] = "allow"
                    else:
                        to_del.append(pre)
    for pre in to_del:
        del(cfg[pre])

# Converts from 0.99.40 to 0.99.45
def upgrade_to_0_99_45 (cfg):
    # Remove some broken 'source' entries:
    # http://bugs.cherokee-project.com/768
    if 'None' in cfg.keys('source'):
        del (cfg['source!None'])

    # 'icons!suffix!package.png' included the 'z' and 'Z' extensions
    # when the property is meant to be caseless.
    tmp = cfg.get_val('icons!suffix!package.png')
    if tmp:
        cfg['icons!suffix!package.png'] = tmp.replace ('Z,', '')

# Converts from 0.99.45 to 1.0.7
def upgrade_to_1_0_7 (cfg):
    # Adds 'check_local_file' to Extension php
    # http://bugs.cherokee-project.com/951
    for v in cfg.keys('vserver'):
        for r in cfg.keys('vserver!%s!rule'%(v)):
            if cfg.get_val('vserver!%s!rule!%s!match'%(v,r)) == 'extensions' and \
               cfg.get_val('vserver!%s!rule!%s!match!extensions'%(v,r)) == 'php':
                cfg['vserver!%s!rule!%s!match!check_local_file'%(v,r)] = '1'

# Converts from 1.0.7 to 1.0.13
def upgrade_to_1_0_13 (cfg):
    # Converts !encoder!<x> = '1' to 'allow', and remove '0's
    for v in cfg.keys('vserver'):
        for r in cfg.keys('vserver!%s!rule'%(v)):
            for e in cfg.keys('vserver!%s!rule!%s!encoder'%(v,r)):
                key = 'vserver!%s!rule!%s!encoder!%s'%(v,r,e)
                val = cfg.get_val(key)
                if val == '1':
                    cfg[key] = 'allow'
                elif val == '0':
                    del(cfg[key])


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

    # Update to.. 0.99.40
    if ver_config_i < 99040:
        upgrade_to_0_99_40 (cfg)

    # Update to.. 0.99.45
    if ver_config_i < 99045:
        upgrade_to_0_99_45 (cfg)

    # Update to.. 1.0.7
    if ver_config_i < 1000007:
        upgrade_to_1_0_7 (cfg)

    # Update to.. 1.0.13
    if ver_config_i < 1000013:
        upgrade_to_1_0_13 (cfg)

    cfg["config!version"] = ver_release_s
    return True
