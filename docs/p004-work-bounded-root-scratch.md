# P004: work-bounded reusable dense root streams

Status: implementation and correctness validation in progress; no performance claim.
Base: armAndX86 3d2f47d. Branch: technique/p004-work-bounded-root-scratch.
Native Parlay: joey001/parlaylib 51017699dcc421f80479cdb238d3092233ad0d26.

## One idea and exact changes

After the ordinary serial peel/order phase, launch Q logical streams using one
native parlay::parallel_for with grain 1. Each stream owns reusable dense
visited, common-neighbor count, block-membership and local-search buffers.
A relaxed atomic next-root counter assigns each root exactly once. A stream
never forks or invokes nested Parlay while it owns scratch. Scratch belongs to
the callback, not a worker ID or TLS slot; stealing before callback execution
cannot create aliases. Each root search itself uses the existing serial branch
methods. Per-stream 64-bit output counts are reduced only after the join.

The emitter/original-ID instrumentation, serial preprocessing barrier, serial
root-search interface and obsolete OpenMP-task removal are explicitly reused
from rejected P001 as runtime scaffolding, not as an accepted sparse-state
technique. P004 replaces its hash maps and per-root search allocations with
ordinary dense arrays and reusable search containers. Mainline safe sequence
counting is retained. No OpenMP/Cilk/TBB/custom pool is linked to the candidate. Compile-time backend
guards reject accidental OpenMP/Cilk/Parlay-alternative builds; the make default
is the native P004 target. Callback-local result aggregation publishes once after
the stream finishes, avoiding hot writes to adjacent result records.

The inherited legacy residual visited[start] on an early-pruned root is cleared
explicitly. Thus scratch begins each root with zero visited/count/membership
flags. This may change later decomposition/left-check work versus the archived
legacy run; we claim equal output semantics, not identical visited-node work.
No pivot, branching feasibility, global maximality or threshold is changed.

## Correctness and ownership

Peeling/order are complete before root callbacks start. Graph/order/original-ID
maps are shared read-only. Existing decomposition assigns a plex to its unique
minimum vertex in the completed degeneracy order; all roots remain present and
are fetched exactly once. Serial branch/exclusion/maximality semantics inside
each root are preserved. Consequently the root union is complete/disjoint.
Resetting stale scratch removes dependence on which previous root a stream
received; it does not delete any candidate. All touched decomposition counts
and marks, block/left membership flags, and search candidate sets are restored
before that stream fetches another root. Checker/emitter and graph views are
root-local; graph shared_ptr owners outlive the listor and checker. No mutable
buffer or count is concurrently read/written by two callbacks. Atomic fetch is
only a unique index allocator, so relaxed order is sufficient; the native
fork/join publishes immutable input and joins result publication.

## Work, span and space

Let N,M,Delta be the peeled graph size, undirected edges and maximum degree,
P the requested native worker budget, D2=sum_v deg(v)^2, B the serial input,
peel and degeneracy work, and R_i the candidate serial decomposition, block
materialization, local enumeration and output work for root i. R_i includes
b_i^2 common-matrix initialization/fill, left-block scans, local graph arrays,
search/recovery allocation, maximality checker initialization and emission.
The literal code also charges input-graph preprocessing even if peeling is empty.

Q=min(P,N,1+floor(D2/N)) for N>0, otherwise Q=0. Saturate the D2 sum at
(min(P,N)-1)*N; this yields the same capped Q without overflow. This scan costs
O(N), does not require a graph/hardware table and adds no tuning parameter.
Then Q*N <= N+D2 <= N+N*Delta^2. Dense scratch is seven bytes per vertex per
stream (three byte flags plus an int count), initialized exactly once per
stream. Root assignment costs O(N+Q) atomic operations under the unit-cost
word-RMW model. Serial result reduction costs O(Q). std::vector growth is
amortized over writes; resize zeroing is charged to the root matrix/search
materialization already counted in R_i. RandSet/Stack growth and copies are
explicitly included in R_i, never hidden as scheduler overhead.

The reused cand1/cand2 clear operations scan their entire historical flag
capacity, not only current b_i. Let H_s(i) be the retained maximum block size
in stream s at root i. Charge C_clear=sum_i O(ceil(H_s(i)/32)) separately
for the fixed 32-bit membership words. Because every block lies in a root
two-hop neighborhood, H_s(i)<=min(N,1+Delta^2), hence
C_clear=O(N*min(N,1+Delta^2))=O(N+N*Delta^2). This bound intentionally uses
the common worst-case parameters; sparse later roots need not individually
pay for a previous large capacity.

W = O(B + N + Q*N + sum_i R_i + C_clear), where R_i now explicitly excludes
the separate historical-capacity clears. Therefore under the ordinary common full
worst-case upper-bound model, the added dense initialization is covered by
N+N*Delta^2, the serial two-hop envelope (whether pruning actually visits those
two hops on this instance or not). No hashing expectation is required. This
is not a per-instance claim W_candidate=O(actual W_reference); neither the
serial barrier nor cleared stale flags guarantees the same search tree.

For the actually assigned stream lists A_s, ignoring hardware cache/RMW
latencies, a safe span upper bound is
S = O(B + log(Q+1) + N + max_s [N + sum_{i in A_s} (R_i + clear_i)] + Q).
The serial structural-cap scan costs O(N) work and O(N) span. Each of Q
callbacks initializes its dense arrays serially in O(N) span; native grain-one
forking adds O(log(Q+1)) span before callbacks. These dense-initialization
paths execute in parallel and contribute N, rather than Q*N, to span. The
serial post-join reduction contributes O(Q) span. The N term also conservatively
charges a fully serialized atomic frontier. It is
incorrect to replace stream chains with max_i R_i: callback streams remain
serial, and indivisible expensive roots/imbalance are the central limitation.
No unconstrained root-DAG work-stealing theorem is claimed for this design.

Auxiliary space is O(N + Q*N + sum_live (b_i^2 + b_i + l_i + recursion_i)),
plus immutable input/core graphs and retained capacities from the largest
previous root per stream. More precisely the stream matrix capacities sum over
per-stream historical maxima; they are not all freed at each root. Only Q roots
are active. Default shared_ptr graph views avoid adjacency copies; subgraph
adjacency itself is materialized once per root. A 64-bit result can still
overflow when the mathematical output count exceeds 2^64-1, as in the reference.

## Build and validation protocol

Run directly on Ubuntu from this worktree:

    make parlay-root-streams parlay-root-streams-emit

Targets resolve outer execution-environments.json and pinned deps/parlaylib;
binaries go to build/p004. No SSH occurs on the Ubuntu build/test path.
Same release binary with PARLAY_NUM_THREADS=1 and >1 is used for scaling.
Correctness runner tests/p004_validation.py adapts the existing independent
subset-definition oracle. Frozen/reference-safe identities remain unchanged;
symlinks in build/p004 point to immutable build/p001 reference executables.
Smoke: 19 calls at candidate P1/P4. Full oracle: 2234 cases, candidate P1/P4
new executions, old reference exact-set evidence revalidated by binary/input
SHA and expected full sets; historical reuse is recorded explicitly. Boundary,
ASan+UBSan+leaks, repeated P2/P3 and later coordinated high-worker stress and
remote_sanity must precede performance admission. TSan compatibility is
reported separately and never inferred from sanitizer success.

Predeclared graph/parameter/holdout set is unchanged from tests/test-suites.json
and mainline technique.md. No performance job starts without the coordinating
agent granting a timing-only window. Significantly exceeding both untouched
author implementations means their same parallel executable P1/self-speedup
controls, plus non-regressing absolute/serial performance, not a slow P1 trick.

## Correctness evidence before high-worker admission

Frozen algorithm commit: `41c5dac4b4943258d092d28fb2b7b349c278ef8f`.

- `smoke`: complete, 19 logical checks, `runs/p004-final-smoke/status.json`.
- `exhaustive`: complete, 8936 logical checks, `runs/p004-final-exhaustive/status.json`.
- `boundaries`: complete, 38 logical checks, `runs/p004-final-boundaries/status.json`.
- `stress`: complete, 120 logical checks, `runs/p004-final-stress/status.json`.
- `sanity`: complete, 16 logical checks, `runs/p004-final-sanity/status.json`.
- `sanitizers`: complete, 30 logical checks, `runs/p004-final-sanitizers/status.json`.

Oracle contains 2234 cases: 4468 new candidate P1/P4 exact-set runs and
4468 explicitly reused frozen/safe-reference records rechecked against
binary/input hashes and the independently recomputed complete oracle sets.
The stress/sanity counts above are LOW-worker checks only (stress P2/P3;
sanity P1/P2/P3) and do not replace required high-worker admission.

TSan compatibility: `unsupported-environment`; see
`runs/p004-final-sanitizers/tsan-status.json`. No TSan race-freedom claim.

Release binary SHA256: `d661de3e2c473fe16bae2a6f1de622f99c19a8f72cc508d918ba2aedd5f7ed1d`.
Emit binary SHA256: `3404f08326642c8250454950bf96630b4099355a246de72364edcc4f1f7752e5`.

No performance measurement has been run by this technique owner.
