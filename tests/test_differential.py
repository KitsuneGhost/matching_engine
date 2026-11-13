"""Compare every operation with the existing independent Python reference model."""
from pathlib import Path
import random
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'benchmarks'))
from run import Reference


def compare(binary, seed):
    rng = random.Random(seed)
    reference = Reference()
    commands, expected = [], []
    for _ in range(500):
        if reference.ids and rng.random() < 0.3:
            oid = rng.choice(reference.ids)
            commands.append(f'C {oid}')
            reference.cancel(oid)
        else:
            side, price, quantity = rng.randrange(2), rng.randrange(95, 106), rng.randrange(1, 21)
            commands.append(f'S {side} {price} {quantity}')
            reference.submit(side, price, quantity)
        expected.append(f'{reference.digest.value} {reference.state_digest()}')
    result = subprocess.run([binary], input='\n'.join(commands) + '\n',
                            text=True, capture_output=True, check=True)
    actual = result.stdout.splitlines()
    if len(actual) != len(expected):
        raise AssertionError(f'seed {seed}: expected {len(expected)} steps, got {len(actual)}')
    for step, (observed, wanted) in enumerate(zip(actual, expected), 1):
        if observed != wanted:
            raise AssertionError(f'seed {seed}, step {step}, command {commands[step - 1]}: '
                                 f'expected digests {wanted}, got {observed}')


if __name__ == '__main__':
    for seed in (7, 42, 20261108):
        compare(sys.argv[1], seed)
    print('PASS: 3 seeds, 1,500 operations; execution and complete order-state digests agree after every step')
