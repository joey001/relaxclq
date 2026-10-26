#!/usr/bin/env python3
"""Candidate-local, explicit-path P010 ownership/admission checks.

Uses the canonical independent oracle/runner; never rebuilds a control and never
imports a historical candidate validator. Timing fields are validation only.
"""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path
import re
import subprocess
import sys


def module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def star_complement(k, r):
    n = (k + 1) * r
    missing = {tuple(sorted((base, base + j)))
               for base in range(0, n, k + 1) for j in range(1, k + 1)}
    edges = [e for e in itertools.combinations(range(n), 2) if e not in missing]
    # Each complement star contributes exactly k vertices: omit its center or
    # any one leaf. Larger subsets violate the k-plex degree requirement;
    # smaller subsets are extendible inside the same star.
    expected = {tuple(v for v in range(n) if v not in omitted)
                for omitted in itertools.product(*(range(b, b + k + 1)
                  for b in range(0, n, k + 1)))}
    return n, edges, expected


def diagnostic(runner, binary, row):
    path = runner.out / f'{runner.count:05d}-{row["label"]}-p{row["workers_requested"]}' / 'stdout.txt'
    output = path.read_text()
    def value(label):
        match = re.search(r'^' + re.escape(label) + r': (\d+)$', output, re.M)
        if not match:
            raise ValueError('Missing P010 diagnostic: ' + label)
        return int(match[1])
    result = {key: value(label) for key, label in {
        'attempts': 'P010 funded attempts', 'grants': 'Fork snapshots',
        'denied': 'P010 denied attempts', 'renewals': 'P010 after-join renewal grants',
        'alias': 'P010 exclusion-alias grants', 'reserved': 'P010 final reserved',
        'metered': 'P010 metered iterations (raw cumulative)',
        'funded': 'P010 funded threshold words (raw cumulative)',
        **{f'site{i}': f'P010 site {i} grants' for i in range(3)}}.items()}
    assert 'P010 word totals: valid' in output
    assert result['attempts'] == result['grants'] + result['denied']
    assert sum(result[f'site{i}'] for i in range(3)) == result['grants']
    assert result['alias'] <= result['grants'] and result['renewals'] <= result['grants']
    assert result['reserved'] == 0 and result['funded'] <= result['metered']
    assert row['workers_observed'] == row['workers_requested']
    result.update(label=row['label'], workers=row['workers_requested'], binary_sha256=row['binary_sha256'])
    with (runner.out / 'diagnostics.jsonl').open('a') as f:
        f.write(json.dumps(result) + '\n')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', type=Path, required=True)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--stage', choices=['sanitize', 'diagnostic'], required=True)
    args = parser.parse_args()
    project, repo, build = (x.resolve() for x in (args.project, args.repo, args.build))
    val = module(project / 'relaxclq/tests/validation_support.py', 'validation_support')
    lock = module(project / 'tools/experiments/candidate_validation.py', 'candidate_validation')
    val.configure(project, repo)
    binary = build / ('candidate-asan-emit' if args.stage == 'sanitize' else 'MKP-diagnostic')
    options = argparse.Namespace(repo=str(repo), candidate=str(binary), emit=str(binary),
        output=str(args.output.resolve()), stage=args.stage, reuse_reference_from=None)
    with lock.validation_lock(project / 'runs/parallel-exploration-performance.lock'):
        if args.stage == 'sanitize':
            if build.exists():
                raise ValueError('Sanitizer build directory already exists')
            build.mkdir(parents=True)
            cmd = ['g++', '-std=c++17', '-pthread', '-DPARLAY_ELASTIC_PARALLELISM=0',
                '-O1', '-g', '-DTARGET', '-msse4.2', '-DLISTPLEX_EMIT_SOLUTIONS',
                '-DP010_DIAGNOSTICS', '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                '-I' + str(repo), '-I' + str(project / 'deps/parlaylib/include'),
                str(repo / 'maximal_kplex/toBin.cpp'), str(repo / 'maximal_kplex/listPlex.cpp'),
                str(repo / 'tests/p005_ownership_driver.cpp'), '-o', str(binary)]
            val.dump(build / 'build-command.json', cmd)
            with (build / 'build.log').open('w') as f:
                subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, check=True)
        runner = val.Runner(options)
        totals = {key: 0 for key in ('attempts','grants','denied','renewals','alias','site0','site1','site2')}
        try:
            if args.stage == 'sanitize':
                for n in (5,520,4100):
                    path = runner.fixture('K'+str(n), n)
                    row = runner.run('K'+str(n), binary, path, 3 if n==5 else 2,
                        5 if n==5 else n, 4, 1, timeout=120, sanitizer=True, driver=True)
                    diagnostic(runner, binary, row)
            randomcases = [c for c in val.cases() if c[0].startswith('random-')]
            selection = [randomcases[round(i*(len(randomcases)-1)/19)] for i in range(20)]
            for label,n,edges,k,q in selection:
                path = runner.fixture(label,n,edges)
                expected = val.oracle(n,edges,k,q)
                row = runner.run(label,binary,path,k,q,4,len(expected),
                    expected if args.stage=='sanitize' else None,timeout=30,
                    sanitizer=args.stage=='sanitize',driver=args.stage=='sanitize')
                data = diagnostic(runner,binary,row)
                for key in totals:totals[key] += data[key]
            for k,r in ((2,4),(3,4)):
                n,edges,expected = star_complement(k,r)
                # Independently cross-check the closed form with subset DP.
                assert expected == val.oracle(n,edges,k,k*r)
                path=runner.fixture(f'complement-stars-k{k}-r{r}',n,edges)
                for p in (2,8,16,20):
                    for rep in range(3):
                        row=runner.run(f'stars-k{k}-r{r}-rep{rep}',binary,path,k,k*r,p,len(expected),
                            expected if args.stage=='sanitize' else None, timeout=30,
                            sanitizer=args.stage=='sanitize',driver=args.stage=='sanitize')
                        data=diagnostic(runner,binary,row)
                        for key in totals:totals[key] += data[key]
            if args.stage == 'diagnostic':
                for label,n,edges,k,q in randomcases:
                    if all(totals[key] for key in ('site0','site1','site2','alias','renewals')):
                        break
                    if any(label == item[0] for item in selection):
                        continue
                    path=runner.fixture(label,n,edges)
                    expected=val.oracle(n,edges,k,q)
                    row=runner.run(label,binary,path,k,q,4,len(expected),timeout=30)
                    data=diagnostic(runner,binary,row)
                    for key in totals:totals[key] += data[key]
            val.dump(runner.out/'coverage-summary.json',totals)
            runner.status('complete')
            print('P010 PASS',args.stage,runner.count,totals,flush=True)
        except Exception as exc:
            runner.status('failed',str(exc))
            raise

if __name__=='__main__':main()
