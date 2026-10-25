#!/usr/bin/env python3
"""P005 correctness gates. Run directly on the Ubuntu development/test host.

No SSH, graph-library dependencies, or solver reimplementation: the small-graph
oracle checks the definition by enumerating all subsets. Timing logs here are
validation evidence, never performance measurements.
"""
import argparse, array, datetime, hashlib, itertools, json, os, pathlib, random
import re, resource, shutil, signal, struct, subprocess, tarfile, time

PROJECT=pathlib.Path('/home/joey/k-plex-rethinki-ubuntu')
REPO=pathlib.Path(__file__).resolve().parents[1]
CANON=PROJECT/'relaxclq'
BUILD=PROJECT/'build/p005'
REFSRC=PROJECT/'src/p005-reference'
ARCHIVE_SHA='ae76855e71a296f233fab5322bcf9b6ec07fca07b35e619b895cf0e386f7e104'
CPP=['maximal_kplex/toBin.cpp','maximal_clique/util.cpp','maximal_clique/set_operation.cpp','maximal_clique/bitpack_maximal_clique.cpp','maximum_clique/GraphClique.cpp','maximum_kplex/Graph.cpp','main.cpp','maximal_kplex/listPlex.cpp']
FLAGS=['-std=c++14','-O3','-DNDEBUG','-march=native','-msse4.2','-fopenmp','-DOPENMP','-DTARGET']

def sha(p):
 h=hashlib.sha256()
 with pathlib.Path(p).open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def dump(p,obj):
 p=pathlib.Path(p);p.parent.mkdir(parents=True,exist_ok=True)
 tmp=p.with_suffix(p.suffix+'.tmp');tmp.write_text(json.dumps(obj,indent=2)+'\n');tmp.replace(p)
def stamp():return datetime.datetime.now(datetime.timezone.utc).isoformat()
def command(cmd,cwd,log):
 begin=time.monotonic();p=subprocess.run(cmd,cwd=cwd,capture_output=True,text=True)
 pathlib.Path(log).write_text(p.stdout+p.stderr)
 dump(str(log)+'.json',{'command':list(map(str,cmd)),'cwd':str(cwd),'exit_code':p.returncode,'wall_s':time.monotonic()-begin,'completed_utc':stamp()})
 if p.returncode:raise RuntimeError('Build failed: '+str(log)+'\n'+p.stderr[-3000:])

def prepare_reference():
 manifest=json.loads((CANON/'tests/baselines/listplex-hardened-2026-10-25.json').read_text())
 archive=CANON/manifest['source_archive'];assert sha(archive)==ARCHIVE_SHA
 BUILD.mkdir(parents=True,exist_ok=True)
 command(['g++','-std=c++14','-O1','-g','-DTARGET','-msse4.2','-I.','-fsanitize=address,undefined','-fno-omit-frame-pointer','tests/storage_smoke.cpp','-o',str(BUILD/'storage')],CANON,BUILD/'storage.build.log')
 if not REFSRC.exists():
  REFSRC.mkdir(parents=True)
  with tarfile.open(archive) as t:
   for member in t.getmembers():
    assert (REFSRC/member.name).resolve().is_relative_to(REFSRC.resolve())
    assert not member.issym() and not member.islnk()
   t.extractall(REFSRC)
 for path,digest in manifest['algorithm_source_sha256'].items():
  assert sha(REFSRC/path)==digest,(path,sha(REFSRC/path),digest)
 command(['g++',*FLAGS,*CPP,'-o',str(BUILD/'reference')],REFSRC,BUILD/'reference.build.log')
 trace=PROJECT/'src/p005-reference-emit'
 if trace.exists():raise RuntimeError('Reference emit source already exists; preserve evidence and choose fresh run')
 shutil.copytree(REFSRC,trace)
 p=trace/'maximal_kplex/listPlex.h';s=p.read_text();assert s.count('emitor->emitPlex();')==2
 p.write_text(s.replace('emitor->emitPlex();','emitor->emitPlex(plex->members, plex->sz);'))
 p=trace/'maximal_kplex/listPlex.cpp';s=p.read_text()
 assert s.count('    struct PlexEmitor{')==1
 s=s.replace('    struct PlexEmitor{','    std::vector<int> originalIDs;\n    struct PlexEmitor{\n        const std::vector<int>& block;\n        PlexEmitor(const std::vector<int>& b):block(b){}')
 s=s.replace('void emitPlex(){','void emitPlex(const int* vertices,int size){\n            std::vector<int> ids;for(int i=0;i<size;++i)ids.push_back(originalIDs[block[vertices[i]]]);\n            std::sort(ids.begin(),ids.end());std::lock_guard<std::mutex> lock(coutMutex);std::cout<<"PLEX";for(int v:ids)std::cout<<" "<<v;std::cout<<"\\n";')
 s=s.replace('_seq<int> leadList = sequence::pack(map, mark, n);','_seq<int> leadList = sequence::pack(map, mark, n);\n        originalIDs.assign(leadList.A,leadList.A+leadList.n);').replace('PlexEmitor emitor;','PlexEmitor emitor(blk);')
 p.write_text(s)
 driver=BUILD/'reference_driver.cpp';driver.write_text('#include "maximal_kplex/listPlex.h"\nint main(int argc,char**argv){return ListPlex::main_maximal_kplex(argv[1],atoi(argv[2]),atoi(argv[3]),0,86400);}\n')
 command(['g++',*FLAGS,'-I.','maximal_kplex/toBin.cpp','maximal_kplex/listPlex.cpp',str(driver),'-o',str(BUILD/'reference-emit')],trace,BUILD/'reference-emit.build.log')
 dump(BUILD/'reference-identity.json',{'source_archive':str(archive),'source_archive_sha256':sha(archive),'reference_binary_sha256':sha(BUILD/'reference'),'trace_binary_sha256':sha(BUILD/'reference-emit'),'source_manifest':manifest,'instrumentation':'Emission only, original IDs via core packed list/block; copied source, immutable reference source unchanged.'})


def prepare_safe_reference():
 import difflib
 BUILD.mkdir(parents=True,exist_ok=True)
 safe=PROJECT/'src/p005-reference-safe';emit=PROJECT/'src/p005-reference-safe-emit'
 if safe.exists() or emit.exists():raise RuntimeError('Safe reference source already exists; preserve evidence')
 shutil.copytree(REFSRC,safe);shutil.copytree(PROJECT/'src/p005-reference-emit',emit)
 replacement=(REPO/'maximal_kplex/sequence.h').read_bytes()
 original=(REFSRC/'maximal_kplex/sequence.h').read_text()
 patch=''.join(difflib.unified_diff(original.splitlines(True),replacement.decode().splitlines(True),fromfile='frozen/maximal_kplex/sequence.h',tofile='safe/maximal_kplex/sequence.h'))
 (BUILD/'reference-safe.patch').write_text(patch)
 for folder in (safe,emit):(folder/'maximal_kplex/sequence.h').write_bytes(replacement)
 hashes={}
 for path in safe.rglob('*'):
  if path.is_file():
   rel=path.relative_to(safe);hashes[str(rel)]=sha(path)
   if str(rel)!='maximal_kplex/sequence.h':assert sha(path)==sha(REFSRC/rel)
 command(['g++',*FLAGS,*CPP,'-o',str(BUILD/'reference-safe')],safe,BUILD/'reference-safe.build.log')
 command(['g++',*FLAGS,'-I.','maximal_kplex/toBin.cpp','maximal_kplex/listPlex.cpp',str(BUILD/'reference_driver.cpp'),'-o',str(BUILD/'reference-safe-emit')],emit,BUILD/'reference-safe-emit.build.log')
 dump(BUILD/'reference-safe-identity.json',{'source_archive_sha256':ARCHIVE_SHA,'source_sha256':hashes,'only_source_overlay':'maximal_kplex/sequence.h','overlay_sha256':sha(safe/'maximal_kplex/sequence.h'),'patch':str(BUILD/'reference-safe.patch'),'reference_safe_sha256':sha(BUILD/'reference-safe'),'reference_safe_emit_sha256':sha(BUILD/'reference-safe-emit'),'purpose':'Separate helper-safe serial/legacy control. Frozen archive/control remain unchanged.'})


def write_graph(path,n,edges=None):
 path.parent.mkdir(parents=True,exist_ok=True)
 if edges is None:
  with path.open('wb') as f:
   f.write(struct.pack('<III',4,n,n*(n-1)));array.array('I',[n-1]*n).tofile(f)
   for u in range(n):array.array('I',itertools.chain(range(u),range(u+1,n))).tofile(f)
  return
 adj=[[] for _ in range(n)]
 for u,v in edges:adj[u].append(v);adj[v].append(u)
 with path.open('wb') as f:
  f.write(struct.pack('<III',4,n,2*len(edges)));array.array('I',map(len,adj)).tofile(f)
  for a in adj:array.array('I',sorted(a)).tofile(f)


def oracle(n,edges,k,q):
 """Hereditary subset DP followed by the literal one-vertex maximality test."""
 adj=[0]*n
 for u,v in edges:adj[u]|=1<<v;adj[v]|=1<<u
 valid=bytearray(1<<n);valid[0]=1;result=set();full=(1<<n)-1
 for mask in range(1,1<<n):
  bit=mask&-mask;previous=mask^bit
  if not valid[previous]:continue
  size=mask.bit_count();u=bit.bit_length()-1
  if (adj[u]&mask).bit_count()<size-k:continue
  rest=previous;good=True
  while rest:
   b=rest&-rest;rest^=b
   if (adj[b.bit_length()-1]&mask).bit_count()<size-k:good=False;break
  valid[mask]=int(good)
 for mask in range(1<<n):
  if mask.bit_count()<q or not valid[mask]:continue
  rest=full^mask;maximal=True
  while rest:
   b=rest&-rest;rest^=b
   if valid[mask|b]:maximal=False;break
  if maximal:result.add(tuple(i for i in range(n) if mask>>i&1))
 return result


def cases():
 pairs=list(itertools.combinations(range(5),2))
 for mask in range(1024):
  edges=[e for i,e in enumerate(pairs) if mask>>i&1]
  for q in (3,4):yield (f'exhaustive5-{mask}-q{q}',5,edges,2,q)
 rng=random.Random(20261025)
 for n in (6,9,12,15):
  for density in (.2,.5,.8):
   for seed in range(2):
    edges=[(u,v) for u in range(n) for v in range(u+1,n) if rng.random()<density]
    for k in (2,3,4,5,6):
     for q in (2*k-1,2*k):
      if q<=n:yield (f'random-{n}-{density}-{seed}-k{k}-q{q}',n,edges,k,q)


class Runner:
 def __init__(self,args):
  self.args=args;self.out=pathlib.Path(args.output);self.out.mkdir(parents=True,exist_ok=True)
  self.rows=[];self.started=stamp();self.count=0;self.reference_cache={}
  if args.reuse_reference_from:
   old=pathlib.Path(args.reuse_reference_from)
   for index,line in enumerate((old/'checks.jsonl').read_text().splitlines(),1):
    row=json.loads(line)
    if pathlib.Path(row['binary']).name.startswith('reference'):
     matches=list(old.glob(f'{index:05d}-*'));assert len(matches)==1
     self.reference_cache[row['label']]=(row,matches[0])
  self.affinity=sorted(os.sched_getaffinity(0))[:20]
  self.candidate=pathlib.Path(args.candidate);self.emit=pathlib.Path(args.emit)
  self.source_hashes={str(p.relative_to(REPO)):sha(p) for p in REPO.rglob('*') if p.is_file() and (p.suffix in ('.h','.hpp','.cpp','.c') or p.name=='CMakeLists.txt') and '.git' not in p.parts}
  dump(self.out/'source-sha256.json',self.source_hashes)
  (self.out/'working-tree.patch').write_text(subprocess.check_output(['git','diff','HEAD'],cwd=REPO,text=True))
  self.identities={str(p):sha(p) for p in (BUILD/'reference',BUILD/'reference-emit',self.candidate,self.emit,BUILD/'reference-safe',BUILD/'reference-safe-emit') if p.exists()}
  dump(self.out/'environment.json',{'started_utc':self.started,'host':os.uname().nodename,'affinity':self.affinity,'load':os.getloadavg(),'meminfo':pathlib.Path('/proc/meminfo').read_text(),'compiler':subprocess.check_output(['g++','--version'],text=True),'git_head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=REPO,text=True).strip(),'git_status':subprocess.check_output(['git','status','--porcelain'],cwd=REPO,text=True),'binary_sha256':self.identities,'reference_archive_sha256':ARCHIVE_SHA,'execution':'Ubuntu direct, no SSH/SCP','arguments':vars(args),'validation_timings_only':True})
  self.status('running')
 def status(self,status,error=None):dump(self.out/'status.json',{'stage':self.args.stage,'status':status,'checks':self.count,'started_utc':self.started,'updated_utc':stamp(),'error':error})
 def run(self,label,binary,path=None,k=None,q=None,p=1,expected=None,sets=None,timeout=10,args=None,driver=False,rc=0,sanitizer=False,worker_check=True):
  self.count+=1;d=self.out/f'{self.count:05d}-{label}-p{p}';d.mkdir()
  if self.args.stage=='exhaustive' and label in self.reference_cache:
   old,prior=self.reference_cache[label]
   trace=(prior/'stdout.txt').read_text()
   emitted=[tuple(map(int,l.split()[1:])) for l in trace.splitlines() if l.startswith('PLEX ')]
   assert old['passed'] and old['binary_sha256']==sha(binary) and old['input_sha256']==sha(path)
   assert old['expected_count']==expected and set(emitted)==sets and len(emitted)==len(sets)
   for filename in ('stdout.txt','stderr.txt','resources.json'):
    if (prior/filename).exists():shutil.copy2(prior/filename,d/filename)
   row=dict(old,input=str(path),reused_reference=True,reused_from=str(prior),completed_utc=stamp())
   dump(d/'run.json',row)
   with (self.out/'checks.jsonl').open('a') as f:f.write(json.dumps(row)+'\n')
   self.rows.append(row);self.status('running');return row
  argv=[str(binary),*(list(map(str,args)) if args is not None else ([str(path),str(k),str(q)] if driver else ['-f',str(path),'-m','maximal','-k',str(k),'-l',str(q),'-t','86400']))]
  affinity=self.affinity[:max(1,p)]
  cmd=['/usr/bin/time','-f','{"wall_s":%e,"peak_rss_kib":%M}','-o',str(d/'resources.json'),'taskset','-c',','.join(map(str,affinity)),*argv]
  env=dict(os.environ,PARLAY_NUM_THREADS=str(p),OMP_NUM_THREADS=str(p),OMP_DYNAMIC='FALSE',OMP_MAX_ACTIVE_LEVELS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
  def limits():
   os.setsid();resource.setrlimit(resource.RLIMIT_CORE,(0,0))
   if not sanitizer:resource.setrlimit(resource.RLIMIT_AS,(8*1024**3,8*1024**3))
  begin=time.monotonic();state='completed'
  with (d/'stdout.txt').open('w') as so,(d/'stderr.txt').open('w') as se:
   proc=subprocess.Popen(cmd,stdout=so,stderr=se,env=env,preexec_fn=limits)
   try:proc.wait(timeout=timeout)
   except subprocess.TimeoutExpired:state='timeout';os.killpg(proc.pid,signal.SIGKILL);proc.wait()
  wall=time.monotonic()-begin;stdout=(d/'stdout.txt').read_text();stderr=(d/'stderr.txt').read_text()
  counts=re.findall(r'Number of (?:\d+-plex|plexes)\s*:\s*(\d+)',stdout);count=int(counts[-1]) if counts else None
  emitted=[tuple(map(int,l.split()[1:])) for l in stdout.splitlines() if l.startswith(('PLEX ','SOLUTION '))]
  actual=set(emitted);duplicate_count=len(emitted)-len(actual)
  observed=re.findall(r'(?:Parlay workers|PARLAY_WORKERS|parlay_workers|P005 Parlay workers)\s*[:=]\s*(\d+)',stdout+'\n'+stderr)
  candidate='reference' not in pathlib.Path(binary).name
  passed=state=='completed' and proc.returncode==rc
  if expected is not None:passed=passed and count==expected
  if sets is not None:passed=passed and actual==sets and len(emitted)==len(sets) and duplicate_count==0
  if candidate and worker_check and rc==0:passed=passed and bool(observed) and int(observed[-1])==p
  row={'label':label,'binary':str(binary),'binary_sha256':self.identities.get(str(binary),sha(binary)),'command':cmd,'workers_requested':p,'workers_observed':int(observed[-1]) if observed else None,'affinity':affinity,'input':str(path) if path else None,'input_sha256':sha(path) if path and pathlib.Path(path).exists() else None,'k':k,'q':q,'status':state,'exit_code':proc.returncode,'expected_exit':rc,'count':count,'expected_count':expected,'emitted':len(emitted),'duplicates':duplicate_count,'passed':bool(passed),'wall_s_validation_only':wall,'completed_utc':stamp()}
  if (d/'resources.json').exists():
   try:row.update(json.loads((d/'resources.json').read_text().splitlines()[-1]))
   except (ValueError,IndexError):pass
  if sets is not None:
   row.update(missing=sorted(sets-actual),extra=sorted(actual-sets),expected_sets_sha256=hashlib.sha256(json.dumps(sorted(sets)).encode()).hexdigest())
  dump(d/'run.json',row)
  with (self.out/'checks.jsonl').open('a') as f:f.write(json.dumps(row)+'\n')
  self.rows.append(row);self.status('running')
  if not passed:raise RuntimeError('Validation failed '+str(d)+' '+json.dumps(row))
  return row
 def fixture(self,name,n,edges=None):
  path=self.out/'fixtures'/(name+'.bin')
  if not path.exists():write_graph(path,n,edges)
  return path
 def variants(self,trace=False):
  variants=[('reference',BUILD/('reference-emit' if trace else 'reference'),1,trace),('candidate',self.emit if trace else self.candidate,1,False),('candidate',self.emit if trace else self.candidate,4,False)]
  if self.args.stage=='exhaustive' and self.args.safe_reference:variants.insert(1,('reference-safe',BUILD/'reference-safe-emit',1,True))
  return variants
 def smoke(self):
  suite=json.loads((REPO/'tests/test-suites.json').read_text())['suites']['dev_smoke']
  for case in suite['cases']:
   exact=case.get('comparison')=='exact_sets';expected_sets=set(map(tuple,case['expected_sets'])) if exact else None
   if 'fixture' in case:
    f=case['fixture'];path=self.fixture(case['id'],f['n'],None if f['kind']=='complete' else f['edges'])
   else:path=PROJECT/'datasets/enumeration-2026-10-25'/(case['dataset']+'.bin')
   expected=case.get('expected_count',len(expected_sets) if exact else None)
   for name,binary,p,driver in self.variants(exact):self.run(case['id']+'-'+name,binary,path,case['k'],case['q'],p,expected,expected_sets,driver=driver)
  self.run('storage',BUILD/'storage',args=[],worker_check=False,sanitizer=True)
  assert self.count==19
 def exhaustive(self):
  allcases=list(cases());assert len(allcases)==2234
  for i,(label,n,edges,k,q) in enumerate(allcases):
   path=self.fixture(label,n,edges);expected=oracle(n,edges,k,q)
   dump(path.with_suffix('.json'),{'n':n,'edges':edges,'k':k,'q':q,'expected_sets':sorted(expected),'input_sha256':sha(path)})
   for name,binary,p,driver in self.variants(True):self.run(label+'-'+name,binary,path,k,q,p,len(expected),expected,driver=driver)
   if i%100==0:print('ORACLE',i+1,'/2234 checks',self.count,flush=True)
 def boundaries(self):
  for n in (0,1,5,512,520,4100):
   path=self.fixture('K'+str(n),n);k=3 if n==5 else 2;q=5 if n==5 else max(3,n)
   for name,binary,p,driver in self.variants():self.run('K'+str(n)+'-'+name,binary,path,k,q,p,int(n>=q),timeout=60)
  cli=[(['-h'],0),(['-f','x.bin','-m','maximal','-k','0'],2),(['-f','x.bin','-m','maximal','-k','3x'],2),(['-f','x.bin','-m','maximal','-k','3','-l','4'],2),(['-f','x.bin','-m','maximal','-t','nan'],2),(['-f','x.bin','-m','maximal','-t','-1'],2),(['-f','x','-m','maximal'],2),(['-f','x.bin','-m','garbage'],2),(['-f','x.bin','-m','maximal','-k','999999999999999999999'],2),(['-f','x.bin','-m','maximal','-l','-1'],2)]
  for i,(args,rc) in enumerate(cli):
   for binary in (BUILD/'reference',self.candidate):self.run('cli'+str(i)+'-'+binary.name,binary,args=args,rc=rc,worker_check=False)
 def stress(self):
  randomcases=[c for c in cases() if c[0].startswith('random-')]
  selection=[randomcases[round(i*(len(randomcases)-1)/19)] for i in range(20)]
  dump(self.out/'selection.json',[{'label':x[0],'n':x[1],'edges':x[2],'k':x[3],'q':x[4]} for x in selection])
  for p in (2,8,16,20):
   for rep in range(3):
    for label,n,edges,k,q in selection:
     path=self.fixture(label,n,edges);expected=oracle(n,edges,k,q)
     self.run(label+f'-repeat{rep+1}',self.emit,path,k,q,p,len(expected),expected)
 def sanitizers(self):
  include=str(PROJECT/'deps/parlaylib/include')
  flags=['-std=c++17','-pthread','-DPARLAY_ELASTIC_PARALLELISM=0','-O1','-g','-DTARGET','-msse4.2','-I.','-I'+include,'-fsanitize=address,undefined','-fno-omit-frame-pointer']
  command(['g++',*flags,'-DLISTPLEX_EMIT_SOLUTIONS',*CPP,'-o',str(BUILD/'candidate-asan')],REPO,BUILD/'candidate-asan.build.log')
  command(['g++',*flags,'maximal_kplex/toBin.cpp','maximal_kplex/listPlex.cpp','tests/p005_ownership_driver.cpp','-o',str(BUILD/'candidate-ownership-asan')],REPO,BUILD/'candidate-ownership-asan.build.log')
  self.identities[str(BUILD/'candidate-asan')]=sha(BUILD/'candidate-asan')
  self.identities[str(BUILD/'candidate-ownership-asan')]=sha(BUILD/'candidate-ownership-asan')
  command(['g++',*flags,'tests/p005_flags_smoke.cpp','-o',str(BUILD/'flags-asan')],REPO,BUILD/'flags-asan.build.log')
  self.run('packed-flags-asan',BUILD/'flags-asan',args=[],sanitizer=True,worker_check=False)
  self.run('storage-asan',BUILD/'storage',args=[],sanitizer=True,worker_check=False)
  for n in (5,520,4100):
   path=self.fixture('K'+str(n),n);k=3 if n==5 else 2;q=5 if n==5 else n
   self.run('K'+str(n)+'-asan',BUILD/'candidate-asan',path,k,q,4,1,timeout=120,sanitizer=True)
  randomcases=[c for c in cases() if c[0].startswith('random-')]
  for index in (0,10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170,180,185):
   label,n,edges,k,q=randomcases[index];path=self.fixture(label,n,edges);expected=oracle(n,edges,k,q)
   self.run(label+'-asan',BUILD/'candidate-asan',path,k,q,4,len(expected),expected,sanitizer=True,timeout=30)
  txt=self.out/'fixtures/K8.txt';txt.write_text(''.join(f'{u} {v}\n' for u,v in itertools.combinations(range(8),2)))
  driver=BUILD/'candidate-ownership-asan'
  self.run('repeated-conversion-writer',driver,args=['ownership',txt,self.out/'fixtures/writer.bin'],sanitizer=True,worker_check=False,timeout=60)
  self.run('text-enumeration',driver,txt,3,5,4,1,driver=True,sanitizer=True,timeout=60)
  self.run('convert-enumeration',driver,args=[txt,3,5,1],p=4,expected=1,sanitizer=True,timeout=60)
  clq=self.out/'fixtures/K8.clq';clq.write_text('p edge 8 28\n'+''.join(f'e {u+1} {v+1}\n' for u,v in itertools.combinations(range(8),2)))
  self.run('dimacs-conversion',driver,args=[clq,3,5,1],p=4,expected=1,sanitizer=True,timeout=60)
  long=self.out/'fixtures'
  for i in range(12):long=long/('directory'+str(i)+'x'*85)
  long.mkdir(parents=True,exist_ok=True);long=long/'K8.txt';long.write_bytes(txt.read_bytes());assert len(str(long))>1024
  self.run('long-path',driver,args=[long,3,5,1],p=4,expected=1,sanitizer=True,timeout=60)
  # TSan is a separate runtime compatibility attempt, never an ASan substitute.
  tsan_flags=[x for x in flags if x!='-fsanitize=address,undefined']+['-fsanitize=thread']
  command(['g++',*tsan_flags,*CPP,'-o',str(BUILD/'candidate-tsan')],REPO,BUILD/'candidate-tsan.build.log')
  path=self.fixture('tsan-counterexample',6,[(0,3),(1,2),(1,5),(2,4)])
  env=dict(os.environ,PARLAY_NUM_THREADS='4',TSAN_OPTIONS='halt_on_error=1')
  z=subprocess.run([str(BUILD/'candidate-tsan'),'-f',str(path),'-m','maximal','-k','2','-l','3'],capture_output=True,text=True,env=env,timeout=30)
  (self.out/'tsan.stdout.txt').write_text(z.stdout);(self.out/'tsan.stderr.txt').write_text(z.stderr)
  if z.returncode==0:tsan={'status':'compatible-smoke-pass','exit_code':0,'scope':'one four-worker counterexample only'}
  elif 'unexpected memory mapping' in z.stderr:tsan={'status':'unsupported-environment','exit_code':z.returncode,'limitation':'ThreadSanitizer aborts before solver initialization: unexpected memory mapping; no race-freedom claim. Repeated exact-set stress remains required.'}
  else:raise RuntimeError('TSan failure requires investigation: '+z.stderr[-4000:])
  dump(self.out/'tsan-status.json',tsan)

 def sanity(self):
  suite=json.loads((REPO/'tests/test-suites.json').read_text())['suites']['remote_sanity']
  for case in suite['cases']:
   data=case['dataset'];data='wiki-Vote' if data=='wiki-vote' else data
   path=PROJECT/'datasets/enumeration-2026-10-25'/(data+'.bin')
   self.run(data+'-reference-safe',BUILD/'reference-safe',path,case['k'],case['q'],1,case['expected_count'],timeout=60)
   for p in (1,4,8,20):self.run(data+'-candidate',self.candidate,path,case['k'],case['q'],p,case['expected_count'],timeout=60)


def main():
 a=argparse.ArgumentParser();a.add_argument('--safe-reference',action='store_true');a.add_argument('--prepare-safe-reference',action='store_true');a.add_argument('--reuse-reference-from');a.add_argument('--prepare-reference',action='store_true');a.add_argument('--stage',choices=['smoke','exhaustive','boundaries','sanitizers','stress','sanity']);a.add_argument('--candidate',default=str(BUILD/'MKP'));a.add_argument('--emit',default=str(BUILD/'MKP-emit'));a.add_argument('--output',default=str(PROJECT/'runs/p005-validation'))
 args=a.parse_args()
 if os.uname().nodename!='joey-virtual-machine' or not PROJECT.is_dir():raise RuntimeError('Wrong execution host')
 if args.prepare_reference:prepare_reference()
 if args.prepare_safe_reference:prepare_safe_reference()
 if args.stage:
  r=Runner(args)
  try:getattr(r,args.stage)();r.status('complete');print('VALIDATION PASS',args.stage,r.count,flush=True)
  except Exception as e:r.status('failed',str(e));raise
if __name__=='__main__':main()
