#!/usr/bin/env python3
"""Summarize complete, count-verified adjacent-block timings without rerunning."""
import argparse
import json
from pathlib import Path
import statistics


def analyze(directory):
    rows=json.loads((directory/'measurements.json').read_text())
    completion=json.loads((directory/'completion.json').read_text())
    assert completion['status']=='passed' and completion['runs']==len(rows)
    assert all(x['status']=='completed' and x['count']==x['expected_count'] and not x['validation_only'] for x in rows)
    solvers=list(dict.fromkeys(x['solver'] for x in rows))
    cases=list(dict.fromkeys(x['case'] for x in rows))
    workers=sorted({x['workers'] for x in rows})
    assert 'P011' in solvers and 1 in workers
    result=[]
    for case in cases:
        groups={(s,p):[x for x in rows if (x['case'],x['solver'],x['workers'])==(case,s,p)] for s in solvers for p in workers}
        for p in workers:
            repetitions=sorted(x['repetition'] for x in groups['P011',p])
            assert len(repetitions)>=3
            paired=[]
            for rep in repetitions:
                group={s:next(x for x in groups[s,p] if x['repetition']==rep) for s in solvers}
                c=group['P011']
                assert len({x['block'] for x in group.values()})==1
                paired.append({'repetition':rep,'block':c['block'],'p011_block_position':c['block_position'],
                    'wall_p011_over':{s:c['wall_s']/x['wall_s'] for s,x in group.items() if s!='P011'},
                    'cpu_p011_over':{s:c['cpu_resources']['total_cpu_s']/x['cpu_resources']['total_cpu_s'] if x['cpu_resources']['total_cpu_s'] else None for s,x in group.items() if s!='P011'},
                    'directories':{s:x['directory'] for s,x in group.items()}})
            med={s:statistics.median(x['wall_s'] for x in groups[s,p]) for s in solvers}
            speedup={s:statistics.median(x['wall_s'] for x in groups[s,1])/med[s] for s in solvers}
            result.append({'case':case,'workers':p,'median_wall_s':med,'wall_range_s':{s:[min(x['wall_s'] for x in groups[s,p]),max(x['wall_s'] for x in groups[s,p])] for s in solvers},
                'median_total_cpu_s':{s:statistics.median(x['cpu_resources']['total_cpu_s'] for x in groups[s,p]) for s in solvers},
                'median_rss_kib':{s:statistics.median(x['peak_rss_kib'] for x in groups[s,p]) for s in solvers},
                'same_binary_self_speedup':speedup,'p011_self_speedup_over':{s:speedup['P011']/v for s,v in speedup.items() if s!='P011'},
                'median_adjacent_p011_over':{s:statistics.median(x['wall_p011_over'][s] for x in paired) for s in solvers if s!='P011'},
                'p011_absolute_vs_safe_p1':statistics.median(x['wall_s'] for x in groups['SafeOpenMP',1])/med['P011'],
                'paired':paired})
    return {'scope':'Complete six-solver small-graph screen only; three repetitions vary but do not exhaust six positions. Paired-ratio medians differ from ratios of medians. No trimming/normalization. CPU includes native idle/spinning. Integrity and these statistics are not a technique acceptance decision.', 'cells':result}


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--directory',type=Path,required=True);ap.add_argument('--out',type=Path,required=True)
    args=ap.parse_args();assert not args.out.exists();data=analyze(args.directory)
    args.out.write_text(json.dumps(data,indent=2)+'\n')
    for x in data['cells']:
        print(x['case'],x['workers'],'P011/P005 adjacent',round(x['median_adjacent_p011_over']['P005'],4),'P011wall',round(x['median_wall_s']['P011'],4),'self',round(x['same_binary_self_speedup']['P011'],3))

if __name__=='__main__':main()
