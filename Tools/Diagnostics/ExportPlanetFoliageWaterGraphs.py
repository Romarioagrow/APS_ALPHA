"""Bounded read-only exports: actual leaf and water graphs, no package saves."""
import hashlib
import json
from pathlib import Path
import re
import unreal

output = Path('F:/ChatGPT/APOSFERA/work/planet_water_surface_20260930/leaf-water-graph-audit-v1')
if output.exists():
    raise RuntimeError('Existing audit must not be overwritten')
output.mkdir(parents=True)
pending = [
    '/WorldScape/Ressources/Mesh/Tree/MI_Grass_Leaf',
    '/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnalytic20260930/MI_APS_WaterAnalytic',
]
seen = set()
records = []
dependencies = re.compile(r'^\s*(?:MaterialFunction|Parent)="[^\x27\n]*\x27([^\x27\n]+)\x27"', re.MULTILINE)
while pending:
    path = pending.pop(0).split('.')[0]
    if path in seen:
        continue
    if len(seen) >= 48:
        raise RuntimeError('Graph bound exceeded')
    seen.add(path)
    obj = unreal.load_asset(path)
    if not obj:
        raise RuntimeError('Missing '+path)
    target = output / (obj.get_name()+'_'+hashlib.sha256(path.encode()).hexdigest()[:8]+'.t3d')
    task = unreal.AssetExportTask()
    task.object = obj
    task.filename = str(target)
    task.exporter = unreal.ObjectExporterT3D()
    task.automated = True
    task.prompt = False
    task.replace_identical = False
    if not unreal.Exporter.run_asset_export_task(task):
        raise RuntimeError('Export failed '+path)
    raw = target.read_bytes()
    text = raw.decode('utf-16') if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else raw.decode('utf-8-sig')
    pending.extend(dependencies.findall(text))
    records.append({'path':path, 'export':str(target), 'sha256':hashlib.sha256(raw).hexdigest()})
(output/'inventory.json').write_text(json.dumps(records, indent=2), encoding='utf-8')
unreal.log('APS_LEAF_WATER_AUDIT_EXPORTED count='+str(len(records))+' output='+str(output))
