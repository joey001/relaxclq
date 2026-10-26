#!/usr/bin/env python3
"""Read-only paired analysis for the registered nine-call P16 confirmation."""
import argparse,hashlib,json,statistics
from pathlib import Path
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--plan',type=Path,required=True);parser.add_argument('--directory',type=Path,required=True);parser.add_argument('--out',type=Path,required=True);args=parser.parse_args();assert not args.out.exists()
plan=json.loads(args.plan.read_text());directory=args.directory
assert plan['single_cell_confirmation'] is True and plan['planned_invocations']==9
rows=json.loads((directory/'measurements.json').read_text());completion=json.loads((directory/'completion.json').read_text());assert completion['status']=='passed' and completion['runs']==len(rows)==9
assert len({x['case'] for x in rows})==len({x['workers'] for x in rows})==1
assert all(x['status']=='completed' and x['count']==x['expected_count'] and not x['validation_only'] for x in rows)
solvers=['P013','P005','SafeOpenMP'];groups={s:[x for x in rows if x['solver']==s] for s in solvers}
for group in groups.values():assert len(group)==3 and {x['block_position'] for x in group}=={0,1,2}
paired=[]
for rep in range(3):
    group={s:next(x for x in groups[s] if x['repetition']==rep) for s in solvers};candidate=group['P013'];assert len({x['block'] for x in group.values()})==1
    paired.append({'repetition':rep,'block':candidate['block'],'p013_block_position':candidate['block_position'],'wall_p013_over':{s:candidate['wall_s']/group[s]['wall_s'] for s in ('P005','SafeOpenMP')},'cpu_p013_over':{s:candidate['cpu_resources']['total_cpu_s']/group[s]['cpu_resources']['total_cpu_s'] for s in ('P005','SafeOpenMP')},'directories':{s:x['directory'] for s,x in group.items()}})
new=[x['wall_p013_over']['P005'] for x in paired];old=plan['frozen_decision']['original_ratios']
result={'scope':'Exactly nine registered jazz(3,10) P16 calls;three solvers each occupy all three positions. No P1 call,self-speedup or cross-batch denominator. Frozen raw timing/count/resource values unchanged.','case':rows[0]['case'],'workers':rows[0]['workers'],'median_wall_s':{s:statistics.median(x['wall_s'] for x in group) for s,group in groups.items()},'wall_range_s':{s:[min(x['wall_s'] for x in group),max(x['wall_s'] for x in group)] for s,group in groups.items()},'median_total_cpu_s':{s:statistics.median(x['cpu_resources']['total_cpu_s'] for x in group) for s,group in groups.items()},'median_rss_kib':{s:statistics.median(x['peak_rss_kib'] for x in group) for s,group in groups.items()},'self_speedup':None,'self_speedup_scope':'Not measured;P1 absent;no borrowed denominator','paired':paired,'original_ratios':old,'new_ratios':new,'frozen_rejection_rule_met':all(x>1 for x in old+new),'rule':plan['frozen_decision']['rule'],'plan_sha256':hashlib.sha256(args.plan.read_bytes()).hexdigest(),'measurements_sha256':hashlib.sha256((directory/'measurements.json').read_bytes()).hexdigest()}
args.out.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
