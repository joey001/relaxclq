#!/usr/bin/env python3
"""Read-only audit of frozen adjacent-block runs. Never launches a solver."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import statistics


def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as f:
        for block in iter(lambda:f.read(1024*1024),b''):h.update(block)
    return h.hexdigest()


def audit(plan_path, directory):
    plan=json.loads(plan_path.read_text())
    driver=Path(plan['measurement_driver_frozen'])
    assert sha(driver)==plan['measurement_driver_sha256']
    spec=importlib.util.spec_from_file_location('audited_frozen_driver',driver)
    shared=importlib.util.module_from_spec(spec);spec.loader.exec_module(shared)
    queue=shared.make_queue(plan)
    metadata=json.loads((directory/'metadata.json').read_text())
    assert metadata['driver_sha256']==sha(driver) and metadata['plan_sha256']==sha(plan_path)
    assert not metadata['validation_only']
    rows=json.loads((directory/'measurements.json').read_text())
    assert len(rows)<=len(queue)
    for solver in plan['solvers']:
        assert sha(solver['binary'])==solver['binary_sha256']
        for key in ('correctness_gate','identity','source_identity','release_source_provenance'):
            if key+'_sha256' in solver:assert sha(solver[key])==solver[key+'_sha256']
        for rel,digest in solver.get('source_sha256',{}).items():
            assert sha(Path(solver['source_repository'])/rel)==digest
        if 'identity' in solver:
            identity=json.loads(Path(solver['identity']).read_text())
            entry=next(x for x in identity['entries'] if x['name']==solver['kind'])
            assert entry['binary_sha256']==solver['binary_sha256']
            assert sha(entry['archive'])==entry['archive_sha256']
            for rel,digest in entry['source_sha256'].items():assert sha(Path(entry['source'])/rel)==digest
    for case in plan['cases']:assert sha(case['input'])==case['input_sha256']
    for i,(row,(case,solver,p,rep)) in enumerate(zip(rows,queue)):
        assert (row['index'],row['case'],row['solver'],row['workers'],row['repetition'])==(i,case['id'],solver['id'],p,rep)
        assert row['block']==i//len(plan['solvers']) and row['block_position']==i%len(plan['solvers'])
        assert row['binary_sha256']==solver['binary_sha256'] and row['input_sha256']==case['input_sha256']
        assert row['status']=='completed' and row['count']==row['expected_count']==case['expected_count']
        assert not row['validation_only']
        d=directory/row['directory'];assert json.loads((d/'run.json').read_text())==row
        out=(d/'stdout.txt').read_text();err=(d/'stderr.txt').read_text();timing=(d/'time.txt').read_text()
        count,seen,valid,_=shared.parse(solver['kind'],out,err,row['exit_code'],p)
        assert valid and count==case['expected_count']
        if solver['kind']=='candidate':assert seen==row['observed_runtime_workers']==p
        elif solver['kind'] in ('FP','PlexEnum'):
            assert seen==row['configured_workers']==p and row['observed_runtime_workers'] is None
        assert shared.resource_metrics(timing)==row['cpu_resources']
        rss=re.search(r'Maximum resident set size \(kbytes\): (\d+)',timing)
        assert rss and int(rss[1])==row['peak_rss_kib']
        if solver['kind'] in ('FP','PlexEnum'):
            peer=Path(row['command'][row['command'].index(solver['binary'])+1])
            with Path(case['input']).open('rb') as f:f.read(4);expected=hashlib.sha256(f.read()).hexdigest()
            assert sha(peer)==expected
    complete=len(rows)==len(queue)
    completion_path=directory/'completion.json'
    if complete:
        marker=Path(plan.get('controller_exit_marker',str(plan_path.parent/'exit.txt')))
        assert marker.read_text().strip()=='0'
        completion=json.loads(completion_path.read_text())
        assert completion['status']=='passed' and completion['runs']==len(queue)
        summary=json.loads((directory/'summary.json').read_text())
        keys=[(cell['case'],cell['solver'],cell['workers']) for cell in summary]
        expected_keys={(c['id'],s['id'],p) for c,s,p,rep in queue}
        assert len(keys)==len(set(keys)) and set(keys)==expected_keys
        for cell in summary:
            group=[x for x in rows if (x['case'],x['solver'],x['workers'])==(cell['case'],cell['solver'],cell['workers'])]
            assert len(group)==plan['repetitions'] and cell['median_wall_s']==statistics.median(x['wall_s'] for x in group)
            assert cell['median_peak_rss_kib']==statistics.median(x['peak_rss_kib'] for x in group)
    return {'status':'audited-complete' if complete else 'audited-partial','completed':len(rows),'planned':len(queue),'plan_sha256':sha(plan_path),'driver_sha256':sha(driver),'measurements_sha256':sha(directory/'measurements.json'),'worker_semantics':'Candidate runtime workers checked; FP/PlexEnum printed workers are configuration, external thread samples are separate OS observations.','position_limitation':('Three repetitions do not exhaustively balance six solver positions.' if len(plan['solvers'])>plan['repetitions'] else 'Repetitions cover every solver position for this three-solver confirmation.'),'scope':'Raw count/resources/order/identity integrity only, not performance acceptance.'}


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--plan',type=Path,required=True);ap.add_argument('--directory',type=Path,required=True);ap.add_argument('--out',type=Path,required=True)
    args=ap.parse_args();assert not args.out.exists()
    result=audit(args.plan.resolve(),args.directory.resolve())
    args.out.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main()
