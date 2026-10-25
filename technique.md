# ListPlex Parallel Techniques

## Scope and current state

This is the English experiment ledger on the repository mainline, **`armAndX86`** (verified as the remote default branch on 2026-10-25). Do not rename it to `main` or treat a technique branch as the ledger's authoritative copy.

Future candidate algorithms use **the native ParlayLib scheduler** from [joey001/parlaylib](https://github.com/joey001/parlaylib.git), initially pinned to `51017699dcc421f80479cdb238d3092233ad0d26`. OpenMP, Cilk/OpenCilk, TBB, and independent thread-pool backends are outside the candidate implementation contract. The archived ListPlex reference and unchanged author comparators retain their historical runtime. This policy change does not port the solver yet.

[GBBS](https://github.com/ParAlg/gbbs) is a useful source of Parlay graph-algorithm patterns; it is neither a mandatory dependency nor the boundary of permitted ideas. Its `gbbs/bridge.h` exposes Parlay loops and fork-join operations. GBBS currently pins a different Parlay revision: inspect compatibility instead of silently replacing the selected fork. Preserve source attribution and applicable licenses for adapted code.

**No Parlay technique has been implemented, measured, accepted, or rejected yet.** Existing hardening and OpenMP measurements are historical evidence, not experiments on the proposals below. The three branches below are reserved for independent trials; an empty branch is not an attempted implementation. No graphs are downloaded and no experiments are launched by maintaining this ledger.

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
| P001 | Native Parlay root parallelism | `technique/p001-parlay-roots` | planned; branch reserved | not validated | not run |
| P002 | Bounded fork-join splitting of heavy subtrees | `technique/p002-bounded-fork-join` | planned; branch reserved | not validated | not run |
| P003 | Shared immutable blocks with compact task state | `technique/p003-shared-task-state` | planned; branch reserved | not validated | not run |

### P001 - Native Parlay root parallelism

**Hypothesis.** Run the existing independent root computations with `parlay::parallel_for`, preserving the serial search within each root. Use native default granularity first, task-safe scratch ownership and local counts followed by reduction. This proposal deliberately keeps the existing search as a minimum runtime-port control, not a claim of a new search algorithm; that design choice does not prohibit algorithm adaptations in other proposals.

**Work.** A proposed decomposition gives `W = W_ref + O(R) + C_init`. Since the reference must already visit its roots, `O(R)` is chargeable. Acceptance still requires proving `C_init = O(W_ref)`: eagerly zeroing graph-sized scratch for every worker can introduce `O(P*n)` work and is not automatically acceptable. All affected preprocessing must be included; unchanged serial phases remain serial.

**Span.** Under a binary fork-join parallel loop, `S = O(S_pre + log(R+1) + max_i(W_i + A_i))`, where `A_i` charges initialization/counter overhead on the strand that runs root `i`. If those costs are charged into root work, this reduces to the familiar root-granularity bound. One heavy root remains a serial bottleneck. This is a model-level bound to verify against the actual loop and ownership design.

**Simplicity/parameters.** No new algorithm knobs. Use the pinned native scheduler and worker count `P`; avoid copying the global graph or creating a scheduler per root. Scratch cannot be reused while a suspended task still owns it.

**Experimental analysis.** Not run. First pass `dev_smoke`, remote oracle/race/memory checks and `remote_sanity`. Compare reference one-thread, candidate native P=1 and candidate P=2/4/8/16/20; use the archived same-P legacy control to expose any runtime-port regression. Freeze the development/held-out split from the manifest. Report per-case timing and RSS, root duration distribution and initialization/copy work. No measured speedup or generalization claim is available.

**Closure.** Accept only if the complete work accounting and broad non-regression tests pass. Reject a root-port design that adds unchargeable worker-scaled work or repeats clear serial/parallel regressions. Base/implementation commits and run paths: pending implementation.

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
