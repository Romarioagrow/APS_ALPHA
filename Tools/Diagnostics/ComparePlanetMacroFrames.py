"""Read-only pixel comparison of controlled Unreal captures; not visual acceptance."""
import argparse
import json
from pathlib import Path

import numpy as np
from PIL import Image


def compare(directory: Path):
    result = []
    for original in sorted(directory.glob('*_Macro0Native.png')):
        record = {'view': original.stem.removesuffix('_Macro0Native')}
        a = np.asarray(Image.open(original).convert('RGB'), dtype=np.float64)
        for label in ('Macro1Control', 'Macro2Mean', 'Macro3Aperiodic'):
            path = original.with_name(original.name.replace('Macro0Native', label))
            if not path.exists():
                record[label] = {'missing': True}
                continue
            b = np.asarray(Image.open(path).convert('RGB'), dtype=np.float64)
            if b.shape != a.shape:
                raise ValueError(f'Capture dimensions changed: {path}')
            difference = np.abs(b - a)
            record[label] = {
                'mean_abs_rgb_0_255': float(difference.mean()),
                'p99_abs_rgb_0_255': float(np.quantile(difference, .99)),
                'pixels_over_3': float((difference.max(axis=2) > 3).mean()),
                'mean_rgb_0_255': b.mean(axis=(0, 1)).tolist(),
            }
        result.append(record)
    if not result:
        raise ValueError('No macro native captures found')
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    args = parser.parse_args()
    print(json.dumps(compare(args.directory), indent=2))
