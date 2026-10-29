import json
from pathlib import Path
import tempfile
import unittest

from run import Reference, make_trace


class ReferenceTests(unittest.TestCase):
    def test_price_time_and_partial_fill(self):
        ref = Reference()
        ref.submit(1, 101, 4)
        ref.submit(1, 100, 3)
        ref.submit(1, 100, 5)
        ref.submit(0, 101, 6)
        self.assertEqual(ref.orders, {1: [1, 101, 4], 3: [1, 100, 2]})
        self.assertEqual(ref.fills, 2)
        ref.cancel(3)
        ref.submit(0, 101, 10)
        self.assertEqual(ref.orders, {5: [0, 101, 6]})
        self.assertEqual(ref.ids, [5])

    def test_sell_matches_highest_bid(self):
        ref = Reference()
        ref.submit(0, 100, 4)
        ref.submit(0, 101, 3)
        ref.submit(1, 100, 5)
        self.assertEqual(ref.orders, {1: [0, 100, 2]})
        self.assertEqual(ref.fills, 2)

    def test_configs_generate_exact_ratios_and_repeatable_traces(self):
        for path in sorted((Path(__file__).parent / 'configs').glob('*.json')):
            config = json.loads(path.read_text())
            config.update(prefill_orders=200, measured_commands=100)
            with tempfile.TemporaryDirectory() as temp:
                first, second = Path(temp) / 'a', Path(temp) / 'b'
                stats = make_trace(config, first)
                self.assertEqual(stats, make_trace(config, second))
                self.assertEqual(first.read_bytes(), second.read_bytes())
                commands = first.read_text().splitlines()[1 + config['prefill_orders']:]
                self.assertEqual(sum(c.startswith('C') for c in commands), config['cancel_percent'])
                self.assertGreater(stats['active_min'], 0)


if __name__ == '__main__':
    unittest.main()
