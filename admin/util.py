import os

#
# Virtual Server
#

def cfg_vsrv_get_next (cfg):
    """ Get the prefix of the next vserver """
    tmp = [int(x) for x in cfg.keys("vserver")]
    tmp.sort()
    next = str(tmp[-1] + 10)
    return "vserver!%s" % (next)

def cfg_vsrv_rule_get_next (cfg, pre):
    """ Get the prefix of the next rule of a vserver """
    tmp = [int(x) for x in cfg.keys("%s!rule"%(pre))]
    tmp.sort()
    if tmp:
        next = str(tmp[-1] + 10)
    else:
        next = "100"
    return "%s!rule!%s" % (pre, next)

def cfg_vsrv_rule_find_extension (cfg, pre, extension):
    """Find an extension rule in a virtual server """
    for r in cfg.keys("%s!rule"%(pre)):
        p = "%s!rule!%s" % (pre, r)
        if cfg.get_val ("%s!match"%(p)) == "extensions":
            if extension in cfg.get_val ("%s!match!extensions"%(p)):
                return p


#
# Information Sources
#

def cfg_source_get_next (cfg):
    tmp = [int(x) for x in cfg.keys("source")]
    if not tmp:
        return "source!1"
    tmp.sort()
    next = str(tmp[-1] + 10)
    return "source!%s" % (next)

def cfg_source_find_interpreter (cfg, 
                                 in_interpreter = None,
                                 in_nick        = None):
    for i in cfg.keys("source"):
        if cfg.get_val("source!%s!type"%(i)) != 'interpreter':
            continue

        if (in_interpreter and
            in_interpreter in cfg.get_val("source!%s!interpreter"%(i))):
            return "source!%s" % (i)

        if (in_nick and
            in_nick in cfg.get_val("source!%s!nick"%(i))):
            return "source!%s" % (i)


#
# Paths
#

def path_find_binary (executable, extra_dirs=[], custom_test=None):
    """Find an executable.
    It checks 'extra_dirs' and the PATH.
    The 'executable' parameter can be either a string or a list.
    """

    assert (type(executable) in [str, list])

    dirs = extra_dirs

    env_path = os.getenv("PATH")
    if env_path:
        dirs += filter (lambda x: x, env_path.split(":"))

    for dir in dirs:
        if type(executable) == str:
            tmp = os.path.join (dir, executable)
            if os.path.exists (tmp):
                if custom_test and custom_test(tmp):
                    return tmp
        elif type(executable) == list:
            for n in executable:
                tmp = os.path.join (dir, n)
                if os.path.exists (tmp):
                    if custom_test and custom_test(tmp):
                        return tmp



