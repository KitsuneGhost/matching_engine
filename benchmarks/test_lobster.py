import json
from pathlib import Path
import tempfile
import unittest
import zipfile

from analyze_lobster import analyze
from run import make_trace


class LobsterTests(unittest.TestCase):
    def test_denominators_and_partial_cancellation(self):
        rows = ['1,1,10,100,10000,1', '2,2,10,20,10000,1',
                '3,3,10,80,10000,1', '4,1,11,100,10100,-1',
                '5,4,11,10,10100,-1', '6,5,0,10,10100,-1',
                '7,6,0,10,10100,-1', '8,7,0,0,-1,-1']
        with tempfile.TemporaryDirectory() as temp:
            p = Path(temp) / 'LOBSTER_SampleFile_TEST_2012-06-21_10.zip'
            with zipfile.ZipFile(p, 'w') as archive:
                archive.writestr('TEST_message_10.csv', '\n'.join(rows))
            result = analyze(p)
            self.assertEqual(result['cancellation_event_percent'], 25)
            self.assertAlmostEqual(result['full_delete_supported_command_percent'], 100 / 3)
            self.assertEqual(result['combined_cancel_add_command_percent'], 50)
            self.assertEqual(result['observed_submissions_with_later_cancel_percent'], 50)
            self.assertEqual(result['counts']['2'], 1)
            self.assertEqual(result['first_time_seconds'], '1')
            self.assertEqual(len(result['message_sha256']), 64)

    def test_fractional_ratio_rounds_down_and_zero_is_valid(self):
        config = dict(name='fraction', cancel_percent=.125, seed=9, prefill_orders=200,
                      measured_commands=1000, price_levels_per_side=10, max_quantity=10,
                      aggressive_submit_percent=0, repetitions=2)
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'trace'
            stats = make_trace(config, path)
            self.assertEqual(stats['cancellation_commands'], 1)
            self.assertEqual(stats['actual_cancel_percent'], .1)
            config['cancel_percent'] = 0
            self.assertEqual(make_trace(config, path)['cancellation_commands'], 0)


if __name__ == '__main__':
    unittest.main()
