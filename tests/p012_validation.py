#!/usr/bin/env python3
"""Explicit-path P012 actual-predicate and ownership validation, never timing."""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path
import re
import subprocess

def module(path,name):
    spec=importlib.util.spec_from_file_location(name,path)
    result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result

def stars(k,r):
    n=(k+1)*r
    missing={tuple(sorted((b,b+j))) for b in range(0,n,k+1) for j in range(1,k+1)}
    edges=[e for e in itertools.combinations(range(n),2) if e not in missing]
    expected={tuple(v for v in range(n) if v not in omitted)
              for omitted in itertools.product(*(range(b,b+k+1) for b in range(0,n,k+1)))}
    return n,edges,expected

KEYS='queries cheap cached scanned original pushes rejectNew rejectCached extensions passes pgPasses forks nonemptyParentForks'.split()
REQUIRED='queries cheap cached scanned pushes rejectNew rejectCached extensions pgPasses forks nonemptyParentForks'.split()

def diagnostic(runner,row):
    out=runner.out/f'{runner.count:05d}-{row["label"]}-p{row["workers_requested"]}'/'stdout.txt'
    output=out.read_text();assert 'P012 totals: valid' in output
    data={key:int(re.search(r'^P012 '+key+r': (\d+)$',output,re.M)[1]) for key in KEYS}
    assert data['cached']+data['scanned']<=data['original']
    assert row['workers_requested']==row['workers_observed']
    assert row.get('peak_rss_kib',0)<=8*1024**2
    with (runner.out/'diagnostics.jsonl').open('a') as f:f.write(json.dumps(dict(data,label=row['label'],workers=row['workers_requested']))+'\n')
    return data

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for key in ('project','repo','build','output'):parser.add_argument('--'+key,type=Path,required=True)
    parser.add_argument('--stage',choices=['diagnostic','sanitize'],required=True)
    args=parser.parse_args();project,repo,build=(getattr(args,x).resolve() for x in ('project','repo','build'))
    val=module(project/'relaxclq/tests/validation_support.py','validation_support')
    lock=module(project/'tools/experiments/candidate_validation.py','candidate_validation')
    val.configure(project,repo)
    binary=build/('candidate-asan-emit' if args.stage=='sanitize' else 'MKP-check-emit')
    options=argparse.Namespace(repo=str(repo),candidate=str(binary),emit=str(binary),output=str(args.output.resolve()),stage=args.stage,reuse_reference_from=None)
    with lock.validation_lock(project/'runs/parallel-exploration-performance.lock'):
        if args.stage=='sanitize':
            build.mkdir(parents=True,exist_ok=False)
            cmd=['g++','-std=c++17','-pthread','-DPARLAY_ELASTIC_PARALLELISM=0','-O1','-g','-DTARGET','-msse4.2',
                 '-DLISTPLEX_EMIT_SOLUTIONS','-DP012_DIAGNOSTICS','-fsanitize=address,undefined','-fno-omit-frame-pointer',
                 '-I'+str(repo),'-I'+str(project/'deps/parlaylib/include'),str(repo/'maximal_kplex/toBin.cpp'),
                 str(repo/'maximal_kplex/listPlex.cpp'),str(repo/'tests/p005_ownership_driver.cpp'),'-o',str(binary)]
            val.dump(build/'build-command.json',cmd)
            with (build/'build.log').open('w') as f:subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT,check=True)
        runner=val.Runner(options);totals={key:0 for key in KEYS}
        def run(label,n,edges,k,q,p,expected=None,timeout=30):
            expected=val.oracle(n,edges,k,q) if expected is None else expected
            path=runner.fixture(label,n,edges)
            row=runner.run(label,binary,path,k,q,p,len(expected),expected,timeout=timeout,sanitizer=args.stage=='sanitize',driver=True)
            for key,value in diagnostic(runner,row).items():totals[key]+=value
        try:
            if args.stage=='sanitize':
                for n in (5,520,4100):
                    path=runner.fixture('K'+str(n),n)
                    row=runner.run('K'+str(n),binary,path,3 if n==5 else 2,5 if n==5 else n,4,1,timeout=120,sanitizer=True,driver=True)
                    for key,value in diagnostic(runner,row).items():totals[key]+=value
            randomcases=[c for c in val.cases() if c[0].startswith('random-')]
            selection=[randomcases[round(i*(len(randomcases)-1)/19)] for i in range(20)]
            val.dump(runner.out/'fixed-selection.json',[x[0] for x in selection])
            for label,n,edges,k,q in selection:run(label,n,edges,k,q,4)
            for k,r in ((2,4),(3,4)):
                n,edges,expected=stars(k,r);assert expected==val.oracle(n,edges,k,k*r)
                for p in (2,8,16,20):
                    for rep in range(3):run(f'stars-k{k}-r{r}-rep{rep}',n,edges,k,k*r,p,expected)
            # Finite registered fixed-pool extension only if coverage is absent.
            extra=[]
            for label,n,edges,k,q in randomcases:
                if all(totals[key] for key in REQUIRED):break
                if any(label==x[0] for x in selection):continue
                run(label,n,edges,k,q,4);extra.append(label)
            assert all(totals[key] for key in REQUIRED),totals
            val.dump(runner.out/'coverage-summary.json',dict(totals=totals,extra_fixed_pool=extra,required=REQUIRED))
            runner.status('complete');print('P012 PASS',args.stage,runner.count,totals,flush=True)
        except Exception as exc:runner.status('failed',str(exc));raise

if __name__=='__main__':main()
