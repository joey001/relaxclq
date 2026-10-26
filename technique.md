# ListPlex Parallel Techniques

## Scope and current state

This is the English experiment ledger on the repository mainline, **`armAndX86`** (verified as the remote default branch on 2026-10-25). Do not rename it to `main` or treat a technique branch as the ledger's authoritative copy.

Future candidate algorithms use **the native ParlayLib scheduler** from [joey001/parlaylib](https://github.com/joey001/parlaylib.git), initially pinned to `51017699dcc421f80479cdb238d3092233ad0d26`. OpenMP, Cilk/OpenCilk, TBB, and independent thread-pool backends are outside the candidate implementation contract. The archived ListPlex reference and unchanged author comparators retain their historical runtime. This policy change does not port the solver yet.

[GBBS](https://github.com/ParAlg/gbbs) is a useful source of Parlay graph-algorithm patterns; it is neither a mandatory dependency nor the boundary of permitted ideas. Its `gbbs/bridge.h` exposes Parlay loops and fork-join operations. GBBS currently pins a different Parlay revision: inspect compatibility instead of silently replacing the selected fork. Preserve source attribution and applicable licenses for adapted code.

**P001 has completed correctness validation and 396 timed runs; its current sparse-state route is rejected for repeatable regressions. P002/P003 remain unimplemented.** Existing hardening and OpenMP measurements are historical evidence, not experiments on the proposals below. P001 has a retained implemented branch; P002/P003 are reserved and an empty branch is not an attempted implementation. No graphs are downloaded and no experiments are launched by maintaining this ledger.

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

**Identity.** Branch `technique/p001-parlay-roots`; base `f85c6e833e7f67330a684a0fe894620e874eb988`; port `cc107ae2e08dde256260046be76588e96e1ece06`; isolated helper repair `0c6a5eacedf10b7ed1185fe28e93e888199ff433`; tested source/harness HEAD `2969a9ee8c2a597c735a07147012b6df9587a1d2`. Native Parlay fork revision `51017699dcc421f80479cdb238d3092233ad0d26`, codeload archive SHA `4f2fc759c1ca474db2f45585ee2300997a128d48fba79bf47aaa579aa8858ee8`; source manifest in `../deps/parlaylib-source-manifest.json`. GCC 11.4, candidate C++17/pthread/O3/DNDEBUG/march=native/msse4.2/TARGET, no OpenMP backend; observed native workers equal every request. No new algorithm tuning knobs; grain one is structural for irregular roots.

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

**Correctness first.** After the helper repair: smoke 19; 2,234 independent exhaustive/random cases, checking candidate P1/P4 and safe control against complete oracle sets, counts and duplicate-free emission; boundaries/CLI 38; ASan+UBSan+leaks 30 including K4100 and flag length/alignment/pattern regression; 240 repeated exact-set cases at P2/8/16/20; real-input count sanity 20. The final 9,283 logical checks comprise **7,049 new executions and 2,234 explicitly reused/reverified immutable-reference traces**. No failed gate was counted as a pass; initial pre-repair runs and the UBSan failure are retained. TSan builds but exits66 before initialization with unexpected memory mapping: **unsupported environment, not a race-check pass**. See `../runs/p001-validation-summary.json` and branch `tests/p001_validation.md`.

**Measured protocol.** All development, compilation and execution were local to Ubuntu joey-virtual-machine at `/home/joey/k-plex-rethinki-ubuntu`; no self-SSH dispatch. One benchmark process, affinity 0-19, <=20 workers, 48 GiB child address-space cap, whole-batch 160 GiB and <=70% available memory. No data normalization/builds overlapped timings. Three seeded interleaved repetitions for six development/intermediate cases at P1/2/4/8/16/20: **216 successful measurements**. Additional five interleaved repetitions for jazz(4,12), Enron(3,20), Wiki(3,20): **180 successful measurements**. All396 counts match, every 198 native worker observation matches; no timeout/OOM/crash/censored result. Independent agent audits recomputed every summary cell from raw logs and verified source/binary/input/driver/gate hashes. Runtime is a VMware guest with 24 visible vCPUs, not 24 verified exclusive physical cores.

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

**Confirmation (5 additional repetitions, seconds; median and full observed min/max).** These are separate from the initial 3, not silently pooled. The complete six-worker grids remain in raw summaries.

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

The confirmed Enron P1 median is 24.9% slower, Wiki P1 is 12.4% slower, and jazz(4,12) P2 is 47.4% slower. For these cells every candidate run exceeds every reference run; paired candidate/reference ratios are respectively[1.2477,1.2506], [1.1213,1.1318] and[1.4641,1.4882]. These are clear repeated regressions across serial and parallel execution, not an aggregate-mean judgment. High-worker points are noisier: Enron P20 changes from 1.337 times legacy initially to 1.099 on confirmation, and Wiki P20 paired ratios span[1.0444,1.8472]. Do not claim a stable 34% Enron P20 slowdown or formal statistical significance from these repetitions. Some small cases improve at some P, and candidate self speedup can exceed 10x, but that does not cancel regression against the same-worker control elsewhere.

**Memory and phase limits.** Initial P20 median peak RSS is 15.56 MiB candidate vs 9.75 MiB legacy on Enron,13.69 vs 9.00 MiB on Wiki and31.92 vs 28.83 MiB on DBLP. These are observed RSS, not full live-task/allocator accounting. Candidate Preprocessing time covers allocations, serial peeling/rebuild and ordering, excluding loading; Root search time includes scheduler startup, Result initialization, roots and sequential reduction, not just enumeration. Running time combines those phases; primary end-to-end wall also includes loading and process/runtime teardown. Legacy exposes fewer phase fields, so do not invent matched core/subgraph/search speedups. No search-node/copy/allocation counters were collected; measured differences belong to the whole port/state-layout/preprocessing change and cannot be attributed solely to hashing or scheduler.

**Scope and closure.** All five predeclared core inputs are now prepared, including new SNAP Epinions/Pokec/Skitter binaries with provenance, exact n/m and SHA. Core/held-out/large performance runs were **not run**, following the predeclared early-rejection rule after confirmed intermediate-case regressions. They are not passes, and no hard-graph generalization claim is made. Theory is conditional average-hash; experiment gate fails. Close this route as rejected, retain branch/code/logs, and do not add graph/machine dispatch knobs to mask regressions. Different task-state representations or decomposition ideas need their own explicitly recorded trial/branch.

**Evidence/reproduction.** See `evaluation/2026-10-25-p001/REPORT.zh-CN.md`, `../runs/p001-20261025/plan.json`, `confirmation-plan.json`, `perf-a/{metadata,measurements,summary}.json`, `perf-confirm/{metadata,measurements,summary}.json`, the correctness summary, all raw stdout/stderr/time files, and `../datasets/p001-core-20261025/manifest.json`. On Ubuntu, build the retained branch with `make -C /home/joey/k-plex-rethinki-ubuntu/worktrees/p001 parlay-roots parlay-roots-emit`. The default performance-script invocation only prints a plan; `--run` additionally checks correctness admission and exact binaries.

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

### P010 - Earned-work admission with bounded live snapshots (2026-10-26)

**Preregistration/status:** design reviewed by two explicitly delegated
`gpt-6-astra`/max agents; implementation assigned to `gpt-6.1-sol`.
Pending implementation and evidence, not accepted. Dedicated branch:
`technique/p010-earned-work-forks`, new checkout `../worktrees/p010`.
Exact prerequisite base **1ace08d901d0f0bf93514af88cf5a473284e18e2**
(P005; algorithm commit d066514), itself still pending, not an accepted mainline
optimization. Its native dense root streams and compact strict-joined snapshots
are required scaffolding. Mainline armAndX86 and both immutable references remain
unchanged. Attribute P010's incremental effects against this P005 baseline as well
as reporting end-to-end comparisons with the safe reference and pristine authors.

**Paper grounding.** Read Dhulipala, Blelloch and Shun,
[Theoretically Efficient Parallel Graph Algorithms Can Be Fast and Scalable](https://arxiv.org/abs/1805.05208),
arXiv v4 (21 August 2019), sections 3 and 6. Sections 6.1/6.2 motivate accounting
for contention and memory traffic, and using useful work to amortize parallel
organization. Algorithm 18 logically blocks traversal work and packs live output;
it is not a k-plex search algorithm. The earned-work rule below is our proposed
adaptation of work-efficiency discipline, not an algorithm or theorem attributed
to the paper. Do not transfer its traversal bounds or measured speedups to enumeration.

**Single hypothesis.** P005/P006 total per-root creation budgets may spend copies
on cheap early branches and cannot renew after joins. Charge each new admission
attempt to actual completed search work, and cap simultaneously live copies
instead of total lifetime creations. This may avoid early cheap copies and allow
later work to expose parallelism. It need not outperform either existing strategy:
ancestor-held permits can still serialize a deep heavy continuation, and expensive
past work does not predict useful work in the next sibling. No new pivot, pruning,
branch partition, matrix construction, root schedule, runtime or graph-specific
threshold is part of this experiment.

**Precise admission rule.** For a root with b current block vertices and l left
vertices, fix the structural threshold in 32-bit-word equivalents:
`F = 1 + 12*b + 2*l + 4*(floor(b/32)+1) + (floor(l/32)+1)`.
This is Theta(1+b+l), derived from compact backing capacities plus constant
objects; it is not an exact machine-instruction count. Each logical state starts
with zero private credit. Only after a complete existing addG/subG/addP/subP
neighbor-update loop, add the actually traversed degree, saturating at F without
overflow. Never credit copying, initialization, comparisons skipped by early exit,
child work, estimated future work or inherited credit. Undo loops count only when
they actually execute. Backtracking does not restore a historical credit value.
At an existing canonical fork site, insufficient credit falls back to the existing
serial first/rest path. Otherwise set credit to zero BEFORE attempting admission,
even when admission fails. A successful child starts at zero; the parent remains
zero and thereafter earns only its own executed work. Never merge child credit.
For P1 disable admission/accounting via a null pool; the conditional checks still
have measurable cost and are not claimed to be free.

One process-global pool, outside all root callbacks, permits at most C=P-1 admitted
live clones. One relaxed fetch-add reserves a tentative slot; old>=C immediately
rolls it back with fetch-sub and uses the serial fallback. No retry, wait, spin or
custom scheduler is added. The counter includes in-flight failed reservations and
can temporarily exceed C; its type must safely represent C+P. A noncopyable RAII
guard is created before any clone storage and destroyed after all clone members,
including the strict join and output merge. The pool lives through all root joins.
Matrices/graph remain shared read-only; complete compact member suffixes, position
arrays, bitmaps, counts, left checker scratch and exclusion aliasing remain private
as in P005. Cloning historical capacities, TLS borrowing and early permit return
are forbidden; the existing root scratch reuse remains unchanged.
The strict-join lifetime argument concerns normal completion. The pinned native
Parlay pardo is not an exception-safe join: exceptions during dispatched search
are fatal, not a caught-and-resumed or completed-result path. RAII can release a
reservation on pre-dispatch allocation failure, but does not make post-dispatch
unwinding safe. Do not add catch-and-continue behavior or modify the scheduler.

**Correctness and accounting.** Admission changes only whether an existing
complete/disjoint first/rest partition runs serially or in independent joined
states. Preserve restoration and global maximality checks, exact 64-bit counting,
complete output and duplicate freedom. Let U_i be actual executed iterations of
the four metered loops in root i, A_i its funded attempts, and K_i successful copies.
Disjoint private-credit intervals imply `K_i <= A_i` and `A_i*F_i <= U_i`.
Thus all copy/construction/destruction work is
`O(sum_i K_i*(1+b_i+l_i)) = O(sum_i U_i)`, and funded synchronization is
`O(sum_i A_i)`. Meter calls, including degree-zero calls, and unfunded branch tests
add `O(J + N_branch)` for existing update-call and fork-site counts. They must not
be omitted. This is an upper bound under a common search model; U_i is actual
candidate work, not the measured P1 work or proof of per-input reference dominance.
P005's existing algorithmic/common-model bound remains a prerequisite to review.
Unit-cost atomic accounting is not a promise of constant cache-coherence latency.
More explicitly, with B the inherited preprocessing work, N the peeled vertex
count, Q the inherited stream count, and L_i actual local construction/search
work excluding snapshot/admission overhead, total work is
`O(B+P+N+Q*N+sum L_i+J+N_branch+sum A_i*F_i)`
`= O(B+P+N+Q*N+sum L_i)`.
The metered iterations and original call/site costs are contained in L_i;
`Q*N <= N+sum deg(v)^2` is inherited from P005. This common-model accounting
does not identify L_i with a measured serial execution.

**Span/space.** Use the actual weighted DAG: granted sites pay reservation and
copy, then max(child,rest), join/merge/destruction; denied sites execute the serial
sum. Add the existing preprocessing, callback dispatch/root assignment and root
construction spans. Model global atomic serialization in the actual augmented
weighted event DAG: for consecutive pool operations a_j, finish(a_j) is its cost
plus max(finish(local predecessor), finish(a_(j-1))). Other vertices retain their
ordinary local/fork/join dependencies. Cross-root edges can concatenate long
computation segments, so adding only O(sum_i A_i) to a stream-span bound is NOT
justified. Use the longest augmented path, conservatively bounded by total work;
no logarithmic contention, balanced search or universal span improvement is
claimed. At most Q stream-owned base roots and
C admitted clone states are live, including suspended ancestors. Sum historical
stream scratch/matrix capacities and the base-root recovery stacks, then add at
most `O(C * max_i(1+b_i+l_i+b_i*k))` for clone state/recovery along with native
runtime metadata. Do not replace all live state by P arrays or ignore retained
capacity. The counter's transient excess consists of reservations without clones,
not additional snapshots. Native nonelastic idle CPU remains visible in logs.

**Predeclared validation and decisions.** Phase 1 starts with the canonical
19-invocation smoke, then the fixed 2234-case independent oracle/reference plan,
relevant boundaries, compact suffix/exclusion-alias regressions, ASan/UBSan and
representative P2/8/16/20 exact-set repetitions. Add targeted admission tests for
zero/overflow-safe credit, failed-attempt consumption, after-join renewal, nested
ancestor-held fallback, global cap and storage-before-permit destruction. Abstract
event fixtures demonstrate admission properties only, not realizable graph
speedups. Real graph output still needs independent oracle checks. Diagnostics
must show real forks and conservation; release timing must not include extra
shared diagnostic counters. P1/P>1 use the same fixed-native-runtime executable.
Phase 2 uses jazz(3,10), ca-GrQc(3,10), jazz(4,12). Phase 3 uses the fixed
Enron/Wiki/DBLP development cases and all difficult core/held-out cases; no held-out
tuning. Each timing checks input identity/count, P1/2/4/8/16/20 and at least three
interleaved repetitions. No extra mandatory standalone sanity phase. Compare
absolute wall, P1 cost, same-binary scaling, CPU and RSS; diagnose copy/admission
traffic separately. Use temporally adjacent controls for disputed changes.
Repeatable regression, excessive copying/contention, invalid bounds or needless
complexity can reject early; missing tests are pending, never success or rejection.
The development prediction is approximately unchanged P1 time versus P005
(conditional-accounting overhead must be measured), removal of P005's confirmed
approximately 4.1% Enron P4 overhead if early copies caused it, and a reproducible
P8/16/20 wall improvement on at least Enron or DBLP versus P005. These are
falsifiable hypotheses, not observations. There is no matrix-speedup prediction
for Wiki. Insufficient metered work, global-gate serialization or ancestor-held
permits may eliminate all benefit. Fewer copies alone does not meet the existing
objective of materially exceeding both authors' same-binary scaling.
Existing canonical time/memory limits apply; freeze a concrete finite batch plan
and current resource grant before launch rather than inheriting an unlimited run.

**Execution hold at preregistration.** Native Ubuntu dev=test was verified again.
The old eight-solver study still has 739/864 saved records without terminal markers;
its existing performance lock is held (inode 18779599). Host PID/tmux visibility is
unavailable. Host-terminal read-only status has been requested. No new compiler,
solver, benchmark, count resume, lock replacement or process signal is authorized
by this record while that resource conflict remains unresolved. Source work and
static proof review can proceed in the separate P010 branch. After completed tests
or justified early rejection, apply the current lean cleanup and authorized normal
technique-branch commit/push rules; no automatic mainline merge or publication.


#### P010 source-only implementation checkpoint

Sol 6.1 has prepared the initial implementation in the registered branch/worktree.
Two Astra-max source reviews found no outstanding blocking issue in credit
accounting, the process-global permit pool, normal-completion ownership or the
unchanged branch partitions. These are static findings, not correctness admission.
The original three first/rest lambdas, restoration, exclusion alias and global
maximality checks are unchanged from the explicit P005 prerequisite. Release adds
only the admission policy and private metering, without diagnostic shared counters.
The separate diagnostic build checks funded work against actual metered work,
reservation return, after-join grants and overflow-invalid statistics. The existing
root callback comment now explicitly permits suspension/helping while ownership
remains callback-local. Future builds require an explicit version output directory;
the reproduction instructions reject existing paths and hold the existing lock
without creating/replacing it.

**Executed validation:** source/diff review and `git diff --check` only. Compilation,
abstract admission unit, independent graph oracle, all sanitizers, real concurrent
reservation/renewal coverage and both performance phases are **UNRUN**. The written
admission unit is a sequential abstract model, not evidence of real concurrent
interleavings or graph speedup. Actual branch-site/exclusion-alias and after-join
renewal coverage remain to be demonstrated on graphs. No candidate binaries or
run data were produced, so no test intermediates required cleanup. No commit,
push, mainline merge, reference edit or paused-job action occurred. The mainline
algorithm/build sources remain unchanged. Current status is **source prepared,
pending correctness; execution blocked by the unresolved existing test window**.
The host-state request remains unanswered at this checkpoint; no inference of
termination follows from unavailable PID/tmux observation.

Draft file identities (uncommitted source, not binary/build admission):

| File in P010 checkout | SHA-256 |
| --- | --- |
| `makefile` | `7083745300570cea058b7fc63b20533799056000bac1a90fb3e64ca1152727e7` |
| `maximal_kplex/listPlex.cpp` | `0f25fd3a545de9680a7fd9391dedffef4bcf3e77c38a2b607a5bb87020753f21` |
| `maximal_kplex/listPlex.h` | `56600daf5eb5031db3ae5cced4610c4afe6463ecbf6c419f6aea037387bd0a9f` |
| `maximal_kplex/p010_fork_admission.h` | `885c2db8f6b3725509966032c481124396e599a1660b8eac837f6f02e474bf4b` |
| `tests/p010_admission.cpp` | `bcc8176a9e4b556df325350076fd449fc3823977a973a61ff4d7adbb73a2df76` |
| `tests/P010-REPRODUCE.md` | `8afca310cd6ad9e00ecb9ee44f1bc4672c41a4b09f4370d4e5faf092d2f7ead0` |


#### User-authorized retirement of old run state (2026-10-26)

The user cannot supply host lslocks/tmux/ps output and explicitly authorized
clearing the previous run state. The earlier request for that output is withdrawn;
do not request it again. The eight-solver development study and old safe-reference
count job have been removed from the active `jobs` registry and retained as
`retired_jobs` with `abandoned_by_user`, no automatic resume and termination
explicitly unconfirmed. The comparison's last 739/864 saved records and established
Wiki/Epinions/DBLP reference counts remain historical evidence. The incomplete
study is not a pass, an algorithm rejection or an obligation to finish before
selecting another experiment. Old handoff/entry documentation now marks the resume
procedure superseded by retirement. No result files or source/reference identities
were rewritten, and no active output directories were moved under a possible writer.

Administrative cleanup is complete; physical process cleanup is NOT complete.
A fresh nonblocking probe still finds the original lock inode **18779599** held.
Only sandbox-local PIDs are visible. The attempted host-scope read-only identity
check was rejected by the execution permission policy because sandbox escalation
is disabled; no host command ran and no process signal was sent. Existing user
authorization covers stopping these old tasks once their identities can be
verified through an execution environment with host-process access. Do not treat
missing sandbox PIDs as exited or signal a reused historical PID. Do not bypass
that permission denial through self-SSH, namespace entry or a substitute lock.
The cooperative pause flag stays in place to avoid unintentionally resuming the
old count while termination is unverified. Neither old task is a resume target.

The status tool now separately reports active jobs, retired jobs with unresolved
process cleanup, and a nonblocking read-only-open probe of the existing lock.
Lightweight checks confirmed: zero active/two retired jobs, no fabricated exit,
held/free/missing-lock reporting, preservation of the lock inode and no creation
of a missing lock. Documentation and JSON were updated consistently. No compiler,
solver or benchmark launched. P010 still has no executable correctness/performance
evidence; its remaining execution blocker is host process control and the held OS
lock, not missing permission from the user or a request for user-provided output.


#### Old process cleanup completed on the host (2026-10-26)

With host access enabled and explicit user authorization to clear blocking
processes/locks, current command lines and /proc start ticks were checked before
opening pidfds. The actual lock owner was stopped comparison controller PID145695
(start 193453417), not an orphaned lock file. Old Pokec time/solver PIDs66392/66393
were also stopped with their recorded start 193323068. Controllers were quiesced
before termination to prevent launching further work. TERM and CONT terminated
all eight verified old task processes: 145693,145695,4149628,4149630,4149631,
4150658,66392,66393. Pidfd exit readiness and a subsequent /proc inspection confirmed
termination; no KILL escalation was needed. No unrelated Codex, desktop or service
process was signalled. Both old tmux sessions ended naturally. Their shell wrappers
wrote exit143; these are cancellation outcomes, not completed experiments.

A nonblocking exclusive lock acquisition succeeded on the ORIGINAL inode
18779599, which was then released. The lock file was neither removed nor replaced.
The stale cooperative pause flag was removed only after all related processes
were verified terminated; its prior hash is recorded in the handoff registry.
Partial comparison records, independent verified reference counts, archives,
source and P010 draft remain intact. The active registry remains empty and both
retired jobs now explicitly have termination_confirmed=true. Current config,
entry/handoff documents and status output reflect actual resource clearance rather
than the superseded permission blocker. Future jobs must still acquire the same
lock and probe current resource limits. P010 is ready to begin correctness testing;
this process cleanup itself does not compile or validate any algorithm.


#### P010 R1 execution grant and timing preregistration (2026-10-26)

A new explicit gpt-6.1-sol/high implementation session, with verified host process
and tmux access, owns the testing window; the old restricted Sol session launched
no build or solver. Fresh paths are `../build/p010-r1-20261026-1323` and
`../runs/p010-r1-20261026-1323`, tmux `p010_r1_correctness`. Stage 1 has a finite
4-hour ceiling, one compiler/solver at a time, smoke P4 and at most 20 workers,
and batch memory <=160 GiB and <=70% current available memory. Host observation
showed 24 allowed CPUs, no CPU quota, about 241 GiB available and a free original
coordination lock. Every execution component retains the same lock. The initial
sequence is serial release/emitter/diagnostic build and admission/compact units,
then canonical 19-call smoke, 2234 oracle cases, boundaries and 240 high-worker
exact-set repetitions, followed by scoped sanitizer/real-fork coverage. Immutable
reference evidence may be reused only after identity checks. Before the release
freeze, diagnostic-only counters distinguish grants at the three canonical fork
sites and the exclusion alias; no release algorithm parameter changes were made.
Builds/checks in progress are not a correctness pass or performance grant.

Before any small-graph timings, extend the initial two-way P010/P005 screen with
the immutable safe ListPlex control: three solvers x three fixed small cases x
P1/2/4/8/16/20 x three repetitions = **162 timed invocations**, each checking its
answer. Safe P1 supplies the serial reference; safe P>1 is explicitly the historical
OpenMP same-worker comparator, not a native-Parlay baseline. P005 is an unaccepted
prerequisite comparator, not a substitute for the safe control. Shuffle blocks of
(case,worker,repetition) with a fixed seed, and balance/rotate solver order INSIDE
each adjacent block, rather than scattering each comparison across the whole
batch. This addresses the previously observed temporal variation without dropping
any measurement. Preserve the existing frozen drivers/results. Proposed case
limit120s, batch limit14400s, one process and 48GiB per-process memory subject to
the live 70% bound. A concrete hash-pinned plan and fresh exclusive grant are still
required after stage1 passes. Adjacent confirmation of a disputed regression can
reject early; no new percentage tolerance or held-out tuning is introduced.


#### P010 R1 correctness admission

The complete planned first phase passed and an independent Astra-max audit
re-read the recorded source/binary/input identities and raw evidence. There are
**7090 logical checks = 4856 fresh invocations + 2234 identity-verified reference
reuses**, plus the admission and compact snapshot units under ASan/UBSan/leak
options. Components: smoke19, oracle6702 (2234 graphs x reference/P1/P4),
boundaries38, P2/8/16/20 exact-set stress240, diagnostic44, sanitizer47. No count,
set, duplicate or worker mismatch was found. Reported peak RSS reached293952KiB.
The initial controller exited1 because the standalone compact-unit compile command
omitted the x86 TARGET/SSE flags. This produced no test binary and did not change
algorithm sources; the already-built release/emitter/diagnostic identities were
retained. A fresh compactfix output passed, followed by separate successful
resume/extra controllers (both exit0). Failed compilation evidence is retained.

Release SHA256: **f592cd7305b15bbed0bdbba739674bc2384acaecde904789872d706bf35675d2**.
Sanitizer+diagnostic+emitter SHA256:
**bca0a2f89bd749839d93d27006579e1994b121e0d4af01ad7cffc5dcc1d595ce**.
Gate `../runs/p010-r1-20261026-1323/correctness-gate.json`, SHA256
**c325701b14f38e350d44cceceeeb46143231097802af3db966a6ebde65b78723**,
pins the source snapshot, flags and evidence. In sanitizer exact-set runs the
three canonical sites granted23/633/55 snapshots, alias grants495 and after-join
renewal grants562;919 funded attempts=711 grants+208 denials. All calls ended with
zero reservations and funded work no greater than actual metered work.
The single fixed `random-12-0.5-0-k3-q5` P4 input (SHA d0b1359e...40e684)
independently yields102 exact sets and covers sites7/2/6, alias7 and renewal10.
This establishes the requested coverage without further fixture expansion.
TSan was not rerun for R1 because of the recorded host incompatibility; it is NOT
a pass or proof of race freedom. Native high-worker exact-set tests did run.

Astra found the evidence sufficient for the preregistered small-graph performance
phase. This is correctness eligibility only: no performance success, universal
speedup or technique acceptance is implied. Original hypothesis, common-model
work bound and conservative augmented-DAG span limitations remain in force.


#### P010 R1 small-graph exclusive timing grant

Final plan `../runs/p010-r1-small-plan-20261026-1333/plan.final.json`, SHA256
**b99a42dbf921b2175f9de5a17987d836ac141050f2614cff767d8776b008a01e**,
grants the preregistered162-call adjacent-block screen after independent Astra
correctness admission. The new frozen measurement driver SHA256 is
**96b80fd33a5c96e7ff9fb0628f4e5c822717aeb276948ae74e29857b2aa30f54**;
old frozen drivers remain unchanged. Each case/worker cell rotates solver position
across its three repetitions. Per-process memory is tightened to8GiB for these
small inputs (still subject to160GiB/70%available), timeout120s, batch14400s.
Fresh output `../runs/p010-r1-small-20261026-1333`, tmux `p010_r1_small`;
other agents are read-only with no competing compilation or solver. Both native
candidates retain their admitted same-binary P1/P>1 identities, and safe P>1 is
labelled the historical OpenMP control. Completion, counts and stable paired
comparisons are required before any speed claim. No difficult-core timing is
implied by this small-graph grant.


#### P010 R1 decision: rejected after small-graph performance

All162 adjacent-block timing invocations completed with verified answers, source/
binary/input identities, workers and GNU CPU/RSS records; controller exit0,
wall145.449742s. Astra independently audited raw records, all54 blocks, balanced
positions and the paired analysis. `measurements.json` SHA256 is
**5df60922bef65399c4d6b1d6e3dd433b3211ec488fab7d742049c6e77540c65d**.
No samples were discarded or retrospectively normalized. GrQc's10-30ms observations
are startup/noise dominated and are not used to reject the technique.

| Case/workers | P010/P005 adjacent wall ratios (three repetitions) | Observation |
| --- | --- | --- |
| jazz(4,12),P20 | 1.3648 / 1.3206 / 1.4122 | Repeated parallel regression across all positions |
| jazz(4,12),P1 | 1.0313 / 1.0280 / 1.0385 | Repeated single-worker regression on7.4-7.8s calls |
| jazz(3,10),P16 | 1.1582 / 1.3150 / 1.3961 | Additional repeated parallel regression |

For jazz(4,12), P010/P005 P20 adjacent CPU ratios are1.4808/1.2894/1.3215;
P010/SafeOpenMP P20 wall ratios are1.3289/1.2500/1.0772. P1 wall relative to safe
is1.0548/1.0484/1.0430. Thus the failure is not inferred solely from speedup
normalization or comparison with an already-slow prerequisite. Median P1/P20 wall
seconds are P0107.709296/0.420230, P0057.455806/0.307917 and safe7.366939/0.363594;
same-binary self-speedups are18.35x/24.21x/20.26x respectively. P20 median total CPU
seconds are7.73/5.22/6.16 and RSS5740/8064/4800KiB. Smaller candidate RSS does not
compensate for the observed time regression, nor is RSS measured copied bytes.
Ratios of medians and medians of adjacent paired ratios are distinct statistics;
use the preserved per-pair data for the repeated-regression finding.

**Decision: rejected.** The original repeatable-regression condition is met;
no post-hoc tolerance is introduced. Correctness and the conservative common-model
work/space argument survived, but paid-copy admission with a reusable global live
cap did not deliver the required practical non-regression. The measurements do
not isolate copying, metering, atomics or code generation as the causal bottleneck.
Do not claim any one cause without a separate experiment. No graph-specific tuning
or threshold revision is made inside P010. Stage3 large/core/held-out performance
is **UNRUN by justified early rejection**, not passed, failed or missing-case
acceptance. No further confirmation is needed to close this clearly negative
trial. Preserve the tested source and compact audited evidence, then perform the
already-authorized scoped cleanup and normal technique-branch publication; the
mainline algorithm remains unchanged and no candidate is merged.


#### P010 R1 lean cleanup and publication preparation

All P010 controllers exited, no P010 tmux/solver/compiler remains, and the original
lock inode18779599 was independently acquirable. The rejection and identities are
recorded above. A dependency check found no pending job using the following fresh
P010-only intermediates. Before removal, `../runs/p010-r1-20261026-1323/cleanup-manifest.json`
recorded exact paths/hashes/sizes and verified duplicate traces byte-for-byte.
Remove two deterministic K4100 fixture binaries (134480024 bytes total), three
rebuildable admission/compact/sanitizer test binaries, and6702 copied reference
stdout/stderr/resource files (2234 verified reuses). The original protected P005
reference traces remain at each record's `reused_from`; no control is rebuilt or
deleted. Retain all fresh invocation raw evidence, per-run/checks rows, independent
small-graph fixtures/expected sets, gates, source snapshot/patch/identities, sanitizer
build command/log, original compile failure and both successful exit markers.
Retain the three small tested release/emitter/diagnostic binaries and all162
performance records, plans and frozen driver. No shared data, other technique,
pending candidate or uncertain process file is removed. The manifest records the
actual removal completion after these checks; generated complete fixtures can be
recreated from the retained canonical generator and pinned SHA.

Cleanup completed: **6707 artifact files / 144507461 bytes** removed,
plus only P010-created Python bytecode caches. Manifest SHA256:
**5dc7d15b8bee6799c521b7ae2177f96cee218a21d3566238c4a25bacb502a45e**.
The original immutable reference/archive/control paths and every fresh candidate
raw stdout/stderr/resource record remain available. The P010 worktree and source
are retained for the authorized same-name technique-branch publication.
