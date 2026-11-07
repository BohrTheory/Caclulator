"""Protocol boundary tests; synthetic reports do not prove product coverage."""
import json
import os
from pathlib import Path
import shutil
import tempfile
import unittest
from unittest.mock import patch
from gtest_inventory import verify_run


class InventoryTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.binary = self.root / 'fixture'
        self.inventory = self.root / 'inventory.json'
        self.inventory.write_text(json.dumps({'schemaVersion': 1, 'targets': {
            'fixture': {'cases': {'Suite.Required': 'boundary oracle'}}}}))

    def execute(self, *, discovery='Suite.\n  Required\n', xml=None, code=0):
        if xml is None:
            xml = ('<testsuites tests="1" failures="0" disabled="0" errors="0">'
                   '<testsuite tests="1" failures="0" disabled="0" errors="0">'
                   '<testcase classname="Suite" name="Required" status="run" '
                   'result="completed"/></testsuite></testsuites>')
        self.binary.write_text('#!/usr/bin/env python3\nimport sys, os\nfrom pathlib import Path\n'
            'assert not any(k.startswith("GTEST_") for k in os.environ)\n'
            f'if "--gtest_list_tests" in sys.argv:\n print({discovery!r}); sys.exit(0)\n'
            f'xml = {xml!r}\n'
            'if xml != "MISSING":\n'
            ' Path(next(a.split("xml:", 1)[1] for a in sys.argv if a.startswith("--gtest_output="))).write_text(xml)\n'
            f'sys.exit({code})\n')
        self.binary.chmod(0o755)
        verify_run(self.binary, self.inventory, 'fixture', self.root / 'result')

    def test_complete_inventory(self):
        self.execute()
        self.assertEqual(json.loads((self.root / 'result/test-evidence.json').read_text())['status'], 'passed')

    def test_dropped_or_extra_case(self):
        for discovery in ['Suite.\n', 'Suite.\n  Required\n  Surprise\n']:
            with self.subTest(discovery=discovery), self.assertRaisesRegex(ValueError, 'discovery'):
                self.execute(discovery=discovery)
            shutil.rmtree(self.root / 'result')

    def test_nonzero_exit_even_with_passing_xml(self):
        with self.assertRaisesRegex(ValueError, 'execution exit'):
            self.execute(code=1)
        self.assertEqual(json.loads((self.root / 'result/test-evidence.json').read_text())['phase'], 'execution')

    def test_missing_and_malformed_xml(self):
        for xml in ['MISSING', '<broken>']:
            with self.subTest(xml=xml), self.assertRaisesRegex(ValueError, 'XML'):
                self.execute(xml=xml)
            shutil.rmtree(self.root / 'result')

    def test_swallowed_failure_skipped_disabled_zero_and_duplicates(self):
        cases = [
            '<testcase classname="Suite" name="Required" status="run" result="completed"><failure/></testcase>',
            '<testcase classname="Suite" name="Required" status="run" result="skipped"><skipped/></testcase>',
            '<testcase classname="Suite" name="Required" status="notrun" result="suppressed"/>',
            '',
            '<testcase classname="Suite" name="Required" status="run" result="completed"/>' * 2,
        ]
        for case in cases:
            with self.subTest(case=case), self.assertRaisesRegex(ValueError, 'XML'):
                self.execute(xml=f'<testsuites tests="1" failures="0" disabled="0" errors="0"><testsuite tests="1" failures="0" disabled="0" errors="0">{case}</testsuite></testsuites>')
            shutil.rmtree(self.root / 'result')

    def test_environment_cannot_filter_or_shard_required_cases(self):
        with patch.dict(os.environ, {'GTEST_FILTER': 'DoesNotExist.*',
                                     'GTEST_TOTAL_SHARDS': '2', 'GTEST_SHARD_INDEX': '1'}):
            self.execute()
        self.assertEqual(json.loads((self.root / 'result/test-evidence.json').read_text())['executed'], 1)

    def test_missing_target(self):
        self.inventory.write_text('{"schemaVersion":1,"targets":{}}')
        with self.assertRaisesRegex(ValueError, 'target'):
            self.execute()

    def test_stale_output(self):
        (self.root / 'result').mkdir()
        with self.assertRaises(FileExistsError):
            self.execute()


if __name__ == '__main__':
    unittest.main()
