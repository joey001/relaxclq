# P010 earned-work fork admission

Branch: `technique/p010-earned-work-forks`. Explicit prerequisite: P005
`1ace08d901d0f0bf93514af88cf5a473284e18e2` (not an accepted optimization).
The authoritative argument, trial outcome and limitations are in `technique.md`.
Mainline and the immutable safe reference are unchanged.

## Recorded R1 evidence

The first phase passed: admission and compact-copy units under ASan/UBSan, smoke 19,
fixed 2234 independent-oracle graphs (6702 records:2234 verified reference reuses and
4468 fresh candidate calls), boundaries 38, high-worker exact-set stress 240,
diagnostic 44 and ASan/UBSan/leak checks 47. There are 7090 logical records and 4856
fresh invocations, plus the two units. TSan was not rerun after the previously
recorded pre-main mapping incompatibility; this is not a TSan pass.

The initial standalone compact-test compile omitted `-DTARGET -msse4.2`, selected
an ARM header and exited1. No compact binary was produced. A fresh output with the
correct flags passed; already completed algorithm binaries were reused unchanged.
Original failure logs and both subsequent exit0 controllers remain recorded.

Evidence root: `../../runs/p010-r1-20261026-1323/`.
`correctness-gate.json` SHA256:
`c325701b14f38e350d44cceceeeb46143231097802af3db966a6ebde65b78723`.
Release `MKP` SHA256:
`f592cd7305b15bbed0bdbba739674bc2384acaecde904789872d706bf35675d2`.
Sanitizer+diagnostic+emitter SHA256:
`bca0a2f89bd749839d93d27006579e1994b121e0d4af01ad7cffc5dcc1d595ce`.

A single fixed oracle input, `random-12-0.5-0-k3-q5` at P4, has 102 exact sets with
zero duplicates and grants at all three sites 7/2/6, exclusion alias 7 and after-join
renewal 10. Funded 2742<=metered 5172, attempts 20=grants 15+denials 5 and final reserved 0.
Its input SHA256 is
`d0b1359e0d0d0d11e8250ac832fb835ad2667052a4e81474702e349f2140e684`.
Diagnostic site0/1/2 mean `kSearch`, `branchInCandBase`, `branchInPlexBase`.

The second phase completed all 162 count-verified timings: P010, its P005
prerequisite and the safe reference's same OpenMP binary, three small cases,
P1/2/4/8/16/20 and three interleaved repetitions. Safe P1 is the frozen serial
reference; safe P>1 is a historical OpenMP control, not a Parlay candidate.
`../../runs/p010-r1-small-plan-20261026-1333/plan.final.json` SHA256:
`b99a42dbf921b2175f9de5a17987d836ac141050f2614cff767d8776b008a01e`.
Frozen driver SHA256:
`96b80fd33a5c96e7ff9fb0628f4e5c822717aeb276948ae74e29857b2aa30f54`.
Results, CPU/RSS and paired ratios:
`../../runs/p010-r1-small-20261026-1333/`.
P010 is **rejected** under the preregistered repeatable-regression condition.
For jazz(4,12) at P20, adjacent P010/P005 wall ratios were 1.3648/1.3206/1.4122;
P1 ratios were 1.0313/1.0280/1.0385. CPU regressions agreed; lower RSS did not
compensate for the time regression. Its cause was not isolated. Third-phase
large/core/held-out performance is unrun by justified early rejection. No tuning
or additional tests were performed after closure.

Lean cleanup retained the tested release/emitter/diagnostic binaries, all fresh
raw checks/timings, source snapshots, gates and failure evidence. It removed
rebuildable test binaries, two deterministic K4100 fixtures and duplicate copies
of reference traces, after verifying byte equality with retained originals.
`cleanup-manifest.json` records exact identities; each reused row's `reused_from`
points to the protected original trace.

## Reproduction with a new resource grant

Reading this file does not start jobs. First run inventory/status and probe the
verified Ubuntu development/test host. Obtain the coordinator's finite exclusive
window; no compiler, data preparation or other solver may compete. Use fresh
versioned outputs, the original existing lock (never replace/create it), at most 20
solver workers and at most 160 GiB/70% currently available memory. Correctness smoke
uses 4 workers. Do not rerun identical passed checks without a relevant change,
failure or uncertainty. Never invoke historical `p005_validation.py` or rebuild
immutable controls.

```bash
set -euo pipefail
PROJECT=/home/joey/k-plex-rethinki-ubuntu
REPO="$PROJECT/worktrees/p010"
OUT="$PROJECT/build/p010-<fresh-revision>"
RUN="$PROJECT/runs/p010-<fresh-revision>"
test ! -e "$OUT"
test ! -e "$RUN"
mkdir "$RUN"
(
  flock -n 9
  make -C "$REPO" -j1 parlay-earned-work parlay-earned-work-emit \
    parlay-earned-work-diagnostic parlay-earned-work-unit \
    P010_OUT="$OUT" P010_PROJECT_ROOT="$PROJECT" \
    PARLAY_ROOT="$PROJECT/deps/parlaylib"
  g++ -std=c++17 -DTARGET -msse4.2 -O1 -g \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    "$REPO/tests/p005_compact_smoke.cpp" -o "$OUT/compact-unit"
  ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
    UBSAN_OPTIONS=halt_on_error=1 "$OUT/admission-unit"
  ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
    UBSAN_OPTIONS=halt_on_error=1 "$OUT/compact-unit"
) 9< "$PROJECT/runs/parallel-exploration-performance.lock"
```

`MKP` is the release binary for both P1 and P>1. `MKP-emit` emits exact sets.
`MKP-diagnostic` has shared counters and supplies no performance evidence.
All counters compile out of release. The admission unit is a sequential abstract
fixture; actual native forks/renewals/alias behavior are checked separately on
graphs. F is a 32-bit capacity-word proxy, not actual copied bytes or instructions.

Run the canonical correctness components sequentially. Each driver acquires the
existing lock itself; do not wrap these calls in another flock.

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
python3 "$REPO/tests/p010_validation.py" --project "$PROJECT" --repo "$REPO" \
  --build "$OUT" --output "$RUN/diagnostic" --stage diagnostic
python3 "$REPO/tests/p010_validation.py" --project "$PROJECT" --repo "$REPO" \
  --build "$OUT-sanitize" --output "$RUN/sanitize" --stage sanitize
```

The reference-reuse option verifies binary/input/count/exact-set identity for
every reused row. Remove that option only if a genuinely new reference execution
is required. Sanitizers use a separate fresh build, native fixed Parlay,
ASan/UBSan/leaks and assertions; virtual ASan mappings are not restricted to the
ordinary 8 GiB address-space limit. Actual peak RSS is recorded. The graph checks
start with the 20 fixed random selections and two complement-of-disjoint-star
families, cross-checked against independent subset DP. Diagnostic expansion is
bounded by the existing fixed random pool and stops once coverage is present.
R1 needed no expansion. Sanitizer exact sets reproduce the observed coverage.

After correctness admission, `tests/p010_performance.py --plan PLAN --out NEW_OUT`
prints the timing plan without launching solvers. `--run` additionally requires
an explicit finite exclusive grant in the plan. The R1 final plan illustrates
pinned source/binary/gate/input identities and budgets. Complete blocks contain
all three solvers for one case/worker/repetition; seeded block shuffling and
rotating solver position give adjacent controls and balanced order over three
repetitions. Each invocation verifies input/binary SHA and output count, and
records wall, user/system CPU (including native scheduler spinning), RSS and
observed native worker count. Same-binary P1 is the self-speedup denominator. Keep all outcomes;
missing/timeout/OOM/error evidence is not a pass. The third phase requires its
own finite grant and includes all canonical difficult core/held-out cases before
positive acceptance. No held-out tuning or mandatory standalone real-count round.

The admission proof covers normal completion, all suspended ancestor snapshots
and the actual augmented atomic/fork/join DAG. The pinned native runtime does not
provide exception-safe joins after dispatch. The unit's unwind fixture is strictly
pre-dispatch; there is no catch-and-continue or partial-result recovery. A global
P-1 clone cap can deny useful splits. Saturation discards surplus; every funded
attempt consumes credit even when denied; child credit is zero and no credit is
merged or restored after joins. These limitations remain part of the trial.
