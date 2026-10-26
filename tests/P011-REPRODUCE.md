# P011 minority-row updates with private offsets

Branch `technique/p011-minority-row-updates`; exact prerequisite
`1ace08d901d0f0bf93514af88cf5a473284e18e2` (P005, pending and unaccepted).
The complete argument, retained limitations and final decision belong to the
English `technique.md`. P011 changes count representation only. P005 root
scheduling, fork credits, partitions, pivots and maximality decisions are retained.

`ShiftedCounts` reads `raw[v]+bias`. Each `UpdateRows` is owned by one root until
all descendants strictly join. Sparse rows borrow adjacency; selected dense rows
own the closed complement, including the vertex itself. The threshold is
`degree > b-degree`; ties use adjacency. Complement construction is fused with
existing matrix materialization. Clones copy both raw arrays and biases and share
the immutable table. The temporary P=G maximality check substitutes/restores the
whole count view. No new tuning parameter, scheduler or shared atomic is added.

## R1 correctness evidence

All 7090 logical records passed: 4856 fresh calls and 2234 identity-verified
reference reuses, plus the new ASan/UBSan/leak differential/actual-checker unit.
Components: smoke 19, fixed 2234-graph oracle 6702, boundaries 38, fixed high-worker
exact sets 240, diagnostic 44 and sanitizer 47. Peak recorded RSS: 294088 KiB.
TSan's previously recorded pre-main host mapping incompatibility was not rerun;
this is not a TSan pass or race-freedom claim.

Evidence root: `../../runs/p011-r1-20261026-1557/`.
Gate SHA256:
`682be62f7b1f5f3a06796b0fccf0635c6465e3067963cfff1b75af4ad309c4ac`.
Release SHA256:
`a58ca5b1d483a3f11179d2605b02ddfd1fd0c8d201878436b3ccfeee0b545511`.
Sanitizer+diagnostic+emitter SHA256:
`7103646cbb32e91ef0b696fd54c5d07b747bace2f64100b7ccf61249aad1da19`.

The initial build completed. Before any tests, review added diagnostic-only child
logical-copy and root G-bias restoration assertions and an overflow unit fixture.
Fresh diagnostic/check-emitter/unit outputs were built in
`../../build/p011-r1-reviewfix-20261026-1604/`; initial versions were superseded
and unrun. Original release/emitter outputs were reused only after complete
preprocessed bytes matched for both affected translation units in both modes.
`release-source-provenance.json`, original/final source snapshots and the two
preprocessing manifests distinguish physical compilation from equivalence reuse.

Diagnostics compare every update against independent original-adjacency updates,
verify actual maximality-checker inputs against explicit adjacency, compare
parent/child logical arrays before dispatch, and verify whole-view restoration.
Counters are task-private and merge after joins; overflow visibly invalidates
cumulative statistics. All instrumentation compiles out of release. The 44
exact-set calls observed 353968 old eligible row entries versus 89206 actual
entries, with 1084 nonzero P-bias and 534 nonzero G-bias forks. These counters
are validation evidence, not a whole-program work bound or performance result.

The fixed random-12-0.5-0-k3-q5 input at P4 has 102 exact sets and no duplicates;
its sanitizer run exercised 11 P-bias forks, 7 G-bias forks and 54 biased whole-view
substitutions with 129 restores. Input SHA256:
`d0b1359e0d0d0d11e8250ac832fb835ad2667052a4e81474702e349f2140e684`.
The direct unit also covers empty/singleton, odd/even universes, exact-half ties,
all H prefixes, inverse/recovery, independent clones, raw G reaching +b on clique
removal, the actual saturated-left-checker counterexample, and stats overflow.

## R1 performance decision

P011 is **rejected** under the rule fixed before its only finite confirmation.
All 324 six-solver screen timings and the additional nine P1 confirmation timings
completed with the expected counts. The confirmation used only jazz(4,12), P1,
P011/P005/Safe and three balanced positions. New P011/P005 wall ratios were
1.016100/1.015291/1.014076, with CPU regressions in the same direction. Together
with the original three ratios, this met the frozen repeatable-regression rule.
No percentage tolerance was added. A P16 improvement remains visible in the
complete screen; it does not erase the repeated P1 regression.

Performance evidence:
`../../runs/p011-r1-small-20261026-1610/` and
`../../runs/p011-r1-p1-confirm-20261026-1620/`.
Frozen plans/driver and outer exits are in their corresponding `-plan-` directories.
The third phase is unrun by justified early rejection. No further tests or tuning
followed. This trial did not meet the objective against both authors: jazz(4,12)
P20 gave P011 16.99x self-speedup versus FP 17.52x and absolute wall 0.2842s versus
0.1961s. The measurements do not identify the cause of P011's regression.

Lean cleanup removed deterministic K4100 fixtures, byte-identical copied
reference outputs and rebuildable/superseded unit, sanitizer and unexecuted
full-main diagnostics. It retained the original release (also reserved for a
separate future research profile), emitter and actual diagnostic check-emitter,
all fresh raw evidence, original physical/final source snapshots and PP reuse
provenance. `cleanup-manifest.json` records exact removed identities and retained
original reference paths. No research profile is part of this trial.

## Fresh reproduction

First run project inventory, workspace status and resource probing. New builds
and solvers require the coordinator's finite exclusive window; reading this file
launches nothing. Use local tmux, one compiler/solver at a time, at most 20 solver
workers and memory at most 160 GiB and 70% currently available. Ordinary
correctness cases use an 8 GiB address-space budget; ASan mappings need a separate
fresh build without that virtual-address restriction, with actual RSS recorded.
Never replace/create the coordination lock or rebuild immutable controls.

```bash
set -euo pipefail
PROJECT=/home/joey/k-plex-rethinki-ubuntu
REPO="$PROJECT/worktrees/p011"
OUT="$PROJECT/build/p011-<fresh-revision>"
RUN="$PROJECT/runs/p011-<fresh-revision>"
test ! -e "$OUT"
test ! -e "$RUN"
mkdir "$RUN"
(
  flock -n 9
  make -C "$REPO" -j1 parlay-minority-rows parlay-minority-rows-emit \
    parlay-minority-rows-diagnostic parlay-minority-rows-check-emit \
    parlay-minority-rows-unit P011_OUT="$OUT" \
    P011_PROJECT_ROOT="$PROJECT" PARLAY_ROOT="$PROJECT/deps/parlaylib"
  ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
    UBSAN_OPTIONS=halt_on_error=1 "$OUT/counts-unit"
) 9< "$PROJECT/runs/parallel-exploration-performance.lock"
```

Do not rerun unchanged passed identities without a change, failure or uncertainty.
The canonical components acquire the original lock themselves; do not nest flock.

```bash
for stage in smoke exhaustive boundaries stress; do
  extra=()
  if [ "$stage" = exhaustive ]; then
    extra=(--reuse-reference-from \
      "$PROJECT/runs/p005-correctness-20261025/exhaustive")
  fi
  if [ "$stage" = stress ]; then extra=(--high-worker-authorized); fi
  python3 "$PROJECT/tools/experiments/candidate_validation.py" \
    --repo "$REPO" --candidate "$OUT/MKP" --emit "$OUT/MKP-emit" \
    --output "$RUN/$stage" --stage "$stage" "${extra[@]}"
done
python3 "$REPO/tests/p011_validation.py" --project "$PROJECT" --repo "$REPO" \
  --build "$OUT" --output "$RUN/diagnostic" --stage diagnostic
python3 "$REPO/tests/p011_validation.py" --project "$PROJECT" --repo "$REPO" \
  --build "$OUT-sanitize" --output "$RUN/sanitize" --stage sanitize
```

These stage names are internal components of one correctness phase. No standalone
real-count sanity round is required. Candidate-local drivers import the canonical
oracle/runner; historical candidate validators with fixed build paths are not used.

The second phase uses the fixed jazz(3,10), GrQc(3,10), jazz(4,12), workers
1/2/4/8/16/20 and three adjacent-block repetitions, comparing P011, exact P005,
safe ListPlex, FP O3, PlexEnum O3 and separate author-default Ofast: 324 calls.
Three repetitions vary six solver positions without exhaustive position balance.
`p011_performance.py --plan PLAN --out NEW_OUT` only prints a plan; `--run`
requires a separate explicit finite grant and pinned correctness gate. It checks
source/binary/input/gate/provenance identities and every output count. Safe P1 is
the frozen serial reference; safe P>1 is the historical OpenMP control. FP's exit1
is accepted only with a completion marker and exact count. FP/PlexEnum printed
threads are configuration, not observed runtime workers; external OS-thread
samples are separate. PlexEnum uses its Parallel executable at P1 too, and the
author-default tau=0.1 ms stays unchanged. Preserve all compiler flag identities.

`p011_audit_performance.py` audits the frozen queue, complete and unique summary
keys, raw counts/resources/workers, source/input/binary identities and outer exit0.
It launches no solver and never rewrites measurements. Its integrity result does
not itself establish a performance improvement. Report absolute wall, P1 cost,
same-binary self-speedup, user/system CPU including native scheduler spinning,
RSS, and comparisons against both unchanged authors. Beating P005 alone does not
meet the continuing objective. Positive acceptance requires complete performance data and Astra review;
phase 3 would include the broader Enron/Wiki/DBLP cases and all five
core/fixed held-outs under a separate finite grant. Unknown Pokec/Skitter counts
must be established there with fresh jobs, not paper hints or retired processes.
