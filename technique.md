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


### P012 - Lazy saturation prefixes within candidate-filtering passes (2026-10-26)

**Preregistration/status.** Proposed by `gpt-6-astra`/max; implementation belongs
to `gpt-6.1-sol`/high on `technique/p012-lazy-saturation-prefix`, in
`../worktrees/p012`. This registration is not an implementation, performance
admission or acceptance. P010 and P011 remain rejected; the continuing broad
baseline/author objective remains unfinished.

**Exact base and explicit prerequisite.** Start from P005
**1ace08d901d0f0bf93514af88cf5a473284e18e2**, whose algorithm is d066514, as a
**pending, unaccepted prerequisite**. Correct mainline remains
**6d035d10469fdc18a6bfcae767b1115c975432ee**. P005 supplies the reviewed native
dense-root streams and compact, bounded, strict within-root forks. Recreating
those ownership/runtime changes from the OpenMP mainline would confound the
single search-loop change; using R2 alone would remove useful within-root
parallelism. P005's unresolved Enron P4 regression is not waived. Compare with
its exact frozen executable, the safe ListPlex reference and unchanged authors;
improvement over P005 alone is insufficient for acceptance. Do not inherit
P006/P008/P009/P010 admission policies or P011 count offsets/complement rows.
Keep P005's counts, adjacency, branch partitions, pivots, bounds, root scheduling,
fork credits and global maximality checker unchanged.

**Evidence and hypothesis.** The completed four-call development observation
and identical-text offline attribution above provide concrete motivation. On
P005 jazz(4,12) and development DBLP(3,20), respectively, canFormPlex accounts for
716/4740 (15.11%) and409/3016 (13.56%) exclusive innermost samples. Its saturation
test line351 has407 and226 samples; the P-loop line347 has192 and89. These are
optimized DWARF/sample attributions, not precise operation costs. The underlying
summary SHA256 is
**e35c27ae27c8a4bd069ffec31723f777e60ea53b4713135f2ccd194d3bcd94a9**.
No held-out profiling or extra mechanism-probe round is required before this
candidate. Saturation is constant while a filtering pass repeatedly asks whether
different vertices can extend the same P. Discover saturated vertices only as
far as actual queries need them, then reuse that discovery within the pass.
This reduces repeated P/count scans without a full-P prepass, new shared atomic,
changed pruning rule or new scheduling policy.

**Paper grounding.** Dhulipala, Blelloch and Shun,
[Theoretically Efficient Parallel Graph Algorithms Can Be Fast and Scalable](https://arxiv.org/abs/1805.05208v4),
sections3 and6, separate work from depth and show why work, memory traffic and
parallel overhead must all be considered in practical graph algorithms. P012
uses that discipline to shorten private sequential work inside existing parallel
tasks. The lazy-prefix algorithm, k-plex argument and bounds below are original
adaptations; the paper proves neither this cache nor k-plex speedups.

**Frozen algorithm and interface.** Each KplexListor owns a reusable
`vector<int> saturationScratch`, initially empty. Each filtering invocation
creates a scope-local prefix helper which clears that vector and starts its
cursor at0. It borrows only this task's vector; it does not allocate/reserve b
entries in advance. Capacity grows lazily on actual discoveries and can survive
between passes. A fresh fork child starts with an empty vector and no inherited
logical cache. The helper is not copied across a fork.

For the fixed ordered P of a pass, the invariant is: cursor lies in [0,|P|], and
the cache consists exactly of the saturated vertices in P[0:cursor], in P order.
Retain the original two cheap tests, using the current G count on every query.
Then perform:

```text
canFormPlex(u, extra, prefix):
    if neiInP[u] + k < |P| + 1
       or neiInG[u] + k < max(lb, |P|) + extra:
        return false
    for v in prefix.saturated:
        if not adjacent(v, u): return false
    while prefix.cursor < |P|:
        v = P[prefix.cursor]
        prefix.cursor += 1
        if neiInP[v] + k == |P|:
            prefix.saturated.push_back(v)
            if not adjacent(v, u): return false
    return true
```

Append a newly discovered saturated vertex **before** its rejecting adjacency
test, so later queries must check it. Use the existing low adjacency bit of the
immutable pair matrix. Preserve debug assertions about u not belonging to P.
An initially cheap-rejected query scans no P entries, reads no cached entries
and triggers no capacity growth. The cheap G test is not cached or hoisted.

Create a separate helper for each invocation of exactly four existing sites:
`updateCand1`, `updateCand1Fake`, `updateExcl`, and the excluded-vertex loop in
`checkAsMaxPlex`. The first three finish their helper scope before returning to
recursion. In `checkAsMaxPlex`, first append C to P and substitute neiInG for
neiInP as before, then create the helper; finish its scope before restoring the
pointer and P size. Do not reuse one pass's logical prefix in the next pass,
even when P happens to look identical. The K-only filters retain their original
logic and need no cache. The external DecomposeMaxChecker receives the same
arrays and is unchanged. Do not add a threshold, eager scan, sorted saturation
list, cross-call validity machinery or P011 representation.

**Correctness and exact search decisions.** Within each of the first three
passes, P membership/order, |P| and neiInP are immutable. Removing candidates
updates G counts but cannot change which P vertices are saturated. In the
temporary P=G pass, both the substituted counts and appended P remain fixed
through the excluded-vertex queries. The helper therefore maintains its prefix
invariant by induction. A query examines exactly the saturated vertices in the
original P order, stopping at the same first nonneighbor as the original loop,
or exhausting P. Unsaturated vertices impose no condition and can be skipped
after their first discovery. The cheap tests remain identical and precede all
cache work. Thus every query returns the original Boolean on the same state.

Induct over each filtering loop: equal prior answers preserve its candidate
order/removals and current G counts, giving equal next cheap tests and answers.
All resulting candidate/exclusion states, bounds, pivots and maximality calls
are consequently unchanged for the same fork-admission trace. The prerequisite's
complete, disjoint root/branch partitions and global-maximality decisions are
preserved, so no maximal k-plex is omitted, duplicated or made nonglobal.
Timing can change which P005 shared credit is acquired by which branch; this
does not establish equal node counts across different parallel schedules or an
input-by-input work comparison to a differently scheduled reference.

**Comparable work, including cache traffic.** For a filtering pass and query j,
let r_j be the number of P entries the original linear loop would inspect after
its cheap tests, with r_j=0 for a cheap failure. Let a_j be the number of cached
saturated entries inspected and e_j the number of newly scanned P entries.
If rejection occurs inside the known prefix, a_j is at most that rejecting
entry's original P position and e_j=0. Otherwise, with previous cursor c,
`a_j <= c` and `e_j = r_j-c`. Therefore, for every query,

```text
a_j + e_j <= r_j.
```

Each newly discovered saturated vertex is appended once within its pass, so
total pushes are at most sum_j e_j. Count those writes, cursor updates, vector
capacity allocation/growth/copy/destruction, O(1) helper setup/query overhead,
and clearing old contents. Standard geometric vector growth is amortized linear
in entries pushed; each old entry can be cleared only after a charged push.
There is no O(b) initialization on a cheap-only pass. This proves an asymptotic
work bound, not fewer machine instructions or no allocation overhead.

For root i, let F_i be its pass count, L_i its predicate-query count, X_i=sum_j r_j,
Y_i=sum_j(a_j+e_j), and R_i its remaining actual search/recovery/output work.
The cache costs O(F_i+L_i+Y_i), with Y_i<=X_i; original filter entries and
queries already cost Omega(F_i+L_i). Root graph/matrix materialization remains
O(b_i^2+b_i*l_i). P005 makes at most b_i-1 compact snapshots, each O(b_i+l_i),
including initialized arrays, live stacks, removed suffixes, checker storage
and destruction. Child cache construction is O(1), not a vector copy.
The existing monotone strong-CAS budget/retry cost retains its O(b_i^2) charge;
root-dispatch atomics are unchanged. With inherited preprocessing B, peeled
vertices N and logical streams Q, the same RAM/word-atomic model gives

```text
W_P012 = O(B + P + N + Q*N
           + sum_i (b_i^2 + b_i*l_i + R_i + F_i + L_i + Y_i))
       <= O(B + P + N + Q*N
           + sum_i (b_i^2 + b_i*l_i + R_i + F_i + L_i + X_i)).
```

The inherited `Q*N <= N+sum_v deg(v)^2` dense-stream charge still applies.
The comparison uses common structural parameters and a fixed actual search
trace, not P1 measured work versus a different multiworker execution. There is
no added factor depending on worker count and no uncharged initialization,
packing, output or snapshot cost.

**Weighted span and live/suspended space.** Keep the actual fork/join DAG and
its serial filtering, recovery and construction dependencies. Assign each
query its O(1+a_j+e_j) weight plus actual amortized vector operations; a growth
event itself has its full copy weight on that path. Include O(b_i+l_i) snapshots,
O(b_i^2+b_i*l_i) materialization, joins, cleanup and ordered atomic events from
both the global root queue and each root's fork-credit counter. An atomic event
depends on its program predecessor and the previous event on that object.
The longest path of this augmented weighted DAG is the span, conservatively
at most W_P012. Less repeated scanning can shorten a heavy branch, but does
not prove balanced tasks, an additive cross-root atomic bound or universal
speedup. P005's admission bottlenecks and native idle CPU are unchanged.

One task's cache capacity is O(b_i) under geometric growth, even while that task
is suspended; each helper adds O(1) automatic storage. A root has at most b_i
listor states including its original and compact children, so aggregate added
cache capacity is O(b_i^2), including ancestors retained through strict joins.
The full inherited per-live-root task/recovery envelope remains conservatively
O(b_i^2+b_i*l_i+b_i^2*k), with the new cache included. Also retain Q stream-owned
dense scratch and historical buffer/matrix capacities, immutable graph/left
storage, output and runtime metadata. Sum over all live roots and suspended
states; neither P live arrays nor only running children is a valid space bound.
No mutable cache or scratch is TLS/shared between callbacks. Its owner survives
every child join; a child never borrows the parent's vector. Existing dispatched
exception handling remains fatal, since native par_do lacks a recoverable
exception-safe join; add no catch-and-continue path.

**Risks and falsifiable predictions.** An eager full-P saturation prepass can
turn a cheap or first-position rejection into Theta(|P|) work and is excluded.
Not caching a rejecting saturated vertex, retaining a cache after a P mutation,
or using the old P cache during P=G can produce wrong answers. All-saturated P,
one-query passes, or frequent immediate cheap/early rejection can save little
while paying helper/vector constants. First-time discoveries allocate; pointer
loads and an extra loop boundary may offset avoided count reads. These are
plausible counterexamples to speedup, despite the work bound. Existing addG/subG
and bounds remain substantial costs; the profile does not predict closing the
entire FP gap with this single idea.

The practical prediction is lower P1 wall on search-heavy jazz(4,12) against
exact P005, useful reduction on development DBLP(3,20), and no repeatable
P1/multiworker regression across the fixed cases. Predict at least some
P8/P16/P20 absolute-wall improvement, not necessarily a larger self-speedup
ratio. No minimum percentage or author victory is assumed. A diagnostic-only
reference predicate can check every cached query's Boolean and prefix-visit
inequality on selected small tests, with private counters, checked/overflow-
invalid totals and join reductions. Release has no mechanism counters or
global diagnostic atomics. These correctness checks are not a new profile or
performance stage and their timings are never acceptance evidence.

**Exactly three test phases.**

1. **Correctness.** Canonical19-call smoke, fixed2234-case independent oracle
   at candidate P1/P4, relevant boundaries and fixed P2/8/16/20 exact-set stress.
   Verify reused reference source/binary/input/trace identities; do not repeat
   unchanged passed reference checks. Add a small direct unit of the actual
   cache/predicate: empty and singleton P; no/all/mixed saturation; first cheap
   failure leaving cursor/cache unchanged; rejecting a newly discovered vertex
   then replaying it; rejection before the old cursor; passing the known prefix
   and extending it; live G reductions changing only cheap tests; reset after
   P changes; and lazy capacity growth. Cover the actual P=G substitution and
   restoration, real fork children with empty independent scratch, parent/child
   subsequent mutations, and repeated recovery. Diagnostic differential checks
   require the original predicate's result and `a_j+e_j<=r_j` per query. Start
   with the existing20 fixed random plus2 star fixtures for cache/alias/fork
   coverage; if a required path is absent, use a finite recorded subset of the
   existing fixed random pool and stop once coverage is obtained. Scope
   ASan/UBSan/leaks and high-worker repetitions to the new ownership paths.
   Previously unsupported TSan remains unsupported, not pass. Astra reviews
   actual source and evidence before performance admission.
2. **Small-graph performance.** Fixed jazz(3,10), ca-GrQc(3,10), jazz(4,12),
   P1/2/4/8/16/20, three interleaved repetitions; P012, exact P005, immutable
   safe ListPlex, pristine FP O3, pristine PlexEnum O3 and separate pristine
   author-default Ofast: initially324 calls. Freeze seeded adjacent six-solver
   (case,P,rep) blocks and varied positions; three repetitions do not balance
   all six positions exhaustively. Every call checks identities and the known
   count. Report all absolute wall, P1 overhead, same-binary self-speedups,
   CPU/RSS and same-worker comparisons, including tiny/noisy GrQc. For a
   suspected consistent regression, allow one finite discriminating confirmation:
   one selected case/P, P012/P005/Safe, three repeats rotated through all three
   positions (9 calls), with the comparison and decision rule fixed before it
   runs. If all original and all confirmation candidate/control wall ratios
   exceed1, reject that repeatable per-case regression; use CPU and raw timing
   spread as supporting evidence, not a post-hoc tolerance. Mixed confirmation
   is unresolved, not acceptance or automatic phase3 admission. A clear
   correctness/work/ownership failure or already decisive repeated performance
   failure can close the trial early; retain unrun planned cells explicitly.
3. **Large-scale-graph performance.** Only after phase2 survives, cover fixed
   Enron/Wiki/DBLP development cases and all five core/fixed held-outs, with
   unchanged defaults, the same six comparators, full worker curve and at least
   three interleaved repeats. Retained safe counts are Wiki(4,20)=46729532,
   Epinions(4,30)=13172906 and held-out DBLP(4,12)=610150817. Establish true
   Pokec(4,20)/Skitter(3,60) counts within this phase under fresh finite budgets;
   do not substitute paper counts or restart retired jobs. Author-format caches
   preserve exact graph/ID semantics and are prepared only when needed. No
   separate mandatory real-count sanity round, held-out tuning or silent case
   replacement. Missing/censored cases remain pending. No optional extended
   case is needed for a new scheduling/memory claim, since none is made here.

Each phase needs its own fresh-path resource grant and preflight: original lock,
one compiler/solver at a time, <=20 solver workers, <=160 GiB and <=70% currently
available memory. Keep pinned native Parlay
`51017699dcc421f80479cdb238d3092233ad0d26`,
`-DPARLAY_ELASTIC_PARALLELISM=0`, requested/observed workers and the same release
binary at P1/P>1; references/authors keep their original runtimes. Suggested
phase1/2 limits are14400s batch and8 GiB per child, with smoke<=120s and small
timings<=120s per call. The coordinator freezes finite phase3 per-case/batch
and memory budgets before execution; timeout/OOM/incomplete is not a timing.
No registration launches a build or solver. Broad baseline/author evidence and
Astra review are required for a positive decision. After planned completion or
justified rejection, use the current scoped cleanup, English-ledger mirroring
and authorized normal same-name technique publication; publication alone is
not acceptance.


#### P012 R1 implementation and correctness

Sol implemented the registered four-pass lazy prefix on exact pending P005
`1ace08d901d0f0bf93514af88cf5a473284e18e2`, in
`technique/p012-lazy-saturation-prefix` / `../worktrees/p012`. Each listor owns
only reusable vector capacity; every pass clears its logical prefix and creates
an uncopied automatic cursor. The original live cheap tests precede every
cache access. Newly found saturated vertices are appended before rejection.
The temporary P=G helper ends before pointer/size restoration. Fork children
construct empty independent scratch and join before diagnostic reduction.
Counts, matrix, bounds, pivots, partitions, root scheduling, credits and the
external global maximality checker remain unchanged.

Fresh paths are `../build/p012-r1-20261026-1713`,
`../build/p012-r1-unitfix-20261026-1715`,
`../build/p012-r1-sanitize-20261026-1713`, and
`../runs/p012-r1-20261026-1713/`. Initial make-j1 compiled the release, emitter
and check-emitter successfully, then exited2 at a unit-only missing dependent
`typename`. Before any test, Astra also found the synthetic129-entry growth
fixture's default counts100 caused cheap rejection; the corrected unit uses
n-1 and retains separate cheap-failure tests. A fresh unit-only directory built
successfully. No solver algorithm/header changed after physical compilation;
only `tests/p012_prefix.cpp` differs between initial and final C/C++ manifests.
A make-only dependency fix lists directly included listPlex.cpp for that unit.
`unitfix-provenance.json`, `release-source-provenance.json`, original/final
hash manifests, build logs and `tested-source/` preserve these distinctions.

The local correctness controller1921092/start_ticks200035695 completed exit0
in180.819s, and its process/tmux plus all children ended. The original lock
inode18779599 was again freely acquirable. Preflight observed local Ubuntu,
24 CPUs/affinity0-23, no CPU quota and about239 GiB available. The finite grant
was14400s, <=20 workers and8 GiB per process; ordinary validators enforced
8 GiB virtual space, while ASan shadow space remained unrestricted virtually.
The outer controller sampled total child-tree RSS every100ms and would stop
above8 GiB. No compiler, data preparation or solver competed with this batch.

All **7090 logical correctness records passed:4856 fresh calls and2234
identity-verified reused reference traces**, plus the separate actual-cache
ASan/UBSan/leak unit. Components: smoke19, fixed2234-graph independent oracle
at P1/P4 (6702 records including reference reuse), boundaries38, fixed high-worker
exact sets240, diagnostic44 and sanitizer47. No additional fixed-pool cases
were needed. Peak recorded solver RSS was294004 KiB. Reuse preserved exact
safe source/binary/input/trace identities and original P005 dependencies; no
control was rebuilt. TSan's prior pre-main host mapping incompatibility remains
**unsupported, not passed**, and was not rerun.

Validation-only code compares every actual cached query's Boolean to the
original predicate, verifies the exact ordered saturated-prefix invariant,
checks `cached visits + newly scanned P entries <= original P entries` per
query, and verifies cheap failure leaves cursor/cache/capacity unchanged.
Checked private counters merge only after strict joins; overflow invalidates
reported totals. The44 diagnostic calls observed53861 queries,45883 cached
visits plus78710 new P entries versus260435 original P entries;26461 pushes,
6294 newly discovered rejection cases,883 cached rejections and2901 prefix
extensions. They exercised4634 actual P=G passes,1314 real fork children and
153 forks with nonempty parent scratch. Every child began empty, retained
independent backing storage and completed before reclamation. These diagnostic
counts validate this finite trace; they are not whole-program speedups or a
comparison across different fork schedules. Sanitizer47 separately covered
53819 queries and1314 forks without reported ASan/UBSan/leak errors.

The actual-helper unit additionally covers empty/singleton P, no/all/mixed
saturation, cheap-zero work, rejecting-then-replaying vertices, rejection before
the old cursor, extension, live G reductions, reset after P changes, growth to129
entries, three actual P=G substitutions/restores, real native four-worker fork
independence and checked-counter overflow. It passed with sanitizers/leaks.

Release SHA256: **c8721a61998811fd6cb7edc45eaa3fd4abb32519a0609a7e3a3a6f5338c19e19**.
Emitter SHA256: **2bd86bd8572f57bc7be5a275d134381130c3a3444e934f59c8bbb3ed3a1dbeca**.
Diagnostic emitter SHA256: **c839a0ace16cf4da37f857fc4aaf429968133b7936ff6bd1dfc8aaeff3e65525**.
Sanitizer emitter SHA256: **2ea3e7e5e3c031aadcc6db1d384fba48086b4533e2902593a3ca0cdcaa9d0533**.
Unit SHA256: **9ac967b9226be1be039f7466d87580632e6196c0b0d3952045dbf7f3ed8b7cff**.
Final C/C++ manifest (44 files) SHA256:
**1bf36f71acb059d3046234df0627bb75c56ebb28bd5dbd2ffea5a7613b79138b**.
Release physical-source provenance SHA256:
**56c3baa3ef642416dc2218eb8aaf0d19e5e61fbb59b8bcf2b1129bd6db233386**.
The read-only full raw/count/resource/source/input/binary/reused-trace audit
passed; preliminary correctness gate SHA256:
**4d8fd633ab12823df91d163e40189ff0d2b203ba3b47605cfe22348644a2ceae**. Its `performance_admission=false` correctly preserves the pending
Astra actual-evidence review and separate coordinator phase2 grant.

Stage2 is only prepared: fixed324-call six-solver plan and frozen driver in
`../runs/p012-r1-small-plan-20261026-1717/`, with exclusivity false and no timing
invocation. Stage3 remains UNRUN. Correctness alone is not acceptance; the
continuing broad baseline/author/core/held-out objective remains pending.


#### P012 phase1 Astra review and limited admission

Astra independently reviewed the actual source and retained evidence and found
no blocker: raw7090 records/counts/workers/resources/run.json,7039 exact-set
hashes,2234 reused original-reference triplets, final source/binary/support
identities, per-query differential/prefix work checks, actual P=G/fork ownership
and unit-only pretest correction provenance all matched. No additional solver
or correctness check was required. This is admission only to the registered
finite phase2 small-graph screen, not a performance pass or technique acceptance.

The preliminary gate **4d8fd633ab12823df91d163e40189ff0d2b203ba3b47605cfe22348644a2ceae** remains
unchanged. `correctness-gate.admitted.json` SHA256:
**3a2dc5139e787b6b00f96b5eb39b5ad7ab0ecd4f6c1efaa7a4ba0d242157170e**. `admission-metadata-delta.json` records only the changed
review/admission metadata and verifies source, binary and test fields unchanged.
`plan.ready.json` references this new gate and still has exclusivity false until
the separate coordinator grant. Neither phase2 nor phase3 has run.


#### P012 phase2 coordinator grant

After Astra's actual phase1 review and root's independent readiness audit of
the admitted gate, all source/binary/provenance/input identities, frozen driver,
324 unique timings and54 adjacent blocks, the coordinator grants only phase2:
the fixed three cases, six solvers, P1/2/4/8/16/20 and three repetitions.
`../runs/p012-r1-small-plan-20261026-1717/plan.final.json` freezes this grant,
with the earlier ready plan and driver unchanged. Budgets:120s per call,
14400s batch,8 GiB per child, affinity0-19, <=20 total solver workers and the
original lock inode18779599; one compiler/solver at a time. Run fresh local
inventory/status/resource preflight and keep all completed/censored cells.
No tuning, extra confirmation or phase3 is authorized by this window. Completion
will be audited before interpreting full curves and paired comparisons.


#### P012 R1 complete small-graph screen (performance, not acceptance)

All324 frozen timings completed with exact counts, requested/observed candidate
workers and complete resource records. Outer controller1956025/start200098378
exited0 after200.052s; the session/controller ended and original lock18779599
was acquirable. The read-only queue/raw/count/resource/source/input/binary audit
passed with108 unique summary cells and54 adjacent blocks. No solver or compile
was added. Evidence: `../runs/p012-r1-small-20261026-1717/` and corresponding
`-small-plan-` directory; `analysis.json` retains every paired ratio, all wall
ranges, same-binary speedups, CPU and RSS. Three repetitions rotate six solver
positions without exhaustive six-position balance. Tiny/noisy GrQc observations
remain included. GNU time CPU includes native idle/spinning; author thread fields
are configuration, separate from external OS-thread observations.

Audit SHA256: **985cca09614805a7a758c74e53b55469d15fa101c7c8bcfe66dbe695388c178d**.
Measurements SHA256: **083f8df241f25361e7c879029dd1d45d671e6a2077fc3a67750f4c8a352cc63d**.
Analysis SHA256: **876015d08466f7a765e29705baa8aa038825e2f96b5d42e5b73da783b70a42fb**.

Complete median end-to-end wall seconds (same release at P1/P>1):

jazz-k3-q10:

| Solver | P1 | P2 | P4 | P8 | P16 | P20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| P012 | 0.414141 | 0.222854 | 0.124361 | 0.074729 | 0.046569 | 0.043142 |
| P005 | 0.410501 | 0.218291 | 0.121533 | 0.072495 | 0.043565 | 0.044564 |
| SafeOpenMP | 0.401417 | 0.204863 | 0.119636 | 0.073053 | 0.046100 | 0.041487 |
| FP | 0.342923 | 0.183702 | 0.101661 | 0.059599 | 0.038831 | 0.037097 |
| PlexEnum | 0.448275 | 0.334234 | 0.319261 | 0.360443 | 0.472238 | 0.536701 |
| PlexEnum-author-default-Ofast | 0.444718 | 0.333138 | 0.321098 | 0.364157 | 0.477377 | 0.529670 |

ca-GrQc-k3-q10:

| Solver | P1 | P2 | P4 | P8 | P16 | P20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| P012 | 0.026177 | 0.025191 | 0.023496 | 0.020592 | 0.020554 | 0.014181 |
| P005 | 0.026151 | 0.023646 | 0.021585 | 0.017543 | 0.020221 | 0.017098 |
| SafeOpenMP | 0.026375 | 0.023286 | 0.018326 | 0.020067 | 0.020179 | 0.020088 |
| FP | 0.034131 | 0.026922 | 0.023357 | 0.023731 | 0.025228 | 0.022297 |
| PlexEnum | 0.166515 | 0.195149 | 0.250467 | 0.328047 | 0.459379 | 0.527946 |
| PlexEnum-author-default-Ofast | 0.167010 | 0.198401 | 0.255190 | 0.330143 | 0.459995 | 0.523922 |

jazz-k4-q12:

| Solver | P1 | P2 | P4 | P8 | P16 | P20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| P012 | 4.753166 | 2.429776 | 1.231356 | 0.628193 | 0.341017 | 0.273754 |
| P005 | 4.755664 | 2.437279 | 1.230929 | 0.628099 | 0.335060 | 0.287045 |
| SafeOpenMP | 4.653117 | 2.369484 | 1.212017 | 0.642216 | 0.364719 | 0.310158 |
| FP | 3.435820 | 1.738872 | 0.879505 | 0.448143 | 0.240253 | 0.194190 |
| PlexEnum | 3.290708 | 1.766396 | 1.035596 | 0.722090 | 0.661200 | 0.700761 |
| PlexEnum-author-default-Ofast | 3.331317 | 1.776548 | 1.042052 | 0.727483 | 0.672111 | 0.679245 |

Median adjacent P012/P005 wall ratios by P1/2/4/8/16/20 are:

- jazz-k3-q10: 1.010016, 1.012203, 1.023272, 1.022584, 1.027966, 0.968088.
- ca-GrQc-k3-q10: 1.000996, 1.001767, 1.088546, 1.261993, 0.983564, 0.854638.
- jazz-k4-q12: 0.998400, 0.996023, 1.001281, 1.000809, 1.024612, 0.970652.

The search-heavy predicted P1 reduction did not materialize: jazz(4,12)
P012/P005 paired walls0.998141/0.998400/1.000108, with median CPU4.74s for
both. Its P16 ratios1.024612/1.014232/1.027479 all exceed1, while P20 improves
0.970652/0.904313/0.974698. Preserve both directions and the unchanged strategy.
P20 P012 self-speedup is17.3629x versus FP17.6931x; absolute walls0.273754s
versus0.194190s. This screen alone does not meet the continuing author objective.

The strongest clean P1 non-regression concern is jazz(3,10): all three adjacent
P012/P005 wall ratios exceed1 (1.010016/1.015043/1.008867), with all three CPU
ratios1.025641 at GNU time's coarse0.01s resolution. Median wall is0.414141s
versus P0050.410501s and Safe0.401417s. At P2 andP8, all three paired wall
ratios also exceed1. These observations warrant the single preregistered finite
discriminating confirmation, with its scope/rule frozen before execution; no
confirmation is launched by this entry. Stage3 remains UNRUN and ungranted.
No acceptance, rejection, tuning, trimming or extra solver follows this screen
automatically. Astra/root should assess whether jazz(3,10) P1 confirmation can
resolve the practical non-regression requirement.


#### P012 phase2 frozen-plan metadata erratum

Astra found copied P011 descriptive metadata in the immutable final plan:
top-level grant time08:14:22 and ready-plan SHA45fe670b... were stale; the new
nested P012 coordinator grant correctly records09:26:48 and the actual P012
ready-plan SHA is **b4f85147e9b89889d8a7fffefa8407cf2bb2d1cde19b23fe2b4f8578424718e1**.
The copied candidate-admission text also mentioned PP provenance; P012 instead
uses byte-identical actual solver sources/headers across the unit-only pretest
fix, as its physical-source provenance records. Sol retained this descriptive
metadata when adapting the prior plan. `../runs/p012-r1-small-plan-20261026-1717/metadata-erratum.json`
records exact stale/actual values. The frozen plan/driver/raw data remain
unchanged; actual new authorization,324 queue entries and all identities were
already audited correctly. No timing needs repetition. The unexecuted pending
confirmation plan removes these inherited fields and explicitly records9 calls.


#### P012 single finite P1 confirmation grant and frozen rule

Astra independently audited all324 raw timings,108 cells and270 paired ratios;
root verified the nine-call pending plan and all identities. Root grants exactly
jazz(3,10), P1, P012/P005/Safe, three repeats rotated through every one of the
three positions (9 calls),120s per call/900s batch/8 GiB under original lock.
The new final plan clarifies this complete three-position balance; the earlier
324-call frozen plan and its metadata erratum remain untouched.

The frozen comparison is P012/P005 whole-process adjacent wall. All original
three ratios were1.010016/1.015043/1.008867. If all three new ratios also exceed1,
reject the repeatable per-case P1 regression, with no new tolerance, trimming or
sample replacement. Mixed confirmation is unresolved, not acceptance or automatic
phase3 admission. CPU/spread and Safe support interpretation; they do not change
the rule. This one confirmation controls a specific confound: original P012 was
immediately after P005 in all three blocks, whereas the new rotated blocks give
each solver each position. No other solver/build/phase3 is granted.


#### P012 R1 completed finite P1 confirmation

Exactly9 additional frozen jazz(3,10) P1 timings completed with count257233,
outer exit0 and every solver occupying each of all three positions exactly once.
The controller1973695/start200158964 and local tmux ended at09:36:58Z; no child
remains and original lock18779599 was freely acquirable. Raw/order/identity/
count/resource audit passed. Evidence:
`../runs/p012-r1-p1-confirm-20261026-1733/` and its corresponding plan directory.
Audit SHA256: **9ca5d35090938b724ac6c2f40c1c893893eb7742d7ac4f01f814cccaa04f6a32**.

New P012/P005 wall ratios are **1.017472/1.035348/1.017377**; P012/Safe ratios
are1.030108/1.049100/1.035834. The original P012/P005 ratios were
1.010016/1.015043/1.008867. All six exceed1, meeting the exact frozen repeatable
per-case P1 regression rule with no added percentage tolerance or sample removal.
New CPU ratios support the same direction (1.025641/1.051282/1.025641), while
retaining GNU time's coarse resolution. Median wall seconds are P0120.413029,
P0050.402629 and Safe0.397352; median CPU0.40/0.39/0.38 seconds.

The proposed decision is **reject**, pending Astra's actual confirmation review;
no extra confirmation or phase3 will run automatically. Correctness and the
common-model asymptotic work argument survived, but practical non-regression
failed on jazz(3,10) P1. The favorable P20 observations in the complete screen
remain recorded and do not erase this repeated per-case regression. This does
not claim slowdown at every P or input, nor isolate the cache/allocation/loop/
scheduling mechanism as its cause. Fewer diagnostic visits did not establish a
whole-program gain. If confirmed, phase3 is UNRUN by justified early rejection,
not passed with missing core/held-out evidence. The continuing broad objective
remains unfinished.


#### P012 R1 final Astra decision: rejected

Astra independently reviewed the complete nine-call confirmation, its raw
counts/workers/resources/source/binary identities and order, and recomputed all
new paired ratios. The rotated confirmation covers every solver position and
both relative P012/P005 orderings; all new ratios exceed1, as did all original
three. **P012 is rejected under its preregistered non-regression rule.** No
additional test, tolerance, tuning, confirmation or phase3 is required. Phase3
is **UNRUN by justified early rejection**, not passed or accepted with missing
large/core/fixed held-outs. Preserve all333 timing records and the favorable
jazz(4,12) P20 observations. The conclusion is this trial's repeated jazz(3,10)
P1 regression, not universal slowdown or a causal claim about vector overhead.
The broad continuing objective remains unfinished and no mainline merge occurs.

#### P012 R1 dependency-checked lean cleanup

All known build and test controllers have ended; correctness/small/confirmation
exits are0, including all333 count-checked timings, and original lock18779599
was acquirable. The only failed build was the explicitly recorded unit-only
pretest compile, repaired before any execution. Before deletion,
`../runs/p012-r1-20261026-1713/cleanup-manifest.json` records each proposed file's
path/size/SHA and protected original where applicable. The plan removes exactly
6710 files /152995534 bytes:6702 byte-identical copied reference outputs, two
deterministic K4100 fixtures, rebuildable unit/sanitizer binaries, two duplicate
small sanitizer build-provenance files already migrated into the run evidence,
and two P012 bytecode caches. Protected original2234 reference traces remain.

Retain the tested release/emitter/check-emitter, all fresh raw correctness
records and logical reference rows with exact reuse provenance, all333 raw
performance records, plans/frozen drivers/queues/audits/analyses, both gates,
final C/C++ snapshot, initial/final source manifests, unitfix/physical compile
commands and logs, sanitizer build provenance, unit raw success and metadata
errata. No shared data, immutable control/archive, other candidate, pending job
or uncertain process file is removed. Tested source/branch/history remain.
The actual admitted binaries/source/tests are unchanged; a post-test read-only
analysis helper and result documentation may be added for publication, with
separate support identities rather than pretending they were compiled solvers.


Cleanup completed: **6710 files /152995534 bytes**. Cleanup manifest SHA256:
**ae9015b7dd657adae8687c32cc879c78fe15f1fc7edf188af73fa4e34d89f06f**. All three retained executable hashes were verified after removal;
all protected original reference dependencies remain. The only post-test support
changes are final reproduction/result documentation and a read-only analysis
helper, recorded in `publication-support-sha256.json`; no solver/validation/
measurement code or tested C/C++ identity changed. No new test was run.
