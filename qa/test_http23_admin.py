#!/usr/bin/env python3
"""Admin configuration QA; no web browser or network sockets required."""
from pathlib import Path
import subprocess
import sys
import unittest
from unittest.mock import patch
from urllib.parse import urlencode

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / "admin/CTK"), str(ROOT / "admin")]
import CTK
from CTK.Post import Post
import PageGeneral


class ProtocolAdminTests(unittest.TestCase):
    def setUp(self):
        self.cfg = CTK.Config()
        self.cfg['server!tls'] = 'libssl'
        self.cfg['server!bind!1!port'] = '443'
        self.cfg['server!bind!1!tls'] = '1'
        self.cfg['server!bind!1!http2'] = '0'
        self.cfg['server!bind!1!http3'] = '0'
        self.info = subprocess.run([str(ROOT / 'cherokee/cherokee-worker'), '-i'], text=True, capture_output=True, check=False).stdout

    def commit(self, values, protocols=('http2', 'http3')):
        post = Post(urlencode(values))
        with patch.object(CTK, 'cfg', self.cfg), patch.object(CTK, 'post', post), \
             patch.object(CTK, 'cfg_apply_post', return_value={'ret': 'ok'}) as apply, \
             patch.object(PageGeneral.Cherokee.support, 'has_protocol', side_effect=lambda name: name in protocols):
            result = PageGeneral.commit()
            return result, apply.called

    def test_accept_tls_protocols(self):
        result, applied = self.commit({'server!bind!1!http2': '1', 'server!bind!1!http3': '1'})
        self.assertEqual(result['ret'], 'ok')
        self.assertTrue(applied)

    def test_reject_cleartext_without_mutation(self):
        result, applied = self.commit({'server!bind!1!tls': '0', 'server!bind!1!http2': '1'})
        self.assertEqual(result['ret'], 'error')
        self.assertFalse(applied)
        self.assertEqual(self.cfg.get_val('server!bind!1!tls'), '1')

    def test_reject_missing_build_feature(self):
        result, applied = self.commit({'server!bind!1!http3': '1'}, protocols=('http2',))
        self.assertEqual(result['ret'], 'error')
        self.assertFalse(applied)

    def test_reject_backend_removal_with_enabled_protocol(self):
        self.cfg['server!bind!1!http2'] = '1'
        for backend in ('', 'gnutls'):
            with self.subTest(backend=backend):
                result, applied = self.commit({'server!tls': backend})
                self.assertEqual(result['ret'], 'error')
                self.assertFalse(applied)

    def test_disable_protocol_and_tls_together(self):
        self.cfg['server!bind!1!http2'] = '1'
        result, applied = self.commit({'server!bind!1!http2': '0', 'server!bind!1!tls': '0'})
        self.assertEqual(result['ret'], 'ok')
        self.assertTrue(applied)

    def test_render_controls_and_capability_reporting(self):
        self.assertIn(' HTTP protocols: http1 http2 http3', self.info)
        with patch.object(CTK, 'cfg', self.cfg), \
             patch.object(PageGeneral.Cherokee.support, '_server_info', self.info):
            html = PageGeneral.PortsTable(CTK.Refreshable({'id': 'qa_protocol_ports'})).Render().html
        self.assertIn('server!bind!1!http2', html)
        self.assertIn('server!bind!1!http3', html)
        self.assertIn('HTTP/2', html)
        self.assertIn('HTTP/3', html)


if __name__ == '__main__':
    unittest.main(verbosity=2)
