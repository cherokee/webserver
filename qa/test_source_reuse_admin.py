#!/usr/bin/env python3
"""Source connection settings administration QA."""
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
from urllib.parse import urlencode

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'admin/CTK'), str(ROOT / 'admin')]
import CTK
from CTK.Post import Post
import PageSource


class SourceAdminTests(unittest.TestCase):
    def setUp(self):
        self.cfg = CTK.Config()
        self.cfg['source!1!nick'] = 'PHP-FPM'
        self.cfg['source!1!type'] = 'host'
        self.cfg['source!1!host'] = '127.0.0.1:9000'

    def commit(self, values):
        with patch.object(CTK, 'cfg', self.cfg), patch.object(CTK, 'post', Post(urlencode(values))), \
             patch.object(CTK, 'cfg_apply_post', return_value={'ret': 'ok'}) as apply:
            result = PageSource.commit()
        return result, apply.called

    def test_accept_connection_settings(self):
        values = {f'source!1!{key}': str(bounds[1]) for key, bounds in PageSource.SOURCE_CONNECTION_OPTIONS.items()}
        values['source!1!keepalive'] = '1'
        result, applied = self.commit(values)
        self.assertEqual(result['ret'], 'ok')
        self.assertTrue(applied)

    def test_reject_invalid_settings_without_mutation(self):
        for name in PageSource.SOURCE_CONNECTION_OPTIONS:
            for value in ('-1', 'abc', '99999999999999999999', '1.2', '١'):
                with self.subTest(name=name, value=value):
                    result, applied = self.commit({f'source!1!{name}': value, 'source!1!nick': 'must not change'})
                    self.assertEqual(result['ret'], 'error')
                    self.assertFalse(applied)
                    self.assertEqual(self.cfg.get_val('source!1!nick'), 'PHP-FPM')
        for value in ('2', 'yes'):
            result, applied = self.commit({'source!1!keepalive': value})
            self.assertEqual(result['ret'], 'error')
            self.assertFalse(applied)

    def test_clear_restores_defaults_and_zero_timeouts(self):
        result, applied = self.commit({'source!1!keepalive': '', 'source!1!keepalive_timeout': '',
                                       'source!1!io_timeout': '0', 'source!1!retries': '0'})
        self.assertEqual(result['ret'], 'ok')
        self.assertTrue(applied)
        for name in ('keepalive_max', 'keepalive_requests', 'keepalive_age', 'keepalive_timeout'):
            result, applied = self.commit({f'source!1!{name}': '0'})
            self.assertEqual(result['ret'], 'error')
            self.assertFalse(applied)

    def test_render_host_and_interpreter_settings(self):
        class Request:
            url = '/source/content/1'
        for kind in ('host', 'interpreter'):
            self.cfg['source!1!type'] = kind
            with patch.object(CTK, 'cfg', self.cfg), patch.object(CTK, 'request', Request()):
                result = PageSource.Render_Source()()
            for name in ('keepalive', *PageSource.SOURCE_CONNECTION_OPTIONS):
                self.assertIn('source!1!'+name, str(result))


if __name__ == '__main__':
    unittest.main(verbosity=2)
