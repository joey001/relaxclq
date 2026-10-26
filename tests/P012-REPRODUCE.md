# P012 lazy saturation prefixes

Branch `technique/p012-lazy-saturation-prefix`; exact prerequisite
`1ace08d901d0f0bf93514af88cf5a473284e18e2` (P005, pending and unaccepted).
The authoritative argument and eventual decision are in English `technique.md`.
The change preserves counts, pair matrix, pivots, bounds, branch partitions,
root scheduling, fork credits and the global maximality checker.

Each listor owns a lazily grown `saturationScratch` vector. Four fixed-P passes
create a noncopyable scope-local prefix cursor, clearing only the logical cache.
The two original cheap tests read current counts before touching the prefix.
Cached saturated vertices retain P order; new discoveries are appended before
their possible adjacency rejection. The P=G helper ends before pointer/size
restoration. Children start with empty independent vectors and strictly join.
There is no threshold, eager full-P scan, TLS cache or new pool.

All diagnostics compile out of release. The check-emitter compares every actual
query to the original predicate and checks the prefix invariant, per-query
`cached visits + new P scans <= original P scans`, and zero cache work/capacity
growth after cheap failure. Task-private checked counters merge only after joins.
Actual fork diagnostics check empty child scratch and distinct live backing
storage. The ASan/UBSan unit exercises ordered new/replayed rejection, extension,
live G changes, reset, lazy growth, actual P=G restoration and real native forks.

Fresh execution requires inventory, workspace status, resource probing and the
coordinator's finite exclusive window. Use local Ubuntu tmux, the existing lock,
one compiler/solver at a time and <=20 workers. Whole-batch memory is <=160 GiB
and <=70% currently available. Ordinary children have an 8 GiB address-space
limit. ASan needs its shadow virtual address space; retain measured RSS and enforce
the same 8 GiB physical budget externally. Never replace the lock or rebuild controls.

```bash
PROJECT=/home/joey/k-plex-rethinki-ubuntu
REPO="$PROJECT/worktrees/p012"
OUT="$PROJECT/build/p012-<fresh-revision>"
RUN="$PROJECT/runs/p012-<fresh-revision>"
test ! -e "$OUT"
test ! -e "$RUN"
mkdir "$RUN"
(
  flock -n 9
  make -C "$REPO" -j1 parlay-saturation-prefix parlay-saturation-prefix-emit \
    parlay-saturation-prefix-check-emit parlay-saturation-prefix-unit \
    P012_OUT="$OUT" PARLAY_ROOT="$PROJECT/deps/parlaylib"
  PARLAY_NUM_THREADS=4 ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
    UBSAN_OPTIONS=halt_on_error=1 taskset -c 0-3 "$OUT/prefix-unit"
) 9< "$PROJECT/runs/parallel-exploration-performance.lock"
```

The following entries are internal components of one correctness phase. The
generic scripts acquire the lock themselves, so do not nest another lock.
Unchanged verified reference traces are reused, without rebuilding controls.

```bash
for stage in smoke exhaustive boundaries stress; do
  extra=()
  if [ "$stage" = exhaustive ]; then
    extra=(--reuse-reference-from "$PROJECT/runs/p005-correctness-20261025/exhaustive")
  fi
  if [ "$stage" = stress ]; then extra=(--high-worker-authorized); fi
  python3 "$PROJECT/tools/experiments/candidate_validation.py" \
    --repo "$REPO" --candidate "$OUT/MKP" --emit "$OUT/MKP-emit" \
    --output "$RUN/$stage" --stage "$stage" "${extra[@]}"
done
python3 "$REPO/tests/p012_validation.py" --project "$PROJECT" --repo "$REPO" \
  --build "$OUT" --output "$RUN/diagnostic" --stage diagnostic
python3 "$REPO/tests/p012_validation.py" --project "$PROJECT" --repo "$REPO" \
  --build "$OUT-sanitize" --output "$RUN/sanitize" --stage sanitize
```

Diagnostic and sanitizer components use the fixed 20 random and two star
fixtures, then only a finite existing fixed-pool subset if a required diagnostic
path is still absent. All calls check exact sets/counts, duplicates and workers.
TSan's previously established pre-main host incompatibility is unsupported,
not passed. Do not repeat identical passed checks without relevant changes,
failure or uncertainty.

The next phase needs its own grant: fixed jazz(3,10), GrQc(3,10), jazz(4,12),
P1/2/4/8/16/20, three adjacent-block repetitions and six solvers (P012/P005/
safe ListPlex/FP O3/PlexEnum O3/separate author-default Ofast), initially324 calls.
`p012_performance.py --plan PLAN --out NEW_OUT` prints a plan; `--run` requires
the grant and identity-pinned correctness gates. Its frozen queue rotates solver
positions without claiming exhaustive six-position balance from three repeats.
Every invocation checks its count and identities and records wall/CPU/RSS.
`p012_audit_performance.py` is read-only and checks raw evidence/queue/identities.
Author thread fields are configuration; Parlay observes runtime workers, while
external OS thread samples are separate. Same-binary P1 is each self-speedup
denominator. Count checks are integrated, without a separate mandatory sanity pass.

The third phase requires its own finite budgets and grant, covering broader
Enron/Wiki/DBLP and all five core/fixed held-outs. Unknown Pokec/Skitter counts
must be established there from same-input controls, never paper hints or retired
jobs. No held-out tuning or automatic acceptance follows publication. Final
results and retained evidence will be appended here when the scope closes.


## Final R1 result: rejected

Correctness passed7090 logical records (4856 fresh calls +2234 verified reused
reference traces), plus the ASan/UBSan/leak actual-cache/fork unit. Correctness
raw evidence is in `../../runs/p012-r1-20261026-1713/`; final admitted gate SHA
`3a2dc5139e787b6b00f96b5eb39b5ad7ab0ecd4f6c1efaa7a4ba0d242157170e`.
The source and release stayed fixed throughout all tests. The unit-only initial
compile/synthetic-fixture corrections happened before execution; physical-source
provenance records them. TSan remains unsupported, not passed.

All324 small-screen and the single nine-call rotated confirmation timings
completed with exact counts. Evidence is in `../../runs/p012-r1-small-20261026-1717/`
and `../../runs/p012-r1-p1-confirm-20261026-1733/`, with sibling plan directories.
The frozen screen's copied descriptive grant/ready/PP fields are corrected by its
`metadata-erratum.json`; actual grant/gates/queue/raw observations remain valid.
The confirmation's derived analysis scope correction is recorded separately.

Astra confirmed the frozen rejection rule: jazz(3,10) P1 P012/P005 original wall
ratios1.010016/1.015043/1.008867 and new rotated ratios
1.017472/1.035348/1.017377 all exceed1. No tolerance or sample removal was added.
The new medians were0.413029s P012,0.402629s P005 and0.397352s Safe. Favorable
jazz4 P20 results remain in the full curves; they do not erase this repeated P1
regression. Phase3 is UNRUN by justified early rejection, not acceptance with
missing evidence. No further tests or tuning follow this trial.

Lean cleanup removes copied reference stdout/stderr/resource files only after
hash checks against protected originals, generated K4100 fixtures and rebuildable
sanitizer artifacts. Compact raw outcomes, physical source/commands/provenance
and all333 timing records remain; `cleanup-manifest.json` records exact identities.
The retained release/emitter/check-emitter are unchanged. Removed artifacts use
fresh paths if rebuilt; do not rerun unchanged passed checks merely to reproduce
the audit. `p012_analyze_performance.py` is a post-test read-only publication
helper; it computes paired ratios and complete wall/CPU/RSS/self-speedup curves
from existing measurements and labels three- versus six-solver scope correctly.
