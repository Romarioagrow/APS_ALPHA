"""Read-only numerical inspection of the one-frame native UE water dump."""
import argparse
import json
import re
from pathlib import Path

import numpy as np


def objects(path):
    text = path.read_text(encoding="utf-8-sig")
    decoder = json.JSONDecoder()
    offset = 0
    values = []
    while offset < len(text):
        if text[offset:].isspace():
            break
        offset += len(text[offset:]) - len(text[offset:].lstrip())
        value, offset = decoder.raw_decode(text, offset)
        values.append(value)
    return values


parser = argparse.ArgumentParser()
parser.add_argument("dump", type=Path)
parser.add_argument("--samples", action="store_true", help="Include neighbouring shore pixel values")
args = parser.parse_args()
root = args.dump
assert (root / "Base/Status.txt").read_text().strip() == "ok"
passes = objects(root / "Base/Passes.json")
resources = objects(root / "Base/ResourceDescs.json")
composite = next(p for p in passes if p["EventName"].startswith("SLW::Composite "))
width, height = map(int, re.search(r"(\d+)x(\d+)", composite["EventName"]).groups())


def array(name, dtype, channels=1, pass_event=None, sub="mip0"):
    if pass_event is None:
        matches = [r for r in resources if r["Name"] == name]
    else:
        p = next(p for p in passes if p["EventName"] == pass_event)
        ids = {key.split(".")[0] for key in p["OutputResources"]}
        matches = [r for r in resources if r["Name"] == name and r["UniqueResourceName"] in ids]
    assert len(matches) == 1, (name, pass_event, matches)
    resource = matches[0]
    prefix = resource["UniqueResourceName"] + "." + sub + ".v"
    if pass_event is None:
        paths = list((root / "Resources").glob(prefix + "*.bin"))
        assert len(paths) == 1, (name, paths)
        path = paths[0]
    else:
        path = root / "Resources" / (prefix + p["Pointer"] + ".bin")
    result = np.fromfile(path, dtype=dtype)
    shape = (resource["ExtentY"], resource["ExtentX"])
    if channels > 1:
        shape += (channels,)
    result = result.reshape(shape)
    return result[:height, :width]


depth_scene = array("SceneDepthZ", "<f4")
depth_copy = array("SLW.DepthPrepassOutput", "<f4", pass_event="SLW::DepthBufferCopy")
depth_water = array("SLW.DepthPrepassOutput", "<f4", pass_event="SingleLayerWaterDepthPrepassParallel")
stencil = array("SLW.DepthPrepassOutput", "u1", pass_event="SingleLayerWaterDepthPrepassParallel", sub="stencil")
gbuffer = array("GBufferB", "u1", channels=4, pass_event="SingleLayerWaterParallel")
depth_without = array("SLW.SceneDepthWithout", "<f4")
prepass_water = (stencil & 1) != 0
basepass_water = (gbuffer[..., 3] & 15) == 10
missing = prepass_water & ~basepass_water


def quantiles(a):
    return [float(v) for v in np.quantile(a, [0, .01, .5, .99, 1])] if a.size else []


report = {
    "dump": str(root),
    "view": [width, height],
    "depth_copy_unequal_pixels": int(np.count_nonzero(depth_scene != depth_copy)),
    "prepass_water_pixels": int(prepass_water.sum()),
    "basepass_water_pixels": int(basepass_water.sum()),
    "prepass_without_water_gbuffer_pixels": int(missing.sum()),
    "water_gbuffer_outside_prepass_pixels": int((basepass_water & ~prepass_water).sum()),
    "depth_device_scene_q": quantiles(depth_scene[prepass_water]),
    "depth_device_water_q": quantiles(depth_water[prepass_water]),
    "depth_without_q": quantiles(depth_without[prepass_water]),
    "device_depth_water_minus_scene_q": quantiles((depth_water - depth_scene)[prepass_water]),
    "prepass_depth_equal_scene_pixels": int(((depth_water == depth_scene) & prepass_water).sum()),
    "missing_water_gbuffer_xy_first_12": np.argwhere(missing)[:12, ::-1].tolist(),
    "stencil_counts": {str(int(v)): int(n) for v, n in zip(*np.unique(stencil, return_counts=True))},
    "shading_model_counts": {str(int(v)): int(n) for v, n in zip(*np.unique(gbuffer[..., 3] & 15, return_counts=True))},
}
before_composite = array("SceneColor", "<f2", channels=4, pass_event="SingleLayerWaterParallel").astype(np.float32)
after_composite = array("SceneColor", "<f2", channels=4, pass_event=composite["EventName"]).astype(np.float32)
normals = array("GBufferA", "<f2", channels=4, pass_event="SingleLayerWaterParallel").astype(np.float32)


def unpack_r11g11b10(values):
    channels = []
    for shift, mantissa_bits in [(0, 6), (11, 6), (22, 5)]:
        component = values >> shift
        mantissa = component & ((1 << mantissa_bits) - 1)
        exponent = (component >> mantissa_bits) & 31
        result = np.where(exponent == 0, mantissa * 2.0 ** (-14 - mantissa_bits),
                          (1 + mantissa / 2.0 ** mantissa_bits) * 2.0 ** (exponent.astype(float) - 15))
        channels.append(result)
    return np.stack(channels, axis=-1)


sun_before = unpack_r11g11b10(array("SLW.SeparatedMainDirLight", "<u4", pass_event="SingleLayerWaterParallel"))
sun_after = unpack_r11g11b10(array("SLW.SeparatedMainDirLight", "<u4", pass_event="TiledUpsample"))
report["sun_shadow_changed_water_pixels"] = int((np.any(sun_before != sun_after, axis=-1) & basepass_water).sum())
report["water_pixels_without_main_sun"] = int((np.all(sun_before == 0, axis=-1) & basepass_water).sum())
report["colour_sample_rows"] = []
for row in ([160, 300, 450, 600] if args.samples else []):
    # The observer has open water on the left and land on the right.
    edge = int(np.flatnonzero(basepass_water[row])[-1])
    samples = []
    for x in range(edge - 12, edge + 3):
        samples.append({
            "x": x, "water": bool(basepass_water[row, x]),
            "device_gap": float(depth_water[row, x] - depth_scene[row, x]),
            "base_rgb": before_composite[row, x, :3].tolist(),
            "composite_rgb": after_composite[row, x, :3].tolist(),
            "sun_rgb": sun_after[row, x].tolist(),
            "encoded_normal": normals[row, x, :3].tolist(),
        })
    report["colour_sample_rows"].append({"y": row, "edge": edge, "samples": samples})
print(json.dumps(report, indent=2))
