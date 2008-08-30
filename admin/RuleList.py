class RuleList:
    def __init__ (self, cfg, prefix):
        self._cfg     = cfg
        self._cfg_pre = prefix

        self._normalize()

    def _normalize (self):
        if not self._cfg[self._cfg_pre]:
            return 

        # Build the new list
        child = []
        to_be_deleted = []
        for ns in self:
            pre = '%s!%s' % (self._cfg_pre, ns)
            entry = self._cfg[pre]
            child.append(entry)
            to_be_deleted.append(ns)
        child.reverse()

        # Delete old child
        for n in to_be_deleted:
            del (self._cfg["%s!%s" % (self._cfg_pre, n)])

        # Set the new childs
        i = 100
        for item in child:
            pre = '%s!%d' % (self._cfg_pre, i)
            self._cfg.set_sub_node (pre, item)
            i += 100

    def __iter__ (self):
        tmp = self._cfg[self._cfg_pre]
        if tmp:
            keys = [int(x) for x in tmp.keys()]
            keys.sort (reverse=True) 
            for k in keys:
                yield str(k)

    def __len__ (self):
        tmp = self._cfg[self._cfg_pre]
        if not tmp: return 0
        return len(tmp.keys())

    def keys (self):
        tmp = self._cfg[self._cfg_pre]
        if not tmp: return []
        return tmp.keys()

    def __getitem__ (self, prio):
        return self._cfg['%s!%s' % (self._cfg_pre, str(prio))]

    def get_highest_priority(self):
        keys = self._cfg.keys(self._cfg_pre)
        if not keys:
            return 100
        tmp = [int(x) for x in keys]
        tmp.sort()
        return tmp[-1]

    def change_prios (self, changes):
        # Build the new child list
        relocated = {}
        for change in changes:
            old, new = int(change[0]), int(change[1])
            if old != new:
                relocated[new] = self[old]

        # Add any missing child nodes
        for entry in self:
            p_entry = int(entry)
            if p_entry not in relocated.keys():
                relocated[p_entry] = self[entry]
                
        # Reassign childs
        for p in self.keys():
            del (self._cfg["%s!%s" % (self._cfg_pre, p)])

        for item in relocated.keys():
            pre = '%s!%d' % (self._cfg_pre, item)
            self._cfg.set_sub_node (pre, relocated[item])
    
