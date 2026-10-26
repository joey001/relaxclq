#!/usr/bin/env python3
"""P013 actual-dispatch/degree/witness checks using the canonical subset oracle.

No control build, timing experiment or copied technique metadata is created.
"""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path
import re

def module(path,name):
    spec=importlib.util.spec_from_file_location(name,path)
    result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result

def fixtures():
    for k,o in ((2,6),(3,5)):
        edges={(0,v) for v in range(1,7)}|{(v,v+1) for v in range(1,6)}
        edges|=set(itertools.combinations(range(7,7+o),2))
        edges|={(c,v) for c in range(1,7) for v in range(7,7+o)}
        witness=(0,1,2,3,7) if k==2 else (0,1,2,3,4,7,8)
        yield f'path-joined-clique-k{k}',7+o,sorted(edges),k,2*k+1,'witness',witness
    edges={(0,v) for v in range(1,6)}|{(1,2)}|{(a,b) for a in (1,2) for b in (3,4,5)}
    yield 'equality-k2',6,sorted(edges),2,5,'equality',(0,1,2,3,4)

KEYS='visits checks elisions equalities granted ungranted degreeChecks restores'.split()
def diagnostic(runner,row,direct=False):
    output=(runner.out/f'{runner.count:05d}-{row["label"]}-p{row["workers_requested"]}'/'stdout.txt').read_text()
    assert 'P013 totals: valid' in output
    data={key:int(re.search(r'^P013 '+key+r': (\d+)$',output,re.M)[1]) for key in KEYS}
    assert data['restores']==data['elisions'] and data['elisions']+data['equalities']<=data['checks']
    assert row['workers_observed']==row['workers_requested']
    assert row.get('peak_rss_kib',0)<=8*1024**2
    if direct:assert 'P013 direct repetitions: 3' in output
    with (runner.out/'diagnostics.jsonl').open('a') as f:f.write(json.dumps(dict(data,label=row['label'],workers=row['workers_requested'],direct=direct))+'\n')
    return data

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for key in ('project','repo','build','output'):parser.add_argument('--'+key,type=Path,required=True)
    parser.add_argument('--stage',choices=['diagnostic','sanitize'],required=True)
    args=parser.parse_args();project,repo,build=(getattr(args,x).resolve() for x in ('project','repo','build'))
    val=module(project/'relaxclq/tests/validation_support.py','validation_support')
    lock=module(project/'tools/experiments/candidate_validation.py','candidate_validation')
    val.configure(project,repo)
    sanitizer=args.stage=='sanitize'
    binary=build/('candidate-asan-emit' if sanitizer else 'MKP-check-emit')
    direct=build/('dispatch-asan' if sanitizer else 'dispatch-check')
    options=argparse.Namespace(repo=str(repo),candidate=str(binary),emit=str(direct),output=str(args.output.resolve()),stage=args.stage,reuse_reference_from=None)
    with lock.validation_lock(project/'runs/parallel-exploration-performance.lock'):
        runner=val.Runner(options);totals={key:0 for key in KEYS};targeted=[]
        def collect(row,is_direct=False):
            data=diagnostic(runner,row,is_direct)
            for key,value in data.items():totals[key]+=value
            return data
        try:
            randomcases=[c for c in val.cases() if c[0].startswith('random-')]
            selection=[randomcases[round(i*(len(randomcases)-1)/19)] for i in range(20)]
            val.dump(runner.out/'fixed-selection.json',[x[0] for x in selection])
            for label,n,edges,k,q in selection:
                expected=val.oracle(n,edges,k,q);path=runner.fixture(label,n,edges)
                collect(runner.run(label,binary,path,k,q,4,len(expected),expected,timeout=30,sanitizer=sanitizer,driver=True))
            for label,n,edges,k,q,mode,witness in fixtures():
                expected=val.oracle(n,edges,k,q);assert witness in expected
                root_expected={x for x in expected if 0 in x};assert root_expected
                degrees=[sum(v in edge for edge in edges) for v in range(n)]
                if mode=='witness':assert degrees[0]==6 and all(degrees[v]>6 for v in range(1,n))
                path=runner.fixture(label,n,edges)
                expected_path=runner.out/'fixtures'/(label+'.expected-root.txt')
                expected_path.write_text(''.join(' '.join(map(str,x))+'\n' for x in sorted(root_expected)))
                targeted.append({'label':label,'input_sha256':val.sha(path),'k':k,'q':q,'oracle_sets':sorted(expected),'root_oracle_sets':sorted(root_expected),'live_C2_witness':witness,'degrees':degrees})
                for p in (1,4):
                    data=collect(runner.run(label+'-full',binary,path,k,q,p,len(expected),expected,timeout=30,sanitizer=sanitizer,driver=True))
                    if mode=='witness':assert data['elisions']>0
                for p,credit in ((1,'exhausted'),(4,'exhausted'),(4,'grant')):
                    row=runner.run(label+'-direct-'+credit,direct,path,k,q,p,len(root_expected),root_expected,
                        timeout=30,sanitizer=sanitizer,args=[path,k,q,expected_path,mode,credit])
                    data=collect(row,True)
                    assert data['checks']>0 and data['degreeChecks']>0
                    assert data['equalities']>0 if mode=='equality' else data['elisions']>0
                    assert data['granted']>0 if p>1 and credit=='grant' else data['granted']==0
            if sanitizer:
                for n in (5,520,4100):
                    path=runner.fixture('K'+str(n),n)
                    collect(runner.run('K'+str(n),binary,path,3 if n==5 else 2,5 if n==5 else n,4,1,timeout=120,sanitizer=True,driver=True))
            assert all(totals[key]>0 for key in KEYS)
            val.dump(runner.out/'targeted-oracle.json',targeted)
            val.dump(runner.out/'coverage-summary.json',{'totals':totals,'fixed_random':20,'full_target_calls':6,'direct_target_calls':9,'direct_internal_repetitions':3,'additional_fixture_tuning':False})
            runner.status('complete');print('P013 PASS',args.stage,runner.count,totals,flush=True)
        except Exception as exc:runner.status('failed',str(exc));raise

if __name__=='__main__':main()
