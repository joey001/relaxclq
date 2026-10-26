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


### P011 - Minority-row updates with private count offsets (2026-10-26)

**Preregistration/status:** proposed by two explicitly delegated
`gpt-6-astra`/max analyses and assigned to `gpt-6.1-sol` for implementation.
Not implemented, measured or accepted at registration. Reserve branch
`technique/p011-minority-row-updates` and checkout `../worktrees/p011`.
The continuing objective is broad parallel improvement against preserved
ListPlex and the unchanged author implementations; closing P010 did not achieve
that objective. No difficult-core or held-out result is inferred here.

**Exact base and prerequisite.** Use P005 commit
**1ace08d901d0f0bf93514af88cf5a473284e18e2**, algorithm d066514, as an explicitly
registered **pending, unaccepted prerequisite**, rather than silently treating it
as mainline. Correct mainline remains
`6d035d10469fdc18a6bfcae767b1115c975432ee`. P005 supplies the already-reviewed
native dense streams, current-universe compact snapshots and strict within-root
joins needed to retain useful parallelism while changing search-state arithmetic.
Porting the original mainline OpenMP/TLS/task code anew would add a separate
runtime/ownership experiment; R2 alone would discard the existing within-root
parallelism. P005's measured Enron P4 regression remains an unresolved liability,
not an accepted cost. Compare against its exact frozen executable to isolate
P011, and also against the safe reference and authors to judge the full candidate.
Do not inherit P006, P008, P009 or P010 policies. Root scheduling, shared P005
creation credits, branch partitions, pivots, pruning, pair classification and
maximality logic stay unchanged.

**Evidence and single hypothesis.** P010's additional per-update accounting and
global admission policy regressed in adjacent measurements. The older incomplete
739-call comparison is retained as historical screening evidence, not a complete
grid or a source of cross-batch speed ratios. It does not justify accepting P006
or P008. Existing diagnostic matrix fractions were only 8.07%/11.27% on
Enron/Wiki and zero on DBLP, with the previously recorded instrumentation limits.
The source exposes a more direct repeated cost: every addG/subG/addP/subP and
recovery scans the entire local adjacency row, even when that row is nearly full.
Reduce those updates to the shorter of the adjacency row and its closed
complement, maintaining exact logical counts with two private uniform offsets.
This is a source-supported cost center, not a measured claim that these loops
dominate total runtime. It can reduce useful work and heavy-path work without
another scheduling policy, per-node meter or new shared atomic.

**Paper grounding.** Dhulipala, Blelloch and Shun,
[Theoretically Efficient Parallel Graph Algorithms Can Be Fast and Scalable](https://arxiv.org/abs/1805.05208v4),
arXiv v4, sections 3 and 6, motivate separating work from depth and reducing
memory traffic/parallel overhead while preserving a defensible work bound.
P011 applies that discipline to local search updates. The offset/complement
representation and its k-plex proof below are our proposal; they are not a
k-plex algorithm, bound or performance result proved in that paper.

**Frozen representation and pseudocode.** For a root universe V of b vertices,
let N(u) be the existing local adjacency row and d(u)=|N(u)|. Select a complement
row exactly when `d(u) > b-d(u)`, avoiding overflow from `2*d(u)`. On a tie retain
the original adjacency row. A selected complement is **V minus N(u), including
u itself**, since the graph has no self loops. The rule has no tunable threshold,
worker dependence, graph name or machine parameter.

Use a small count view, for example `ShiftedCounts { int* raw; int bias; }`,
whose read-only indexing returns `raw[v]+bias`. Both P and G views are mutable
task-owned objects; the old const pointer for G does not mean its new bias is
const. Keep raw writes confined to the update primitive and initialization/copy.
For either count view X and sign s in {+1,-1}:

```text
update(X, u, s):
    if d(u) <= b-d(u):
        for v in N(u): X.raw[v] += s
    else:
        X.bias += s
        for v in V minus N(u): X.raw[v] -= s
```

The four old methods call this operation with their old sign and view. Root P
starts with raw zero and bias zero. Root G keeps the existing raw `degreeHop`
initialization and also starts with bias zero. Do not initialize G as full local
degree: its initial represented set is H={0,...,hopSz-1}. New roots reset both
biases; no prior stream offset survives.

**Implementation boundaries.** A root-owned, noncopyable `UpdateRows` object has
O(b) descriptors, borrows existing adjacency storage for sparse rows and owns
only selected dense complement buffers. In `buildSubgraph`, a row's degree is
known before the existing b-cell adjacency/matrix loop. Allocate its complement
only if selected and fill it in that loop's nonadjacent branch, including the
diagonal. This avoids a separate b-squared pass and any need to change graph
storage/destructors. Use suitable size/overflow checks for allocations; aggregate
lengths, if used, require size_t rather than int. A later matrix classification
byte is not a Boolean adjacency value: only its low bit means adjacency.
Descriptors and buffers are finalized before search and are then read-only.

Every semantic count read must use the corrected view: canFormPlex's two degree
tests and saturated-vertex scan; branchInPlexBase's tolerance; bound1/bound12;
listByCase's minimum-degree pivot/pruning; diagnostic printing; and the external
DecomposeMaxChecker saturation scan. Its left adjacency/binary-search logic is
unchanged. At `checkAsMaxPlex`, temporarily substitute the **entire G view** for
the P view, then restore the saved entire P view. A pointer-only substitution
would silently corrupt both local and global maximality checks.
At the sole P005 snapshot boundary, copy the two current raw arrays and both
bias values; share only the immutable UpdateRows by const reference. Do not
materialize normalized arrays, duplicate complement rows or reset child biases.
Retain the existing exclusion/exclBK alias mapping, checker/output privacy,
removed suffixes and all strict joins. This is one representation change, not a
new fork policy or a patch to the Parlay scheduler.

**Exactness, completeness and global maximality.** Maintain, at every observable
search point, `X.raw[v]+X.bias = |N(v) intersect X_set|` for all v, where X_set is
the P or G set represented by the existing code. Sparse updates are unchanged.
For a dense update every vertex first receives s through the uniform bias; the
closed complement receives -s in raw, so precisely the actual neighbors retain
the increment. Removal is the inverse, including fake/recover callbacks through
addG. Initialization, compact copying and whole-view substitution preserve the
invariant. Therefore all degree comparisons, saturated sets, bounds, pivots and
global maximality decisions equal those of the prerequisite on the same state.
Its complete/disjoint root and branch partitions remain complete and disjoint,
and its global-maximality decision is unchanged. No solution can be lost,
duplicated or made nonglobal by the representation. For the same sequence of
fork admissions, search-state decisions and ordering are identical. Actual
multiworker admission timing may differ through P005's existing shared credits;
do not assert equal cross-schedule node counts or reference work.

**Signed range, including the initial G offset.** Let D be the fixed dense-row
vertices and S=V minus D. At stable points,
`pBias=|P intersect D|` and
`pRaw[v]=|P intersect N(v) intersect S|-|P intersect D minus N(v)|`.
For G initialized from H with zero bias,
`gBias=|G intersect D|-|H intersect D|` and
`gRaw[v]=|H intersect D|+|G intersect N(v) intersect S|-|G intersect D minus N(v)|`.
Thus every bias and raw value is in [-b,b], using
`|H intersect D|+|S| <= b`; the loose bound 2b is unnecessary. Corrected counts
are the original [0,b-1] degrees. Raw G can attain +b, so neither unsigned raw
storage nor an asserted b-1 raw upper bound is valid. Primitive updates move
each element between these valid values; no reader observes a partially applied
update because the whole view is task-private and forks occur at existing safe
sites. Keep the existing signed int vertex/count domain and checked allocation
sizes; the representation requires no wider per-vertex count arrays.

**Comparable work accounting.** For root i, write b_i,l_i for its two universes,
M_i for the actual number of four-loop update calls, and u_t for the updated
vertex at call t. The old row-iteration cost is
`I_i=sum_t d_i(u_t)`; the new cost is
`J_i=sum_t min(d_i(u_t),b_i-d_i(u_t)) <= I_i`.
Calls, bias changes, row tests and corrected reads cost O(1) each; they are not
claimed free. Every old call already incurred O(1), even for degree zero.
If C_i is the total number of stored complement entries, then
`C_i=sum_{u in D_i}(b_i-d_i(u)) <= sum_{u in D_i}d_i(u)`.
Descriptor initialization, buffer construction and destruction cost O(b_i+C_i),
and fused construction has no worse bound than the already mandatory
O(b_i^2+b_i*l_i) block materialization. Count all allocations and writes.

P005 still makes at most b_i-1 compact snapshots per root, each
O(b_i+l_i), including all initialization, full current arrays/suffixes and
destruction. Copying two scalars and a table reference adds O(1) per snapshot;
the table is not cloned. Its monotone strong-CAS budget/retries retain the
previous O(b_i^2) charge, and root dispatch retains its existing atomic cost.
Let R_i be the remaining actual local work, B inherited preprocessing, N peeled
vertices and Q logical streams. Under the same unit-cost RAM/word-atomic model,

```text
W_P011 = O(B + P + N + Q*N
           + sum_i (b_i^2 + b_i*l_i + R_i + M_i + J_i))
       <= O(B + P + N + Q*N
           + sum_i (b_i^2 + b_i*l_i + R_i + M_i + I_i)).
```

The inherited `Q*N <= N+sum_v deg(v)^2` charging still applies. This compares
common structural terms and a fixed actual search trace, not measured P1 work
with a differently scheduled run. Constant extra reads/branches may still lose
wall time, and fewer useful memory updates do not prove lower native scheduler
CPU or faster end-to-end execution.

**Weighted span and live space.** Use the actual execution DAG, with unchanged
root allocation, first/rest fork/join and serial recovery dependencies. Charge
each update O(1+min(d,b-d)), each logical query O(1), each snapshot O(b+l),
construction O(b^2+b*l), and all joins/destruction. Include ordered atomic events
from the inherited root queue and per-root budget in the augmented weighted DAG;
an event finishes after both its local predecessor and preceding operation on
that atomic object. No new atomic events are introduced. The longest augmented
path is the span, conservatively at most total work. Dense updates on a critical
path may shorten, but a bound on total saved writes is not a balanced-search or
universal speedup theorem; construction/serial bounds/other roots can dominate.

Each UpdateRows lives inside its root callback until all descendants join and
root cleanup completes. Suspended ancestors continue to own their references.
The extra root storage is O(b_i+C_i), within O(b_i^2), plus O(1) per live state.
Retain P005's Q callback-owned dense scratch and historical root/bitset/matrix
capacities, immutable local adjacency/left edges, and full private recovery
state. Its conservative per-live-root task/recovery envelope remains
O(b_i^2+b_i*l_i+b_i^2*k); sum over all live roots, including suspended ones,
rather than substituting P live arrays. The new table is root-local, not TLS,
and is neither recycled before join nor retained with uncharged historical
capacity. Native runtime metadata and nonelastic idle CPU remain visible.
As before, dispatched exceptions are fatal; native par_do does not provide an
exception-safe join and no catch-and-continue path is added.

**Counterexamples and falsifiable predictions.** Omitting the diagonal from a
clique's complement wrongly increments the inserted vertex's own degree.
Ignoring a bias in any consumer is equally unsafe: take a local path a-c-b,
P=[a,b,c], k=2, and a left vertex x adjacent to b,c but not a. Only c selects a
complement; after insertion bias=1 and raw=[0,0,1]. Correct saturated vertices
a,b prevent x extending P; treating raw as counts falsely makes only c saturated
and can suppress this maximal answer. On a clique with initial H=V, removing all
G vertices yields gBias=-b and raw=b while true counts are zero. These are direct
representation/checker tests, not claimed benchmark families.

When no row is dense there is no saved row work, while metadata, query additions
and branch tests remain. Near-half rows may save only one write; a dense root
with little recursion can pay construction without amortizing it. Faster loops
can also expose P005 copy/scheduling overhead. The predeclared practical
prediction is lower P1 and P8/P16/P20 wall on search-heavy jazz(4,12) relative to
the exact P005 control, with no repeatable regression across the other small and
development cases. No minimum percentage, improved self-speedup or victory over
the authors is assumed. Separate diagnostic builds may count old eligible row
entries versus actual shorter-row entries on a few fixed development runs,
using task-private counters and join reductions with overflow-invalid reporting;
release has no such instrumentation. They measure the proposed mechanism,
not a whole-program work proof or release timing. The entire difficult suite
and unchanged authors are still needed for a positive performance decision.

**Exactly three test phases.**

1. **Correctness.** Use the canonical 19-call smoke, fixed 2234-case independent
   oracle with candidate P1/P4, relevant existing boundaries and fixed high-worker
   P2/8/16/20 exact-set repetitions. Reuse protected reference traces only after
   identity checks; do not repeat unchanged passed standalone units without a
   reason. Add a small direct count-view differential unit against explicit
   adjacency updates: empty/singleton, clique, path/star, odd/even b, exact-half
   ties, mixed sparse/dense rows, all initial H prefixes, add/remove recovery,
   negative residuals/G bias, nonzero-bias child snapshots and independent parent/
   child changes. Cover whole P=G view substitution/restoration and the saturated
   left-checker counterexample above. Diagnostic small-graph runs must exercise
   real forks with nonzero copied biases, recovery and corrected global
   maximality; they may check counts against explicit set/adjacency counts.
   Scope ASan/UBSan/leaks and repeated high-worker tests to these new ownership/
   arithmetic paths. Retain unsupported TSan as unsupported, never pass. No
   performance admission until actual source and evidence are reviewed.
2. **Small-graph performance.** Fixed jazz(3,10), ca-GrQc(3,10), jazz(4,12),
   P1/2/4/8/16/20, at least three repetitions. Compare P011, exact P005, immutable
   safe ListPlex, pristine FP O3, pristine PlexEnum O3 and its separate pristine
   author-default Ofast sensitivity: initially 324 calls. Freeze seeded
   (case,P,rep) blocks with all six solvers temporally adjacent and varied order;
   three repetitions do not exhaustively balance six positions. Every invocation
   checks input/binary identity and its known answer. Report every worker point,
   P1 overhead, absolute wall, same-binary self-speedup, CPU and RSS. GrQc's tiny
   calls remain visible with an honest noise interpretation. Suspected regression
   receives only a finite, discriminating adjacent confirmation; no post-hoc
   tolerance or expansion intended to hide unfavorable cells.
3. **Large-scale-graph performance.** If phase 2 survives, test the fixed
   Enron/Wiki/DBLP development cases and all five core/fixed held-outs, with the
   same default representation, comparators, worker curve and at least three
   interleaved repetitions. Wiki(4,20)=46729532, Epinions(4,30)=13172906 and
   held-out DBLP(4,12)=610150817 have retained same-input safe count evidence.
   Pokec(4,20) and Skitter(3,60) still need true counts established within this
   phase under fresh finite budgets; never use paper hints or retired processes.
   Prepare missing author-format caches only as needed, preserving graph/ID
   semantics. No held-out tuning and no mandatory separate real-count sanity
   round. Missing/time-limited cases remain pending and cannot support acceptance.

Each phase requires a concrete fresh-output resource grant: one compiler/solver
at a time, <=20 solver workers, <=160 GiB and <=70% available memory, original
coordination lock and pinned native Parlay `51017699dcc421f80479cdb238d3092233ad0d26`
with `-DPARLAY_ELASTIC_PARALLELISM=0`. The same release executable runs P1/P>1;
author runtimes/sources remain unchanged. Suggested small-screen limits are
120s per invocation, 14400s batch and 8 GiB per child; the coordinator freezes
actual resource/large-case budgets before execution. Registration itself launches
nothing. Wrong counts, violated ownership/range/work bounds, needless complexity
or repeatable P1/multiworker regression can reject early. A positive claim needs
broad baseline/author evidence and Astra review, not just reduced row writes.
After completed scope or justified rejection, apply the current scoped cleanup,
English-ledger mirroring and authorized normal same-name technique publication.


#### P011 R1 implementation and correctness execution

Sol implemented the registered representation in the exact P005 checkout; no
P010 admission policy, root scheduling, fork credits, branch partition or pair
classification change was inherited. Native fixed-runtime release and emission
binaries compiled successfully in `../build/p011-r1-20261026-1557`. Astra's actual
source review found no release correctness/lifetime blocker. Before any test,
diagnostic-only refinements added child/parent shadow equality, complete P/G bias
restoration checks and a small statistics-overflow fixture. Their fresh builds
are in `../build/p011-r1-reviewfix-20261026-1604`; initial diagnostic/unit outputs
are superseded and unrun. Both build controllers exited0. Full preprocessed bytes
for main.cpp/listPlex.cpp in release/emit modes are identical before/after, so the
initial two binaries are reused with explicit original-source provenance rather
than claimed rebuilt from final physical files. Original/final source snapshots
and both preprocessing identities are retained in the run directory.

The finite phase1 grant uses `../runs/p011-r1-20261026-1557/correctness.sh`: one
process, original coordination lock, <=20 workers (smoke4), ordinary8GiB address
limit and four-hour controller ceiling. Sanitizers use separate fresh outputs
and no inappropriate8GiB ASan virtual-address limit; actual RSS is recorded.
Components are the new direct differential/actual-checker unit, canonical smoke19,
2234 oracle cases with old protected reference traces reused only after identity
checks, relevant boundaries, fixed high-worker exact sets, final diagnostic
shadow/checker/exact-set coverage, ASan/UBSan/leaks. No unrelated unchanged storage
unit is recompiled/repeated. Execution outcomes remain pending until terminal
records; phase2/phase3 are unrun and require separate grants.


#### P011 R1 correctness evidence

The complete first phase finished with controller exit0 and no remaining solver
or compiler: **7090 logical records =4856 fresh calls+2234 verified reference
reuses**, plus the new differential/actual-checker ASan/UBSan/leak unit. Smoke19,
2234-case oracle6702, boundaries38, high-worker exact sets240, diagnostic44 and
sanitizer47 all passed. No count/set/duplicate/worker mismatch or sanitizer error
was observed. Peak recorded RSS was294088KiB. The fixed direct unit exercises
empty/singleton, clique/path/star/cycle, odd/even and exact-half ties, all H
prefixes, inverse/recovery updates, independent raw+bias snapshots, whole-view
substitution, cliqueGraw=b with bias=-b, the actual left-checker counterexample
and diagnostic overflow propagation. Unrelated standalone units were not rerun.

Gate `../runs/p011-r1-20261026-1557/correctness-gate.json` SHA256:
**682be62f7b1f5f3a06796b0fccf0635c6465e3067963cfff1b75af4ad309c4ac**.
Release SHA256:
**a58ca5b1d483a3f11179d2605b02ddfd1fd0c8d201878436b3ccfeee0b545511**.
ASan+UBSan+diagnostic+emitter SHA256:
**7103646cbb32e91ef0b696fd54c5d07b747bace2f64100b7ccf61249aad1da19**.
The gate pins final source identities/snapshot and the explicit original-release
physical-source/preprocessing equivalence provenance; old diagnostic/unit builds
remain superseded and unrun. Initial and diagnostic-revision build controllers
both exited0. TSan's earlier pre-main host incompatibility was not rerun and is
NOT a pass or race-freedom claim; native high-worker exact sets did run.

All diagnostic mutation shadow checks, actual checker adjacency comparisons,
child-vs-parent logical snapshot comparisons and root P/G bias restoration passed.
The44 diagnostic exact-set calls recorded353968 old eligible row entries versus
89206 actual shorter-row entries, with1314 forks,1084 nonzero P-bias forks,534
nonzero G-bias forks,3713 biased whole-view substitutions and4634 successful
restores. These are validation-only useful-row counters, not whole-program work
or release speedup measurements. Sanitizer coverage likewise exercised nonzero
bias clones and corrected global maximality. The single fixed random12/.5/seed0
k3q5 input at P4 independently produced102 exact sets with zero duplicates:
46 forks included11 P-bias and7 G-bias forks;54 biased swaps,129 restores;
old5166 versus actual4408 entries. Input SHA d0b1359e...40e684.

Source/evidence is now handed to Astra for actual review. Phase2's324-call
six-solver plan is prepared, hash-pinned and still unlaunched pending that review
and the root coordinator's separate finite grant. Phase3 remains unrun.


#### P011 R1 independent correctness review and small-graph grant

Astra independently audited the actual phase1 gate,7090 check/raw records,7039
exact-set hashes, source/binary/input/reference-reuse identities, unit evidence,
three exit0 controllers, original/final source snapshots and all four release/
emit preprocessing identities. It found no blocker and requested no additional
correctness repetitions. This admits the planned performance screen, not P011
acceptance.

The root coordinator grants **324** small-graph timings: three fixed cases,
six worker points and three repetitions of P011/P005/SafeOpenMP/FP/PlexEnumO3/
PlexEnum-author-default-Ofast. The final plan is
`../runs/p011-r1-small-plan-20261026-1610/plan.final.json`, SHA256
**373d205285b2e93b990bda1f80441e45a34759ef8c20444ce5ed1503a4fa4d52**; original ready plan remains
**45fe670b36848e0023406b139dfac6af26e685f9e6d58669da3cf1a741698fc3**.
Frozen driver SHA256:
**56bcce024ffb84f4fc406fd91e6ea78776867b80de4009bfc1e05bce7d8e678e**.
Fresh output: `../runs/p011-r1-small-20261026-1610`. The54 frozen six-solver
blocks keep case/worker/repetition controls adjacent; three varied positions
per solver are not exhaustive position balance. Budgets are120s per child,
14400s batch,8GiB address space, one solver, affinity0..19 and at most20 workers,
with current inventory/status/resource probing and the original coordination
lock. Every timing checks pinned source/provenance/gate/binary/input identity
and the known count. Safe P>1 remains a historical OpenMP control; authors retain
their runtimes/default tau and separate flags. Stage3 is not authorized here.


#### P011 R1 full small screen and finite P1 confirmation grant

All324 small-screen timings completed with correct counts/workers/resources,
controller exit0 and no remaining solver. Batch wall202.303401s, peakRSS5252352KiB
(the unchanged author implementation, within the8GiB child budget). Independent
auditing validated all raw records, frozen queue,108 complete unique summary
cells, source/binary/input/gate/provenance identities, CPU/RSS and self-speedups.
Measurements SHA256:
**5046651a659cac888763f666ba49c37ebf0f748160b35b1475776452c8defc43**.
All324 observations remain unchanged, with no trimming or retrospective
normalization. Full curves and paired ratios are retained in the original run.

The predeclared jazz(4,12) P1 reduction did not appear: adjacent P011/P005 wall
ratios1.0126/1.0167/1.0145 and CPU ratios1.0148/1.0190/1.0147 were slightly
worse, across both before/after relative positions. P20 wall medians were
P0110.2842s, P0050.2846s, FP0.1961s; same-binary self-speedups16.99x/16.69x/17.52x.
Other cells contain improvements and noise, so this is not a universal-regression
claim; nor does it establish the continuing broad improvement objective.

After Astra's complete screen audit, root authorizes exactly one finite
confirmation: **jazz(4,12), P1 only, P011/P005/Safe, three repetitions**, nine
calls, each solver once in each of three positions. Same binary/input/driver,
120s child timeout,900s batch,8GiB, affinity0..19, original lock, fresh output
`../runs/p011-r1-p1-confirm-20261026-1620`. Final plan SHA256:
**23889ea33ebfef82af66c61123450780e1daf904d2d941a32765958f75d0e0ef**.

**Decision rule frozen before launch:** if all three new adjacent P011/P005
wall ratios exceed1, combine with the original three and reject P011 for
repeatable regression, with no percentage tolerance introduced. Mixed directions
mean unstable/pending evidence, not a pass; there is no automatic phase3 or
additional testing. CPU and safe-reference comparisons provide independent
corroboration without a new resolution threshold. No source change, retuning,
extra case or author repetition is authorized by this confirmation.


#### P011 R1 final decision: rejected after finite P1 confirmation

The unique nine-call confirmation finished with all counts/workers/resources
correct and controller exit0. An independent raw/order/identity/summary audit
and Astra final decision review passed. Confirmation measurements SHA256:
**fcfca6198b796e3f43de64f363e271dae0d022b90d1560ab2748f0e5e564f3ae**.
The original324 calls and all nine new calls remain unchanged. No additional
solver, source revision, retuning or difficult-graph experiment followed.

New adjacent P011/P005 wall ratios, in repetition order, are
**1.016100/1.015291/1.014076**. Corresponding CPU ratios are
**1.016913/1.016878/1.012658**.
P011/Safe wall ratios are1.036007/1.038472/1.033376.
Every solver occupied each of the three positions exactly once. Median wall
seconds are P0114.827286, P0054.750799,
safe4.659510; median total CPU seconds4.81/4.74/4.64.
All three new P011/P005 ratios exceed1, as did the original three
(1.012581/1.016698/1.014501).
The frozen rule therefore declares repeatable P1 regression, with no new
percentage tolerance or sample filtering.

**Decision: rejected.** Correctness and the conservative common-model work bound
survived; practical non-regression and the predicted P1 reduction did not. The
complete six-solver screen also did not meet the continuing objective against
both authors: jazz(4,12) P20 self-speedup was16.99x for P011 versus17.52x for FP,
with absolute P20 walls0.2842s versus0.1961s. Better scaling than PlexEnum in this
small screen alone cannot establish the goal. Preserve the P16 local improvement
(P011/P005 paired0.965259/0.951331/0.978190) and
all noisy/tiny-case observations; this is not a claim of universal slowdown.
Fewer diagnostic row entries do not prove a whole-program performance gain, and
these tests do not isolate offsets, table construction, reads or scheduling as
the causal bottleneck.

Stage3 large/core/fixed held-out performance is **UNRUN by justified early
rejection**, not passed or accepted with missing cases. The hypothesis is closed
without changing any registered default or tuning on held-outs. Preserve the
tested source and compact audited evidence, apply dependency-checked lean
cleanup and publish only the same-name technique branch under the existing
authorization. No automatic mainline merge or mainline push is permitted.


#### P011 R1 scoped cleanup

All build/correctness/performance/confirmation controllers exited0, no P011 tmux
or executable remains live, and the original lock inode18779599 was acquirable.
The final rejection and all source/input/binary identities are recorded above.
A dependency check preserves the original P011 release for the root's future,
separate research-only profile; no such profile is part of P011 or launched here.
Before removal, `../runs/p011-r1-20261026-1557/cleanup-manifest.json` records each
disposable path/size/SHA and verifies duplicate reference outputs against their
protected originals. Remove two generated K4100 fixtures, superseded/unexecuted
full-main diagnostic/check-emitter/unit binaries, final rebuildable unit/ASan
binaries, and6702 copied reference stdout/stderr/resource files. Original P005
reference traces at each `reused_from` remain protected and available.

Retain the tested original release and emitter plus the reviewfix check-emitter
that actually supplied diagnostic exact-set evidence. Retain every fresh raw
correctness record and all333 performance raw records, plans/queues/frozen driver,
audits, source hashes, original release physical-source and final snapshots,
both preprocessing manifests, explicit reuse provenance, build/unit logs and
sanitizer build command. No shared data, other candidate, pending evidence,
immutable control/archive or uncertain-job file is removed.

Cleanup completed: **6710 artifact files / 164181629 bytes**,
plus only P011-created bytecode caches. Manifest SHA256:
**8abd4025b262f6f65df2118a81e181cf84b5f74c7eb3d7d0ff5719a046ce6ff5**. All three retained binary hashes
were verified after removal; the original release remains available for its
registered future dependency. No additional test or profile was started.
