import contextlib
import io
import json
from pathlib import Path
import tempfile
import unittest

from crossover import generate, summarize


class CrossoverTests(unittest.TestCase):
    def test_smallest_sampled_winner_uses_paired_measurements(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for percent, main_ns, indexed_ns in ((.5, 80, 100), (1, 120, 100)):
                for seed in (11, 12, 13):
                    config = dict(name=f'p{percent}s{seed}', cancel_percent=percent, seed=seed,
                                  prefill_orders=20000, measured_commands=20000,
                                  price_levels_per_side=100, max_quantity=100, aggressive_submit_percent=20)
                    for engine, ns in (('deque-baseline', main_ns), ('main', indexed_ns)):
                        report = dict(commit=engine, config=config, trace=dict(actual_cancel_percent=percent),
                                      runs=[dict(mode='batch', repetition=i, elapsed_ns=ns) for i in range(1, 8)])
                        (root / f"{engine}-{config['name']}.json").write_text(json.dumps(report))
            with contextlib.redirect_stdout(io.StringIO()):
                summarize(root)
            result = json.loads((root / 'CROSSOVER.json').read_text())
            self.assertEqual(result['smallest_tested_consistent_indexed_win_percent'], 1)
            self.assertEqual(result['rows'][0]['classification'], 'deque wins')
            self.assertEqual(result['rows'][1]['classification'], 'indexed wins')

    def test_generator_preserves_fractional_ratios_and_seeds(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            generate(root, [.5, .75], [11, 12], 7)
            configs = [json.loads(p.read_text()) for p in root.glob('*.json')]
            self.assertEqual(len(configs), 4)
            self.assertEqual({c['cancel_percent'] for c in configs}, {.5, .75})
            self.assertEqual({c['seed'] for c in configs}, {11, 12})


if __name__ == '__main__':
    unittest.main()
