# Historical measurement harness

These files preserve the harness and analysis scripts as they existed when the LOBSTER and crossover experiments were measured. Their historical engine labels were `main` for the deque version and `benchmark` for the indexed version. Those labels are retained here only to preserve the recorded source hashes.

The published reports now label the same measured source commits as `deque-baseline` (old) and `main` (new). No timings or correctness digests were changed during relabeling. Current runnable tooling lives one directory above and uses those new labels with pinned source commits. These archived scripts are provenance copies, not standalone entry points; they assume their original location under `benchmarks/`.
