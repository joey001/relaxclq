# ListPlex Parallel Techniques

## Scope and current state

This is the English experiment ledger on the repository mainline, **`armAndX86`** (verified as the remote default branch on 2026-10-25). Do not rename it to `main` or treat a technique branch as the ledger's authoritative copy.

Future candidate algorithms use **the native ParlayLib scheduler** from [joey001/parlaylib](https://github.com/joey001/parlaylib.git), initially pinned to `51017699dcc421f80479cdb238d3092233ad0d26`. OpenMP, Cilk/OpenCilk, TBB, and independent thread-pool backends are outside the candidate implementation contract. The archived ListPlex reference and unchanged author comparators retain their historical runtime. This policy change does not port the solver yet.

[GBBS](https://github.com/ParAlg/gbbs) is a useful source of Parlay graph-algorithm patterns; it is neither a mandatory dependency nor the boundary of permitted ideas. Its `gbbs/bridge.h` exposes Parlay loops and fork-join operations. GBBS currently pins a different Parlay revision: inspect compatibility instead of silently replacing the selected fork. Preserve source attribution and applicable licenses for adapted code.

**P001 has completed correctness validation and396 timed runs; its current sparse-state route is rejected for repeatable regressions. P002/P003 remain unimplemented.** Existing hardening and OpenMP measurements are historical evidence, not experiments on the proposals below. P001 has a retained implemented branch; P002/P003 are reserved and an empty branch is not an attempted implementation. No graphs are downloaded and no experiments are launched by maintaining this ledger.

## Authorized algorithm adaptations

The frozen serial archive stays immutable as an independent control. **Candidate branches may change local relaxclq implementation and algorithm logic when this makes the design more parallel-friendly.** This includes pivot selection, branching, pruning, search/task partitioning, recovery and the implementation of maximality checks. Retain the enumeration problem and result-set semantics: the same `(G,k,q)` convention and complete, duplicate-free globally maximal k-plexes. Source-level identity, an identical search tree and identical node counts are not required.

Necessary parallel-motivated local changes are authorized within the technique; no additional permission is needed solely because an algorithm condition changes. One main technique can include its necessary algorithm adaptations. Explain the original parallel obstacle, precise changes and correctness argument here, add targeted oracle/counterexample regressions, and separate resulting algorithmic work changes from scheduler speedups. Unrelated optimizations remain separate techniques. The Parlay, small-and-sweet, low-parameter and generalization contracts still apply.

## The small-and-sweet acceptance contract

1. **Small:** one clear idea per branch, short understandable invariants, and native Parlay primitives where they suffice. Prefer no new tuning parameters. Worker count and problem inputs `(G,k,q)` are resource/input settings, not algorithm tuning knobs. Any unavoidable algorithm constant needs one portable default and a structural justification. No graph-name dispatch, hardware-specific threshold table, per-instance timer tuning, or unbounded feature combinations.
2. **Work-efficient under comparable bounds:** preserve the enumeration problem/output semantics while allowing the local algorithm adaptations above. Establish that candidate asymptotic work is no worse than the reference bound under explicitly common problem parameters, cost model and guarantee type, not merely that one large test runs faster. An input-by-input `W_candidate(G)=O(W_ref(G))` guarantee is stronger and needs a separate proof; it is not a blanket requirement to keep the same search tree. Charge initialization, state copies, scans/pack/sort, matrix construction, allocation, synchronization and counting. In particular, an extra `P*n` initialization or a log factor cannot be hidden as "scheduler overhead". State expected/amortized assumptions and constants honestly.
3. **Span-aware:** define the task DAG and weighted critical path. State the remaining serial phases and the largest unsplit subtree. Distinguish an algorithmic span bound from a scheduler running-time guarantee. A claim such as `E[T_P] = O(W/P + S)` requires appropriate strict fork-join and scheduling assumptions; it is not an unconditional guarantee for arbitrary tasks or a VM.
4. **General:** use one default policy across predeclared inputs and worker counts. Compare candidate one-worker overhead against the frozen reference and candidate multi-worker performance against the same-worker accepted implementation. For P001, compare the archived parallel implementation as an explicitly labelled legacy control. A persistent clear regression on another case or worker count fails the gate even when aggregate speedup improves.
5. **Measured:** complete correctness gates and report all selected cases, including failures, timeouts and slowdowns. Use at least three interleaved repetitions, median and variation, and additional paired runs to resolve suspected regressions. Unstable, missing or unprepared cases are pending evidence; they cannot establish generalization. Timings shorter than the noise floor cannot establish an improvement.

Development cases and held-out cases are fixed in `tests/test-suites.json`. The held-out core cases are `com-dblp(4,12)`, `soc-pokec(4,20)`, and `as-skitter(3,60)`; do not tune on their results and continue describing them as held out. Every acceptance report covers all five core cases and the intermediate checks, at one worker and the planned multiple-worker counts. Claims about large-graph scheduling or memory additionally need predeclared large-graph cases. No missing graph may be quietly replaced by an easier input.

## Branch and decision workflow

- Maintain policy, this ledger and final decisions on `armAndX86`. Implement each technique only on its dedicated `technique/<id>-<name>` branch. A materially different main idea or a combination of independent techniques gets a new ID and branch; necessary local algorithm adaptations may belong to their parent technique. Shared prerequisites must be recorded; do not mix unaccepted techniques silently.
- Before implementation, record the base commit, hypothesis, work/span analysis, ownership invariants, parameters, development/held-out split, comparison baselines and bounded experiment plan here. Create/reserve the branch from the mainline. Switch only with a safe working tree; preserve unrelated work and never reset it to start a trial.
- Put implementation and run evidence on the technique branch. Return a concise English result, implementation commit, exact dependency version and evidence paths to the mainline ledger. Each update to the ledger belongs on the mainline, not solely on the trial branch. There is no implicit authorization to push or discard branches.
- `planned -> in_progress -> accepted` requires **both** theoretical and experimental small-and-sweet gates. Record each gate independently. "Accepted" closes development for this technique; follow-up changes use a new entry or an explicitly reopened trial with new evidence.
- If a completed attempt cannot satisfy the work/simplicity or generalization gate, mark **`rejected`** on the mainline: state which gate failed, concrete counterevidence, the branch/commit and why this route is not viable. Keep the branch and logs; do not delete unfavorable results or keep adding per-case knobs.
- `pending_evidence` or `blocked` is non-terminal and explains absent data, environment or measurements. It is not success and is not evidence of algorithmic failure. A theoretical rejection can stop an experiment before expensive runs; the experimental section then explicitly says "not run: theory gate failed".

## Analysis notation

For a fixed normalized input and `(k,q)`, let `R` be the number of independent roots, `W_pre` the reference preprocessing work, `W_i` the complete serial work of root `i` (including its block construction), and `W_ref = W_pre + sum_i W_i`. Let `S_pre` be the candidate preprocessing span. `C` denotes **all additional** copy/initialization work, `N_t` the number of tasks, and `D_i` the weighted critical path inside root `i`, including state creation, joins and counting. `G` denotes resident graph/preprocessing space and `b_i` a block size. Also let `W_local(G,k,q)` be the serial work of the candidate algorithm after any deliberate local changes, and `B_ref(n,m,k,D,...)` a stated reference asymptotic upper bound under the chosen model. A changed search tree uses `W_local` for its scheduling accounting and must justify `W_local + overhead = O(B_ref)` (or a tighter comparable bound). Do not substitute actual per-instance `W_ref` for `B_ref` without proving the stronger claim. Bounds below are proposed obligations, not completed proofs.

## Technique registry

| ID | Technique | Dedicated branch | Status | Theory gate | Experiment gate |
| --- | --- | --- | --- | --- | --- |
| P001 | Native Parlay root parallelism | `technique/p001-parlay-roots` | rejected: sparse-state route | conditional average-hash only | failed: repeatable regressions |
| P002 | Bounded fork-join splitting of heavy subtrees | `technique/p002-bounded-fork-join` | planned; branch reserved | not validated | not run |
| P003 | Shared immutable blocks with compact task state | `technique/p003-shared-task-state` | planned; branch reserved | not validated | not run |

### P001 - Native Parlay root parallelism

**Decision (2026-10-25): REJECTED for the current root-private sparse-state route.** Correctness passes the declared gates, but repeatable serial and parallel regressions violate small-and-sweet generalization. This rejects this implementation path, not Parlay or root parallelism as a whole. No parallel implementation is merged into the mainline. P002/P003 remain unimplemented; neither may call this an accepted control.

**Identity.** Branch `technique/p001-parlay-roots`; base `f85c6e833e7f67330a684a0fe894620e874eb988`; port `cc107ae2e08dde256260046be76588e96e1ece06`; isolated helper repair `0c6a5eacedf10b7ed1185fe28e93e888199ff433`; tested source/harness HEAD `2969a9ee8c2a597c735a07147012b6df9587a1d2`. Native Parlay fork revision `51017699dcc421f80479cdb238d3092233ad0d26`, codeload archive SHA `4f2fc759c1ca474db2f45585ee2300997a128d48fba79bf47aaa579aa8858ee8`; source manifest in `../deps/parlaylib-source-manifest.json`. GCC11.4, candidate C++17/pthread/O3/DNDEBUG/march=native/msse4.2/TARGET, no OpenMP backend; observed native workers equal every request. No new algorithm tuning knobs; grain one is structural for irregular roots.

**Controls and independent safety repair.** The original frozen archive `ae76855e71a296f233fab5322bcf9b6ec07fca07b35e619b895cf0e386f7e104` remains immutable. K4100 UBSan found inherited signed overflow and bool/int aliasing in packed flag counting; replace it with a direct bool sum, without changing enumeration. Both timed programs use that repair. Safe reference source archive `c5c06a6f3f69c23e268adb6d7c956439fd5d6f40075b6df22eddf5252a1b43b2` overlays only sequence.h on the frozen source and is separately pinned by `tests/baselines/listplex-pack-safe-2026-10-25.json`. Its single-thread binary is the reference; the same binary at P>1 is labelled legacy OpenMP control. It is not FP/PlexEnum and no external author's implementation was changed or remeasured here. The validated helper-only repair was independently adopted on mainline as `20bf08c`; the Parlay port remains unmerged.

**Actual decomposition and correctness.** The candidate completes core peeling/rebuild and the degeneracy order before a grain-one `parlay::parallel_for` over roots. Every callback owns its sparse graph-index maps, local dense block/matrix, search/exclusion/recovery state, checker and emitter; graph/order arrays are immutable. Each root writes one independent 64-bit result, then the caller sums after the join. There is no mutable TLS and no callback spawns/yields inside serial search. The minimum degeneracy vertex partitions results completely and without overlap. For q>=2k-1, nonadjacent vertices of a qualifying k-plex have at least q-2k+2>=1 common neighbors, supporting the inherited two-hop decomposition; local exclusions and complete left candidates retain global maximality checks. CLI processes are independent; simultaneous library invocations are not supported because k/q/time parameters remain global. Diagnostic ID emission uses a mutex and is excluded from timing.

**Work and assumptions.** For normalized binary input let n,m be graph sizes, R<=n retained roots, A_i the graph-index scratch accesses, b_i/l_i the right/left block sizes, E_i=sum_{v in B_i}deg(v)+b_i(b_i+l_i) the adjacency/materialization budget, C_i common-neighbor work, and H_i recursive search/maximality work with complete left candidates. In a RAM model with average/amortized O(1) hash operations and linear allocator cost:

`W = O(n+m+R + sum_i(A_i+E_i+C_i+H_i))`.

Each map operation corresponds to a scratch access; insertion, rehashing and destruction are charged to inserted keys, with no reserve(n). There is no eager P*n or R*n zeroing. The fresh b_i^2 matrix zeroing is charged: both programs unconditionally materialize every matrix cell in buildSubgraph before deciding proper. Local sets and checker initialization are covered by block work. The isolated Boolean-sum repair remains Theta(n) work/span and O(1) extra space.

This is a **conditional average-hash bound**, not an unconditional or randomized-expected work-efficiency proof: std::unordered_map<int,...> uses deterministic hashing and collision-heavy keys can increase scratch work. Also, the reference leaves marks from early-skipped roots in worker-local visited, incidentally removing safe left candidates later. Private scratch clears those residual marks. Correctness is preserved, but some left/maximality work can grow: compare common complete-left-candidate bounds, never claim per-instance W_candidate=O(actual W_reference) or identical node counts. The work gate is therefore conditional, not an unconditional pass.

**Span.** For binary input the complete serial preprocessing has O(n+m) work/span. Include serial result initialization/reduction, grain-one binary fork/join and complete root lifetimes:

`S = O(n+m+R+log(R+1)+max_i W_i) = O(n+m+max_i W_i)`.

One large root remains indivisible and preprocessing remains serial. Text loading adds its separately charged sorting/remapping. Under the fully strict randomized work-stealing model with constant-cost primitives, nonblocking count-only callbacks and sufficient processors/memory, expected scheduling time can be bounded by O(W/P+S). This is conditional modeling, not a Parlay-on-VM guarantee.

**Space.** For live roots I, let a_i be distinct scratch keys and e_i_left materialized left adjacency. Resident/auxiliary space is

`O(n+m+R+P*log(R+1)+sum_{i in I}(a_i+b_i^2+e_i_left+l_i+stack_i))`.

At most P callbacks are live because callbacks do not fork or yield. There are no copies of the global graph or a parent matrix per recursive child. Hash allocation constants and RSS still matter; avoiding P*n initialization does not guarantee lower practical memory.

**Correctness first.** After the helper repair: smoke19; 2,234 independent exhaustive/random cases, checking candidate P1/P4 and safe control against complete oracle sets, counts and duplicate-free emission; boundaries/CLI38; ASan+UBSan+leaks30 including K4100 and flag length/alignment/pattern regression; 240 repeated exact-set cases at P2/8/16/20; real-input count sanity20. The final 9,283 logical checks comprise **7,049 new executions and 2,234 explicitly reused/reverified immutable-reference traces**. No failed gate was counted as a pass; initial pre-repair runs and the UBSan failure are retained. TSan builds but exits66 before initialization with unexpected memory mapping: **unsupported environment, not a race-check pass**. See `../runs/p001-validation-summary.json` and branch `tests/p001_validation.md`.

**Measured protocol.** All development, compilation and execution were local to Ubuntu joey-virtual-machine at `/home/joey/k-plex-rethinki-ubuntu`; no self-SSH dispatch. One benchmark process, affinity0-19, <=20 workers, 48GiB child address-space cap, whole-batch160GiB and <=70% available memory. No data normalization/builds overlapped timings. Three seeded interleaved repetitions for six development/intermediate cases at P1/2/4/8/16/20: **216 successful measurements**. Additional five interleaved repetitions for jazz(4,12), Enron(3,20), Wiki(3,20): **180 successful measurements**. All396 counts match, every198 native worker observation matches; no timeout/OOM/crash/censored result. Independent agent audits recomputed every summary cell from raw logs and verified source/binary/input/driver/gate hashes. Runtime is a VMware guest with24 visible vCPUs, not24 verified exclusive physical cores.

**Initial medians (3 repetitions, seconds).** C/L is candidate time divided by same-P safe legacy time; >1 means slower. S20 uses candidate P1/candidate P20.

| Case | Safe legacy P1 s | Parlay P1 s | Safe legacy P20 s | Parlay P20 s | Parlay S20 | C20/L20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| jazz-k3-q10 | 0.3917 | 0.4081 | 0.0451 | 0.0397 | 10.29 | 0.880 |
| ca-GrQc-k3-q10 | 0.0231 | 0.0303 | 0.0204 | 0.0224 | 1.35 | 1.099 |
| jazz-k4-q12 | 4.6524 | 4.6110 | 0.3123 | 0.3317 | 13.90 | 1.062 |
| email-Enron-k3-q20 | 1.8226 | 2.2636 | 0.1885 | 0.2520 | 8.98 | 1.337 |
| wiki-vote-k3-q20 | 4.7578 | 5.3720 | 0.2932 | 0.3668 | 14.65 | 1.251 |
| com-dblp-k3-q20 | 3.0191 | 3.0801 | 0.3065 | 0.2920 | 10.55 | 0.953 |

**Native candidate scalability (initial median speedups, same native executable).**

| Case | P1 | P2 | P4 | P8 | P16 | P20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| jazz-k3-q10 | 1.00 | 1.36 | 3.43 | 5.84 | 7.83 | 10.29 |
| ca-GrQc-k3-q10 | 1.00 | 1.21 | 1.45 | 1.69 | 1.64 | 1.35 |
| jazz-k4-q12 | 1.00 | 1.33 | 3.79 | 7.27 | 12.24 | 13.90 |
| email-Enron-k3-q20 | 1.00 | 1.88 | 3.45 | 6.38 | 9.32 | 8.98 |
| wiki-vote-k3-q20 | 1.00 | 1.95 | 3.86 | 7.49 | 12.56 | 14.65 |
| com-dblp-k3-q20 | 1.00 | 1.84 | 3.55 | 5.97 | 9.94 | 10.55 |

**Confirmation (5 additional repetitions, seconds; median and full observed min/max).** These are separate from the initial3, not silently pooled. The complete six-worker grids remain in raw summaries.

| Case | P | Legacy median [min,max] s | Parlay median [min,max] s | C/L |
| --- | ---: | --- | --- | ---: |
| jazz-k4-q12 | 1 | 4.6613 [4.6551,4.6864] | 4.5960 [4.5827,4.6030] | 0.986 |
| jazz-k4-q12 | 2 | 2.3613 [2.3596,2.3794] | 3.4799 [3.4670,3.5146] | 1.474 |
| jazz-k4-q12 | 20 | 0.3130 [0.3096,0.3326] | 0.3625 [0.2915,0.3917] | 1.158 |
| email-Enron-k3-q20 | 1 | 1.8184 [1.8145,1.8290] | 2.2704 [2.2680,2.2843] | 1.249 |
| email-Enron-k3-q20 | 2 | 0.9362 [0.9255,0.9408] | 1.2063 [1.1971,1.2111] | 1.289 |
| email-Enron-k3-q20 | 20 | 0.1909 [0.1789,0.2143] | 0.2097 [0.1999,0.2507] | 1.099 |
| wiki-vote-k3-q20 | 1 | 4.7882 [4.7534,4.8112] | 5.3801 [5.3628,5.4453] | 1.124 |
| wiki-vote-k3-q20 | 2 | 2.4074 [2.3948,2.4806] | 2.7610 [2.7550,2.8264] | 1.147 |
| wiki-vote-k3-q20 | 20 | 0.2954 [0.2880,0.4481] | 0.3704 [0.3163,0.5319] | 1.254 |

The confirmed Enron P1 median is24.9% slower, Wiki P1 is12.4% slower, and jazz(4,12) P2 is47.4% slower. For these cells every candidate run exceeds every reference run; paired candidate/reference ratios are respectively[1.2477,1.2506], [1.1213,1.1318] and[1.4641,1.4882]. These are clear repeated regressions across serial and parallel execution, not an aggregate-mean judgment. High-worker points are noisier: Enron P20 changes from1.337 times legacy initially to1.099 on confirmation, and Wiki P20 paired ratios span[1.0444,1.8472]. Do not claim a stable34% Enron P20 slowdown or formal statistical significance from these repetitions. Some small cases improve at some P, and candidate self speedup can exceed10x, but that does not cancel regression against the same-worker control elsewhere.

**Memory and phase limits.** Initial P20 median peak RSS is15.56MiB candidate vs9.75MiB legacy on Enron,13.69vs9.00MiB on Wiki and31.92vs28.83MiB on DBLP. These are observed RSS, not full live-task/allocator accounting. Candidate Preprocessing time covers allocations, serial peeling/rebuild and ordering, excluding loading; Root search time includes scheduler startup, Result initialization, roots and sequential reduction, not just enumeration. Running time combines those phases; primary end-to-end wall also includes loading and process/runtime teardown. Legacy exposes fewer phase fields, so do not invent matched core/subgraph/search speedups. No search-node/copy/allocation counters were collected; measured differences belong to the whole port/state-layout/preprocessing change and cannot be attributed solely to hashing or scheduler.

**Scope and closure.** All five predeclared core inputs are now prepared, including new SNAP Epinions/Pokec/Skitter binaries with provenance, exact n/m and SHA. Core/held-out/large performance runs were **not run**, following the predeclared early-rejection rule after confirmed intermediate-case regressions. They are not passes, and no hard-graph generalization claim is made. Theory is conditional average-hash; experiment gate fails. Close this route as rejected, retain branch/code/logs, and do not add graph/machine dispatch knobs to mask regressions. Different task-state representations or decomposition ideas need their own explicitly recorded trial/branch.

**Evidence/reproduction.** See `evaluation/2026-10-25-p001/REPORT.zh-CN.md`, `../runs/p001-20261025/plan.json`, `confirmation-plan.json`, `perf-a/{metadata,measurements,summary}.json`, `perf-confirm/{metadata,measurements,summary}.json`, the correctness summary, all raw stdout/stderr/time files, and `../datasets/p001-core-20261025/manifest.json`. On Ubuntu the branch builds with `make -C ../worktrees/p001 parlay-roots parlay-roots-emit` from project root only when paths are adjusted appropriately: the unambiguous command is `make -C /home/joey/k-plex-rethinki-ubuntu/worktrees/p001 parlay-roots parlay-roots-emit`. The default performance-script invocation only prints a plan; `--run` additionally checks correctness admission and exact binaries.

### P002 - Bounded fork-join splitting of heavy subtrees

**Hypothesis.** Expose independent existing branches with `parlay::par_do` and join before reclaiming parent state, to reduce the longest-root bottleneck. Prove complete/disjoint coverage and valid exclusion/recovery for the chosen partition, allowing parallel-motivated local changes. Build on an explicitly recorded native-Parlay control; do not silently import unaccepted parts of another trial.

**Work.** The accounting target is `W = W_local + O(N_t) + C_copy`. Charge spawning and copying to work of the candidate serial logic and establish the same-or-better comparable asymptotic bound, for example `W_local + N_t + C_copy = O(B_ref)`. If the search is unchanged, a stronger charging result relative to actual `W_ref` may be available; prove it rather than assume it. Copying an entire block or pair matrix at every recursive node does not establish that property. Do not eagerly materialize the full search tree, replay completed subtrees, or add sorting just to schedule tasks.

**Span.** For the resulting strict task DAG, `S = O(S_pre + log(R+1) + max_i D_i)`. `D_i` includes sequential prefix work, state-copy costs and unsplit leaves; a logarithmic search span is not assumed. State how the chosen splitting rule changes the critical path and bound live task space separately.

**Simplicity/parameters.** Prefer a structural, chargeable spawn rule and native scheduling to wall-clock or graph-dependent thresholds. Any unavoidable single global cutoff must have one fixed portable default and a sensitivity argument. Task/parent lifetimes and exclusion ownership must be explicit.

**Experimental analysis.** Not run. Repeat exact-set tests under multiple worker counts and task interleavings. Use all core and held-out cases; predeclare `enwiki-2021(3,50)` if claiming large-graph load-balance benefits. Compare to the same-P accepted Parlay control and the frozen serial reference. Report small-case overhead, longest tasks, waiting/stealing, spawned tasks, copied bytes and peak live state, together with every regression.

**Closure.** Reject if copying worsens asymptotic work, the implementation needs many tuning parameters, or improvement is confined to selected heavy cases with persistent regressions elsewhere. Theory and experiments remain pending; no implementation commit or run evidence exists.

### P003 - Shared immutable blocks with compact task state

**Hypothesis.** Construct a block's read-only graph/matrix once, share it through an explicit lifetime, and give each task only the mutable search/recovery state it needs. Start with ordinary RAII and existing Parlay containers/allocator support; a custom pool is not the default requirement.

**Work.** Write `W = W_local + O(N_t) + C_state`, counting construction, mutable-state initialization, restoration and reclamation. Establish `W_local + N_t + C_state = O(B_ref)` for a stated comparable bound; when the serial search is unchanged, document any stronger per-instance charging result. Reusing an immutable matrix avoids some copying but alone does not prove this bound; full-capacity zeroing and eager per-worker/per-task arrays must be charged explicitly.

**Span and space.** For an unchanged partitioning, the span is the existing task-DAG critical path plus state-management costs on that path; memory sharing alone does not imply an asymptotic span improvement. Analyze the concrete `D_i` after this change. A useful accounting form is `M = O(G + sum_live_blocks b_i^2 + sum_live_tasks state_bytes(t))`; bound the number of simultaneously live tasks rather than claiming constant per-worker space without proof. Synchronization/allocator contention belongs in both analysis and measurements.

**Simplicity/parameters.** No per-graph matrix layout switch or machine-specific pool table. Prefer a single representation and universal defaults. Suspended/stealable tasks must not alias mutable `thread_local` state or recycled slots.

**Experimental analysis.** Not run. Validate growth, removed-suffix restoration and last-owner destruction with sanitizers and repeated exact-set checks. Measure candidate P=1 and P>1 on all core/held-out cases; predeclare a suitable large case for any large-memory claim. Report RSS, allocated/copied bytes, construction/reclamation time, live-task peak and slowdowns, not only the memory saving on one graph.

**Closure.** Reject if the representation complicates the algorithm, inflates work, depends on per-case switches, or causes repeatable serial/parallel regressions. Work/span validation and experiments are pending.

## Required record for every actual trial

Copy this checklist into the appropriate entry and replace pending fields with evidence:

- **Identity:** ID, English name, branch, mainline/base commit, implementation commit, dependency revision and exact changes.
- **Hypothesis/invariants:** one main idea, parallel obstacle, local algorithm changes, retained problem/output semantics and their correctness proof, task partition, ownership, lifetime and completion conditions.
- **Theory:** reference bound and its parameters/model, actual `W_ref` where relevant, candidate serial `W_local`, candidate total `W`, candidate `S`, auxiliary space, primitive costs, assumptions and a complete charging argument. State any unresolved term or counterexample.
- **Parameters:** problem inputs/worker budget separately from algorithm knobs; fixed defaults and why no per-machine/per-graph tuning is needed.
- **Experiment plan:** predeclared development/held-out cases, full comparison set, 1-worker and same-P controls, repetitions, pinning and explicit time/memory/batch budgets.
- **Experiment results:** all cases, statuses, output checks, medians/variation, serial overhead or algorithmic improvement, native-runtime parallel speedups, phase times, RSS and relevant search/work/task/copy counters; link raw logs and source/input hashes. Do not substitute paper times or historical OpenMP results for new Parlay measurements.
- **Generalization analysis:** per-case regressions, additional paired checks for noise, held-out outcomes and limits of the measured scope. A mean improvement never erases a clearly worse case.
- **Decision:** independent theory/experiment gate outcomes; `accepted` with both passing, or `rejected` with why this route is not viable. Keep missing evidence non-terminal. Record the decision date and retained branch/commit on mainline.

See [development policy](docs/parallel-development.md), [runtime contract](tests/runtime-policy.json), [suite manifest](tests/test-suites.json) and [fixed reference](tests/baselines/listplex-hardened-2026-10-25.json). The existing historical runners do not enforce these new contracts yet.

## Execution location after workspace migration

The primary project is `/home/joey/k-plex-rethinki-ubuntu`, and its canonical algorithm repository is `relaxclq/`. When developing directly on this Ubuntu host, development and testing are co-located: build and execute all tiers locally on that server, using its local tmux for long runs. Do not SSH/SCP the work back to the same host. The Windows copy uses SSH only when the selected operation targets the separate Ubuntu host. Names such as `remote_sanity` or legacy `where=remote` identify the test host, not a mandatory network hop. Resolve the profile using `../tools/project-context.py` and `../execution-environments.json`; the 20-worker and memory caps apply regardless of transport.


## Archived P001 pre-execution protocol (2026-10-25)

- **Identity:** native Parlay root parallelism, branch `technique/p001-parlay-roots`, mainline/base `f85c6e833e7f67330a684a0fe894620e874eb988`, pinned runtime `51017699dcc421f80479cdb238d3092233ad0d26`. Solver edits and execution are native to Ubuntu. Implementation, independent oracle validation and work/ownership review are assigned to separate collaborating agents; benchmark admission and this mainline ledger are coordinated by the primary agent.
- **Work review:** the original worker-local dense global-vertex arrays would introduce an uncharged P*n initialization term. A minimal port must remove that term or fail the theory gate. Expected amortized sparse lookup is an allowed candidate design, with its expected assumptions and practical overhead measured explicitly. No success is assumed.
- **Correctness admission:** 19-call smoke, 2,048 exhaustive five-vertex cases plus 186 seeded random cases against independent sets and frozen reference, P=1/4 candidate, duplicate checks; storage/CLI/text/bin/large-capacity boundaries; ASan/UBSan/leaks and repeated 2/8/16/20-worker small cases. Document TSan compatibility separately.
- **Stage A:** jazz(3,10), ca-GrQc(3,10), jazz(4,12), email-Enron(3,20), wiki-vote(3,20), com-dblp(3,20). Compare the same native candidate executable at P=1/2/4/8/16/20 against an independently built immutable legacy reference/control at the same P, with three interleaved repetitions, seed 20261025. The one-worker reference is the serial baseline; candidate speedup uses candidate P=1 only.
- **Core and holdout:** preserve all five manifest core cases and the existing holdout split: com-dblp(4,12), soc-pokec(4,20), as-skitter(3,60). Prepare missing SNAP inputs with provenance, normalization and hashes before any core measurement. A clearly repeatable Stage A regression can reject the route before an expensive full-core sweep; then unrun cases remain explicitly unrun and do not establish generalization. Otherwise proceed through pilots and the full required core grid within the declared budget. Never tune on holdout failures.
- **Budget:** one timed benchmark process, all measurements on the same CPU affinity 0-19, <=20 total workers, 48 GiB per timed child, whole batch <=160 GiB and <=70% currently available memory. Stage A: 60 seconds per invocation, 1,800 seconds batch; core: 1,800 seconds per invocation, 14,400 seconds pilot batch and 43,200 seconds full batch. No data preparation or compilation overlaps timings. At least three repetitions and additional paired checks for apparent regressions; keep censored/incomplete results.
- **Evidence:** Ubuntu `runs/p001-20261025/plan.json`; implementation commit, proof, exact result files and final independent gate outcomes pending. This plan is not an experimental result.

### Archived correctness-driven control amendment (before timing)

The 19-call smoke, 6,702 exact-set oracle/reference/candidate checks and 38 boundary/CLI checks initially passed, but the stronger ASan+UBSan gate found inherited signed overflow in `sequence::sumFlagsSerial` on K4100. Its packed `bool*`-to-`int*` reinterpretation also violates strict aliasing. Performance admission was denied. An isolated candidate engineering repair, commit `0c6a5eacedf10b7ed1185fe28e93e888199ff433`, replaces that branch with a direct Boolean sum: still Theta(n) work/span and O(1) auxiliary space, without changing enumeration logic.

The original frozen archive and reference binaries remain immutable. Before any timing, the comparison plan was amended to independently build a **safe-helper reference/control** from the original frozen source plus only this helper repair. This controls for the engineering fix: both measured candidate and measured legacy OpenMP control use defined Boolean counting. Preserve the raw historical reference for provenance and oracle comparisons; do not relabel it as sanitizer-clean. Candidate binaries and correctness gates are being rebuilt/rerun, with targeted flag patterns and boundary lengths. The new control archive/hash and final results remain pending.
