# ListPlex 并行开发约定

开始修改、编译或测试前，读取项目根目录 `../config.md`、本仓库 `docs/parallel-development.md`、英文主线台账 `technique.md` 和 `tests/runtime-policy.json`；测试参数的唯一结构化清单为 `tests/test-suites.json`。

- 固定归档串行参考保持不变；候选技术分支允许为并行化主动调整relaxclq的实现和局部算法逻辑。当现有设计不亲和并行时，可改变pivot、分支、剪枝、搜索/任务划分、排除与恢复及极大性检查的实现。必须保持目标问题和输出集合语义正确（完整、无重复、全局极大的k-plex及相同k/q定义），不要求逐句保留原串行搜索。
- 固定参考见 `tests/baselines/listplex-hardened-2026-10-25.json`。从其归档独立构建1线程参考；不要使用变化中的工作树替代，不覆盖历史归档。
- 日常开发先用dev_smoke，默认参考1线程+候选1/4线程，6例×3次+存储检查1次=19次；本地单进程、单例10秒、总计120秒。Windows超出120秒/20例、完整回归或性能测试目标是Ubuntu；已在Ubuntu开发时这些任务直接本机运行，不SSH自身。
- 调度、任务状态、共享可变数据、队列/窃取、内存池或所有权变化必须通过Ubuntu测试主机上的dev_regression（在Ubuntu开发即本机执行）及重复多线程小图核对，再进入remote_sanity、perf_core。更重的extended/large/stress按研究目标选择，不默认展开全套。
- 小图以独立oracle核对完整集合、重复输出和计数；真实图要求相同输入SHA与参考计数。论文计数和时间仅作选例依据。
- 共享只读结构与任务私有可变结构分清所有权；任务不得依赖可能被嵌套执行或迁移覆盖的thread_local缓冲区，内存池槽位需在全部使用者结束后回收。
- Ubuntu测试主机当前24vCPU，默认总预算20线程、整批160GiB且不超过可用内存70%；性能一次一个进程，1/2/4/8/16/20线程每组至少3次。同程序1线程是自身加速比分母，保留串行参考的改善另报。
- 新增的清单目前是声明式计划，历史runner不自动遵守它；执行前明确子集、哈希、编译、超时、内存和批次预算。未准备的数据标待准备，timeout/OOM/crash不算零解或完成。
- 保留用户已有改动、作者代码及既有实验原始证据；本轮仅维护规则时不启动算法实验或自动下载数据。

## 强制运行时、技术分支与验收

- 新候选必须基于用户指定的 `https://github.com/joey001/parlaylib.git` 原生调度器；初始SHA见runtime-policy。使用PARLAY_NUM_THREADS与num_workers控制/核实worker；不使用OpenMP等其它后端或独立线程池。历史参考/作者对照例外保留原运行时，不能冒充新候选。
- GBBS可借鉴但不限定方案来源，不必引入GBBS；核对指定fork兼容性并保留代码出处。
- 追求small-and-sweet：一项清晰思想，保持渐近work，完整计入状态复制/初始化/同步，分析work与span；优先原生原语与普适默认值，避免大量机器/图相关参数。
- 主线是armAndX86，在其technique.md用英语维护每项方案及理论和实验分析。先登记方案，再在其独立technique/<id>-<name>分支开发。主线只维护规则/台账和验收结论，不直接堆试验实现。
- 必须检查one-worker开销与多worker泛化表现、固定留出案例；明显且可重复的串行/并行退化不能用均值掩盖。两道门槛都通过才标accepted并结束；尝试无法满足则在主线标rejected/当前路径不可取并保留证据。缺测仅标pending/blocked，不关闭为成功。
- 维护配置不自动启动实验、下载图或依赖，也不自动推送/删除分支；未来runner需落实当前schema3清单的Parlay worker/运行时与验收契约。

## 允许面向并行化的局部算法调整

- 上述许可适用于当前并行策略研究：必要的局部代码和算法逻辑调整可以直接在对应技术分支实施，无需仅因触及算法条件而再次请求许可。保留固定参考供独立对照，不能修改历史参考来迁就候选结果。
- 每种技术仍是一项清晰主假设，可包含使该并行策略成立的必要局部算法调整。在主分支英文technique.md记录原设计的并行障碍、具体改变、正确性论证、可比参数下的work/span/空间分析及实验影响；无关优化另开方案。
- small-and-sweet的work约束按同一问题、参数和模型比较渐近上界，不能隐瞒额外因子；不把“必须逐实例保持相同搜索树/节点数”作为禁止算法调整的门槛。若声称更强的逐实例work保证，需单独证明。
- 对算法逻辑改变增加针对性小图oracle和反例回归，并在同一候选中比较1worker与多worker。报告算法自身改变带来的work/剪枝差异和并行扩展收益，不把混合收益全归为调度改进。原有正确性、少参数、跨案例非退化及分支验收要求继续适用。

## 开发机与测试机角色（所有分支适用）

项目根目录的config.md与execution-environments.json决定实际机器角色；Ubuntu下运行 `python3 ../tools/project-context.py --operation inventory`（从relaxclq目录）核对。在 `/home/joey/k-plex-rethinki-ubuntu/relaxclq` 工作时，开发机与测试机相同，直接本机编译、回归及性能测试，长任务用本机tmux，不通过SSH/SCP重复派发给自己。历史remote_sanity/where=remote只是Ubuntu测试目标名称。Windows笔记本120秒/20例门槛不套用为Ubuntu的异机迁移要求；Ubuntu烟测默认4workers，重任务总预算仍为20workers/160GiB及可用内存70%。外层root/tools/config及仓库内相对结构两端一致；历史src快照不是当前开发仓库。
