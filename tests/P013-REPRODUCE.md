# P013 infeasible include elision

Branch `technique/p013-infeasible-include-elision`, exact pending P005 base
`1ace08d901d0f0bf93514af88cf5a473284e18e2`. The only release algorithm change
is the strict candidate-pivot `minnei < bd` branch in `listByCase`: perform the
existing exclusion recursion and recovery without cloning/searching the doomed
include partition. Equality keeps the original include dispatch. No `kSearch`,
count, bound, pivot selection, matrix, cache or scheduling rule changes.
English `technique.md` contains the argument, schedule qualifications and results.

Correctness-only diagnostics verify actual `neiInG=degree(P union C1)` and
`neiInP=degree(P)` at real `listByCase` checkpoints, and assert any parked C2
has exhausted the root's nonneighbor budget. Actual elision restoration checks
active member prefixes/sizes, active pos mappings, membership flags, P/counts,
exclusion pointer and exclusion-stack contents while allowing valid permutations.
Private checked counters start empty in children and merge only after joins.
Release contains none of these counters/checks/allocations.

The direct dispatch harness calls the actual `kSearch/listByCase` methods with
real graphs and independent externally supplied subset-oracle sets. It covers
registered k2/k3 path-joined-clique witnesses while C2 is still selectable, the
strict elision later in the same real search, equality retention, P1/exhausted/
grantable native forks and three repetitions on restored state. It uses the
actual maximality checker; there is no mirrored toy dispatch implementation.

Fresh builds/tests require inventory, workspace status, resource probing and
the coordinator's finite resource window. Run locally on Ubuntu, one compiler
or solver at a time under the original existing lock, <=20 workers (smoke4),
<=160 GiB and <=70% available memory. Ordinary child virtual space is8 GiB;
ASan shadow space must remain unrestricted virtually with an external8 GiB
physical RSS budget. Never replace the lock or rebuild immutable controls.

```bash
PROJECT=/home/joey/k-plex-rethinki-ubuntu
REPO="$PROJECT/worktrees/p013"
OUT="$PROJECT/build/p013-<fresh-revision>"
RUN="$PROJECT/runs/p013-<fresh-revision>"
test ! -e "$OUT"
test ! -e "$RUN"
mkdir "$RUN"
(
  flock -n 9
  make -C "$REPO" -j1 parlay-include-elision parlay-include-elision-emit \
    parlay-include-elision-check-emit parlay-include-elision-dispatch \
    parlay-include-elision-asan parlay-include-elision-dispatch-asan \
    P013_OUT="$OUT" PARLAY_ROOT="$PROJECT/deps/parlaylib"
) 9< "$PROJECT/runs/parallel-exploration-performance.lock"
```

The source must receive Astra's actual-code review before solver execution.
All entries below are components of one correctness phase; each tool acquires
the lock, so do not nest another lock. Use fresh paths and do not repeat unchanged
passed identities without a change, failure or uncertainty.

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
for stage in diagnostic sanitize; do
  python3 "$REPO/tests/p013_validation.py" --project "$PROJECT" --repo "$REPO" \
    --build "$OUT" --output "$RUN/$stage" --stage "$stage"
done
```

Canonical components preserve19-call smoke,2234 independent oracle cases,
relevant boundaries and240 fixed high-worker calls. The technique-local
components each use20 fixed random,6 full witness/equality and9 direct actual
dispatch calls (each direct call asserts three internal repetitions); sanitizer
adds K5/K520/K4100. No standalone real-count sanity phase is required. TSan's
previously established pre-main host incompatibility remains unsupported, not
passed. Expected sets and graph hashes are recorded by the independent oracle.

Performance has separate later grants. Stage2 is the fixed324-call six-solver
screen, three small cases, P1/2/4/8/16/20 and three adjacent repetitions; every
call checks its count. Stage3, only if the screen survives, covers broader
Enron/Wiki/DBLP and all five core/fixed held-outs under finite budgets. Unknown
Pokec/Skitter counts must come from same-input controls, never paper hints or
retired jobs. No held-out tuning or acceptance follows publication automatically.
Results will be appended when the scope closes.


## Published R1 checkpoint: pending evidence

P013 is neither accepted nor rejected. The finite correctness and small-screen
scope completed; phase3 remains UNRUN and ungranted. Preserve all pending source,
builds, generated fixtures, copied/original reference traces and raw evidence.
No rejected-trial cleanup was applied and no additional solver is authorized.

All7072 correctness records passed (4838 fresh calls +2234 verified reference
reuses), including18 actual dispatch calls checking54 internal recovery cycles.
ASan/UBSan/leak checks passed; TSan remains unsupported, not passed. Evidence:
`../../runs/p013-r1-20261026-1808/`. Admitted gate SHA:
`ab27111a8d6514d318affd86f2290ef382701f8fac8201f7aee6c3d5954bc028`.
All44 tested C/C++ files and all six executables remain unchanged from the first
physical build. There were no source fixes, failed tests or repeated components.

All324 small timings and the sole nine-call P16 confirmation completed with
counts verified, retaining all333 raw timings. Small evidence is in
`../../runs/p013-r1-small-20261026-1816/` and its sibling plan directory; the
confirmation is `../../runs/p013-r1-p16-confirm-20261026-1830/` with its sibling
plan directory. Complete six-solver curves, paired ratios, CPU/RSS and order
limitations remain visible. Jazz(4,12) P1 P013/P005 ratios
0.994410/0.990676/0.992671 improve, as do its P2/P4 measurements; the absolute
FP gap persists and does not meet the continuing broad goal.

Jazz(3,10) P16 original P013/P005 ratios1.097567/1.021112/1.096899 were followed
by the fully rotated three-position ratios1.023982/0.960624/0.850556. The latter
are mixed, so the frozen all-original-and-all-new>1 rejection rule does not fire.
The outcome is unresolved, not a performance pass, rejection or automatic phase3
admission. No tolerance, trimming, sample replacement or extra testing follows.
Astra independently audited the full scope and confirmed this pending status.

`p013_confirmation.py` is a byte-identical copy of the reviewed frozen fixed-P
measurement tool. It allows only an explicit one-case/one-P/three-solver/three-
repeat confirmation and retains the original per-call identity/count/runtime/
resource/measurement loop. The full-curve324 driver is unchanged. Fixed-P
confirmation reports self-speedup null/not measured, adding no P1 call and
borrowing no denominator from another batch. `p013_analyze_confirmation.py`
is likewise the exact executed read-only paired analysis source. These tools
and final result documentation are delivered after testing with separate support
identities; they do not alter the tested candidate or executed validation tools.
Publication is a pending checkpoint, without mainline merge or acceptance.
