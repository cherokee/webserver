class VServerEntries:
    def __init__ (self, host, cfg, user_dir=False):
        self._cfg   = cfg
        self._host  = host
        self._rules = []
        
        # Build the internal rules list
        if not user_dir:
            dirs_cfg = self._cfg['vserver!%s!directory'  % (self._host)]
            exts_cfg = self._cfg['vserver!%s!extensions' % (self._host)]
            reqs_cfg = self._cfg['vserver!%s!request'    % (self._host)]
        else:
            dirs_cfg = self._cfg['vserver!%s!user_dir!directory'  % (self._host)]
            exts_cfg = self._cfg['vserver!%s!user_dir!extensions' % (self._host)]
            reqs_cfg = self._cfg['vserver!%s!user_dir!request'    % (self._host)]

        if dirs_cfg:
            for d_name in dirs_cfg:
                prio = dirs_cfg[d_name]['priority'].value
                self._rules.append (('directory', d_name, prio, dirs_cfg[d_name]))
        if exts_cfg:
            for e_name in exts_cfg:
                prio = exts_cfg[e_name]['priority'].value
                self._rules.append (('extensions', e_name, prio, exts_cfg[e_name]))
        if reqs_cfg:
            for r_name in reqs_cfg:
                prio = reqs_cfg[r_name]['priority'].value
                self._rules.append (('request', r_name, prio, reqs_cfg[r_name]))

        self._rules.sort(lambda x,y: cmp(int(x[2]), int(y[2])))

    # Integration methods
    #
    def __iter__ (self):
        return iter(self._rules)

    def __delitem__ (self, prio):
        for i in range(len(self._rules)):
            entry = self._rules[i]
            if entry[2] == prio:
                del (self._rules[i])

    def __getitem__ (self, prio):
        for e in self._rules:
            if e[2] == prio:
                return e

    def __len__ (self):
        return len(self._rules)
