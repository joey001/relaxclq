#!/usr/bin/env python3
"""Nine-call single-cell confirmation; default only prints the frozen plan.

Derived from the exact screen driver, with unchanged per-invocation measurement,
identity/count/runtime/resource checks. A fixed-P confirmation has no P1
measurement or self-speedup; never borrow a denominator from another batch.
"""
import argparse,fcntl,hashlib,json,os,random,re,resource,signal,statistics,subprocess,time
from pathlib import Path
ROOT=Path('/home/joey/k-plex-rethinki-ubuntu')
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb') as f:
  for chunk in iter(lambda:f.read(1024*1024),b''):h.update(chunk)
 return h.hexdigest()
def save(p,d):
 p=Path(p);tmp=p.with_suffix(p.suffix+'.tmp');tmp.write_text(json.dumps(d,indent=2)+'\n');tmp.replace(p)
def peer_input(path):
 path=Path(path);dest=ROOT/'datasets/parallel-peer-inputs'/(sha(path)+'.author.bin');dest.parent.mkdir(exist_ok=True)
 import struct
 with path.open('rb') as src:
  assert struct.unpack('<I',src.read(4))[0]==4
  raw=src.read()
 if dest.exists():assert dest.read_bytes()==raw
 else:dest.write_bytes(raw)
 return dest

def parse(kind,stdout,stderr,rc,p):
 counts=re.findall(r'Number of (?:\d+-plex|plexes)\s*:\s*(\d+)',stdout)
 count=int(counts[-1]) if counts else None
 if kind=='FP':
  seen=re.findall(r'threads=(\d+)',stdout);complete=rc==1 and 'Enum time:' in stdout and count is not None
 elif kind=='PlexEnum':
  seen=re.findall(r'Thread number\s*:\s*(\d+)',stdout);complete=rc==0 and 'Running time' in stdout and count is not None
 elif kind=='candidate':
  seen=re.findall(r'(?:Parlay workers|PARLAY_WORKERS|parlay_workers)\s*[:=]\s*(\d+)',stdout+'\n'+stderr);complete=rc==0 and count is not None
 else:seen=[];complete=rc==0 and count is not None
 observed=int(seen[-1]) if seen else None
 if kind in ('FP','PlexEnum','candidate'):complete=complete and observed==p
 phases={}
 for label,value,unit in re.findall(r'^([^\n:]+time):?\s*([0-9.]+)\s*(ms|msec|sec|s)?\b',stdout,re.M|re.I):
  phases[label.strip()+'_s']=float(value)/(1000 if unit in ('ms','msec') else 1)
 # FP Reading time is clock() CPU time, not wall; keep raw metric but never sum it.
 if kind=='FP' and 'Reading time_s' in phases:phases['Reading_CPU_time_s']=phases.pop('Reading time_s')
 return count,observed,complete,phases

def resource_metrics(timing):
 fields={'user_cpu_s':r'User time \(seconds\):\s*([0-9.]+)','system_cpu_s':r'System time \(seconds\):\s*([0-9.]+)','cpu_percent':r'Percent of CPU this job got:\s*([0-9.]+)%','voluntary_context_switches':r'^\s*Voluntary context switches:\s*(\d+)','involuntary_context_switches':r'^\s*Involuntary context switches:\s*(\d+)'}
 result={}
 for key,pattern in fields.items():
  m=re.search(pattern,timing,re.M);result[key]=(int(m[1]) if 'switches' in key else float(m[1])) if m else None
 result['total_cpu_s']=result['user_cpu_s']+result['system_cpu_s'] if result['user_cpu_s'] is not None and result['system_cpu_s'] is not None else None
 return result

def diagnostic_metrics(stdout):
 labels={'fork_snapshots':r'Fork snapshots','snapshot_base_capacity_bytes_cumulative':r'Snapshot base capacity bytes(?:\s*\(cumulative\))?','root_streams':r'Root streams','dense_scratch_bytes_reported':r'Dense scratch bytes'}
 result={}
 for key,label in labels.items():
  m=re.search(r'^\s*'+label+r'\s*:\s*(\d+)',stdout,re.M|re.I)
  if m:result[key]=int(m[1])
 return result

def make_queue(plan):
 workers,solvers,cases=plan['workers'],plan['solvers'],plan['cases']
 repeats=plan.get('repetitions',3)
 rng=random.Random(plan.get('seed',20261025))
 orders={}
 for ci,c in enumerate(cases):
  for p in workers:
   order=list(solvers);rng.shuffle(order);orders[(c['id'],p)]=order
 blocks=[(c,p,rep) for rep in range(repeats) for c in cases for p in workers]
 rng.shuffle(blocks)
 queue=[]
 for c,p,rep in blocks:
  order=orders[(c['id'],p)];shift=rep%len(order)
  queue.extend((c,s,p,rep) for s in order[shift:]+order[:shift])
 assert len(queue)==len(cases)*len(workers)*repeats*len(solvers)
 assert len({(c['id'],s['id'],p,rep) for c,s,p,rep in queue})==len(queue)
 for start in range(0,len(queue),len(solvers)):
  block=queue[start:start+len(solvers)]
  assert len({(c['id'],p,rep) for c,s,p,rep in block})==1
 return queue

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--plan',type=Path,required=True);ap.add_argument('--out',type=Path);ap.add_argument('--run',action='store_true');ap.add_argument('--validation-only',action='store_true');args=ap.parse_args()
 plan=json.loads(args.plan.read_text());workers=plan['workers'];solvers=plan['solvers'];cases=plan['cases'];repeats=plan.get('repetitions',3)
 assert workers and all(isinstance(p,int) and 1<=p<=20 for p in workers)
 confirmation=plan.get('single_cell_confirmation') is True
 if confirmation:
  assert not args.validation_only and repeats==3 and len(workers)==len(cases)==1 and len(solvers)==3
  assert {s['id'] for s in solvers}=={'P013','P005','SafeOpenMP'}
 assert args.validation_only or (repeats>=3 and (1 in workers or confirmation)),'Full curves require P1; only explicit nine-call single-cell confirmation may omit it'
 queue=make_queue(plan)
 if not args.run:print(json.dumps({'planned_invocations':len(queue),'workers':workers,'validation_only':args.validation_only,'cases':[c['id'] for c in cases],'solvers':[s['id'] for s in solvers],'all_execution':'Ubuntu direct','out':str(args.out)},indent=2));return
 assert args.out;args.out.mkdir(parents=True,exist_ok=False)
 context=json.loads(subprocess.check_output(['python3',str(ROOT/'tools/project-context.py'),'--operation','regression' if args.validation_only else 'performance','--workers',str(max(workers))]));assert context['development_equals_test'] and context['transport']=='direct'
 lock=(ROOT/'runs/parallel-exploration-performance.lock').open('r+');fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
 if not args.validation_only:assert plan.get('exclusive_window_authorized') is True,'Root must grant exclusive timing window'
 for s in solvers:
  assert sha(s['binary'])==s['binary_sha256'],s
  if 'release_source_provenance_sha256' in s:
   assert sha(s['release_source_provenance'])==s['release_source_provenance_sha256']
  if 'identity_sha256' in s:
   assert sha(s['identity'])==s['identity_sha256']
  if 'source_archive_sha256' in s:
   assert sha(s['source_archive'])==s['source_archive_sha256']
  if 'correctness_gate_sha256' in s:
   assert sha(s['correctness_gate'])==s['correctness_gate_sha256']
  if 'source_identity_sha256' in s:
   assert sha(s['source_identity'])==s['source_identity_sha256']
   identity=json.loads(Path(s['source_identity']).read_text())
   assert identity['reference_safe_sha256']==s['binary_sha256']
  for rel,digest in s.get('source_sha256',{}).items():
   assert sha(Path(s['source_repository'])/rel)==digest,rel
  if s['kind']=='candidate':
   assert 'libgomp' not in subprocess.check_output(['ldd',s['binary']],text=True)
   if not args.validation_only:
    gate=json.loads(Path(s['correctness_gate']).read_text());assert gate['status']=='passed' and gate['performance_admission'] is True
    assert s['binary_sha256'] in [gate.get('release_binary_sha256'),gate.get('candidate_binary_sha256')]
  elif s['kind'] in ('FP','PlexEnum'):
   identity=json.loads(Path(s['identity']).read_text());entry=next(x for x in identity['entries'] if x['name']==s['kind']);assert entry['binary_sha256']==s['binary_sha256']
   for rel,digest in entry['source_sha256'].items():assert sha(Path(entry['source'])/rel)==digest
   if not args.validation_only:
    gate=json.loads(Path(s['correctness_gate']).read_text());assert gate['status']=='passed' and gate['binary_sha256'][s['kind']]==s['binary_sha256']
 for c in cases:
  assert sha(c['input'])==c['input_sha256'];assert isinstance(c['expected_count'],int) and c['expected_count']>=0
  if any(s['kind'] in ('FP','PlexEnum') for s in solvers):c['peer_input']=str(peer_input(c['input']));c['peer_input_sha256']=sha(c['peer_input'])
 assert set(range(20)).issubset(os.sched_getaffinity(0))
 metadata={'driver_sha256':sha(__file__),'plan_sha256':sha(args.plan),'plan':plan,'validation_only':args.validation_only,'context':context,'hostname':os.uname().nodename,'kernel':os.uname().release,'affinity':sorted(os.sched_getaffinity(0)),'load_before':os.getloadavg(),'meminfo_before':Path('/proc/meminfo').read_text(),'cpu_max':Path('/sys/fs/cgroup/cpu.max').read_text() if Path('/sys/fs/cgroup/cpu.max').exists() else None,'compiler':subprocess.check_output(['g++','--version'],text=True),'started_utc':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),'ssh_dispatch':False,'worker_observation_warning':'FP/PlexEnum printed thread fields report configuration, not runtime observation; external_peak_solver_threads is sampled OS-thread count, not active workers or physical cores. Short jobs may be unobserved.','thread_sampling_interval_s':0.05,'diagnostic_semantics':{'fork_snapshots':'Reported completed snapshots, not task steals','snapshot_base_capacity_bytes_cumulative':'Cumulative base heap capacity reported by candidate; not copied bytes, not peak memory, excludes allocator/temporary overhead','root_streams':'Reported logical streams, not proof all workers executed them','dense_scratch_bytes_reported':'Reported dense global-vertex scratch estimate; not total RSS or peak live state'},'CPU_metric_warning':'GNU time CPU includes useful work plus scheduler spinning/stealing/runtime overhead; observed CPU is not a proof of O(useful W). Context switches are OS counts, not task steals.','phase_warning':'Author internal phase definitions differ. FP Reading is CPU time. Primary end_to_end uses monotonic subprocess wall for all variants.'};save(args.out/'metadata.json',metadata)
 save(args.out/'queue.json',[{'case':c['id'],'solver':s['id'],'workers':p,'repetition':rep,'block':i//len(solvers),'position':i%len(solvers)} for i,(c,s,p,rep) in enumerate(queue)])
 rows=[];begin=time.monotonic()
 for index,(c,s,p,rep) in enumerate(queue):
  assert sha(s['binary'])==s['binary_sha256']
  assert sha(c['input'])==c['input_sha256']
  available=int(next(l.split()[1] for l in Path('/proc/meminfo').read_text().splitlines() if l.startswith('MemAvailable:')))*1024
  memory=plan.get('memory_gib',48)*1024**3;assert memory<=160*1024**3 and memory<=.7*available
  assert time.monotonic()-begin<plan.get('batch_budget_s',43200),'Batch budget exhausted; retain partial results'
  d=args.out/f'{index:04d}-{c["id"]}-{s["id"]}-p{p}-r{rep}';d.mkdir()
  if s['kind']=='FP':argv=[s['binary'],c['peer_input'],f'-k={c["k"]}',f'-q={c["q"]}',f'-t={p}']
  elif s['kind']=='PlexEnum':argv=[s['binary'],c['peer_input'],'-k',str(c['k']),'-q',str(c['q']),'-t',str(p),'-tau','0.1']
  else:argv=[s['binary'],'-f',c['input'],'-m','maximal','-k',str(c['k']),'-l',str(c['q']),'-t','86400']
  affinity='0-'+str(max(workers)-1) if args.validation_only else '0-19'
  argv=['/usr/bin/time','-v','-o',str(d/'time.txt'),'taskset','-c',affinity,*argv]
  env=dict(os.environ,LC_ALL='C',PARLAY_NUM_THREADS=str(p),OMP_NUM_THREADS=str(p),OMP_DYNAMIC='FALSE',OMP_MAX_ACTIVE_LEVELS='1',OMP_PROC_BIND='close',OMP_PLACES='cores',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
  def limits():resource.setrlimit(resource.RLIMIT_AS,(memory,memory));resource.setrlimit(resource.RLIMIT_CORE,(0,0))
  start=time.monotonic();proc=subprocess.Popen(argv,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=env,start_new_session=True,preexec_fn=limits);timeout=False
  # External /proc observations do not modify author code. Apply identical
  # 50ms sampling to all solvers; short jobs may have no usable observation.
  thread_samples=[];deadline=start+plan.get('case_timeout_s',1800)
  while True:
   try:
    children=Path(f'/proc/{proc.pid}/task/{proc.pid}/children').read_text().split()
    for child in children:
     cp=Path('/proc')/child
     try:
      if (cp/'exe').resolve()==Path(s['binary']).resolve():
       status_text=(cp/'status').read_text();tm=re.search(r'^Threads:\s*(\d+)',status_text,re.M)
       if tm:thread_samples.append(int(tm[1]))
     except (FileNotFoundError,PermissionError,ProcessLookupError):pass
   except (FileNotFoundError,ProcessLookupError):pass
   remaining=deadline-time.monotonic()
   if remaining<=0:
    timeout=True;os.killpg(proc.pid,signal.SIGKILL);stdout,stderr=proc.communicate();break
   try:stdout,stderr=proc.communicate(timeout=min(.05,remaining));break
   except subprocess.TimeoutExpired:continue
  wall=time.monotonic()-start;(d/'stdout.txt').write_text(stdout);(d/'stderr.txt').write_text(stderr)
  count,seen,valid,phases=parse(s['kind'],stdout,stderr,proc.returncode,p)
  status='timeout' if timeout else 'completed' if valid else 'failed'
  if status=='completed' and count!=c['expected_count']:status='count-mismatch'
  if 'incomplete' in stdout.lower()+'\n'+stderr.lower():status='incomplete'
  timing=(d/'time.txt').read_text();rss=re.search(r'Maximum resident set size \(kbytes\): (\d+)',timing)
  cpu=resource_metrics(timing);diagnostics=diagnostic_metrics(stdout)
  if status=='completed' and any(v is None for v in cpu.values()):status='missing-resource-metrics'
  row={'index':index,'block':index//len(solvers),'block_position':index%len(solvers),'case':c['id'],'solver':s['id'],'kind':s['kind'],'workers':p,'configured_workers':seen if s['kind'] in ('FP','PlexEnum') else None,'observed_runtime_workers':seen if s['kind']=='candidate' else None,'external_peak_solver_threads':max(thread_samples) if thread_samples else None,'external_thread_samples':len(thread_samples),'repetition':rep,'status':status,'exit_code':proc.returncode,'count':count,'expected_count':c['expected_count'],'wall_s':wall,'phases':phases,'peak_rss_kib':int(rss[1]) if rss else None,'cpu_resources':cpu,'diagnostics':diagnostics,'binary_sha256':s['binary_sha256'],'input_sha256':c['input_sha256'],'command':argv,'affinity':affinity,'validation_only':args.validation_only,'directory':d.name};rows.append(row);save(d/'run.json',row);save(args.out/'measurements.json',rows);save(args.out/'progress.json',{'completed':len(rows),'planned':len(queue),'last':row,'elapsed_s':time.monotonic()-begin});print(json.dumps({k:row[k] for k in ['case','solver','workers','status','count','wall_s']}),flush=True)
  assert status=='completed','Admission halted; retain evidence: '+status
 summary=[]
 if not args.validation_only:
  for c in cases:
   for s in solvers:
    for p in workers:
     group=[x for x in rows if x['case']==c['id'] and x['solver']==s['id'] and x['workers']==p];ok=[x for x in group if x['status']=='completed'];complete=len(ok)==len(group)==repeats
     summary.append({'case':c['id'],'solver':s['id'],'workers':p,'completed':len(ok),'total':len(group),'median_wall_s':statistics.median(x['wall_s'] for x in ok) if complete else None,'min_wall_s':min((x['wall_s'] for x in ok),default=None),'max_wall_s':max((x['wall_s'] for x in ok),default=None),'median_peak_rss_kib':statistics.median(x['peak_rss_kib'] for x in ok) if complete else None,'status_counts':{t:sum(x['status']==t for x in group) for t in set(x['status'] for x in group)}})
  for entry in summary:
   group=[x for x in rows if x['case']==entry['case'] and x['solver']==entry['solver'] and x['workers']==entry['workers'] and x['status']=='completed']
   def summarize_values(values):
    return {'reported_runs':len(values),'median':statistics.median(values) if values else None,'min':min(values) if values else None,'max':max(values) if values else None}
   entry['cpu_resources']={key:summarize_values([x['cpu_resources'][key] for x in group if x['cpu_resources'].get(key) is not None]) for key in ['user_cpu_s','system_cpu_s','total_cpu_s','cpu_percent','voluntary_context_switches','involuntary_context_switches']}
   entry['diagnostics']={key:summarize_values([x['diagnostics'][key] for x in group if key in x['diagnostics']]) for key in ['fork_snapshots','snapshot_base_capacity_bytes_cumulative','root_streams','dense_scratch_bytes_reported']}
  for row in summary:
   p1=next((x['median_wall_s'] for x in summary if x['case']==row['case'] and x['solver']==row['solver'] and x['workers']==1),None);row['same_binary_self_speedup']=p1/row['median_wall_s'] if p1 and row['median_wall_s'] else None
   row['self_speedup_scope']='P1 absent in this registered single-cell confirmation;not measured' if p1 is None else 'same-binary measured P1'
  save(args.out/'summary.json',summary)
 completion={'status':'passed' if all(x['status']=='completed' for x in rows) else 'incomplete','runs':len(rows),'wall_s':time.monotonic()-begin,'validation_only':args.validation_only,'binary_sha256_by_solver_id':{s['id']:s['binary_sha256'] for s in solvers},'solver_kinds':{s['id']:s['kind'] for s in solvers}}
 # A kind is not an identity: two candidates or O3/Ofast author variants may
 # share it. Keep the old mapping only when every kind is unambiguous.
 if len({s['kind'] for s in solvers})==len(solvers):completion['binary_sha256']={s['kind']:s['binary_sha256'] for s in solvers}
 save(args.out/'completion.json',completion)
if __name__=='__main__':main()
