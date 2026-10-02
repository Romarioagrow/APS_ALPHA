"""Read-only atmosphere/cloud graph audit. Run only in an authorized offline session.

Exports serialized expressions and referenced material functions; never saves assets.
This is evidence for the next aerial-perspective integration, not a visual test.
"""
import hashlib
import json
from pathlib import Path
import re
import unreal

output = Path('F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/sky-graph-audit-v21')
if output.exists():
    raise RuntimeError('Existing audit must not be overwritten')
output.mkdir(parents=True)
pending = [
    '/AtmoScape/Materials/Master/MM_PlanetaryAtmo',
    '/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/CloudVolume20260930V20/M_APS_PlanetCloud',
]
seen = set()
records = []
dependencies = re.compile(r'^\s*(?:MaterialFunction|Parent)="[^\x27\n]*\x27([^\x27\n]+)\x27"', re.MULTILINE)
while pending:
    asset_path = pending.pop(0).split('.')[0]
    if asset_path in seen:
        continue
    if len(seen) >= 48:
        raise RuntimeError('Graph bound exceeded')
    seen.add(asset_path)
    obj = unreal.load_asset(asset_path)
    if not obj:
        raise RuntimeError('Missing '+asset_path)
    target = output / (obj.get_name()+'_'+hashlib.sha256(asset_path.encode()).hexdigest()[:8]+'.t3d')
    task = unreal.AssetExportTask()
    task.object = obj
    task.filename = str(target)
    task.exporter = unreal.ObjectExporterT3D()
    task.automated = True
    task.prompt = False
    task.replace_identical = False
    if not unreal.Exporter.run_asset_export_task(task):
        raise RuntimeError('Export failed '+asset_path)
    raw = target.read_bytes()
    text = raw.decode('utf-16') if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else raw.decode('utf-8-sig')
    pending.extend(dependencies.findall(text))
    records.append({'path':asset_path, 'export':str(target), 'sha256':hashlib.sha256(raw).hexdigest()})
(output/'inventory.json').write_text(json.dumps(records, indent=2), encoding='utf-8')
unreal.log('APS_SKY_AUDIT_EXPORTED count='+str(len(records))+' output='+str(output))
