# APS-74 gallery pixel-diversity evidence

Date: 2026-08-14

Result: **PASS for distinct rendered family outputs; no new manual visual PASS
is claimed**.

The accepted seed-73991 Frozen/Desert/Water/Living gallery PNGs were inspected
read-only. All images are 876 x 388 and have distinct SHA-256 identities.
Their PNG alpha channel is zero, but RGB data is valid, so statistics explicitly
ignore alpha and sample every fourth source pixel.

| Accepted case | SHA-256 | Mean RGB | Mean luma | Non-black samples |
| --- | --- | --- | ---: | ---: |
| Ice / Cryogenic | `FCD67602D4628D3887D8A581331830B80B9ECE3EEBDF768E46C66D5180D59ED3` | 14.49 / 15.11 / 14.50 | 14.94 | 9.09% |
| Desert | `D6620588281EF5FE030E7CC048C55ED7C1427345D07A518A472FC6737ED5AA28` | 15.07 / 8.41 / 2.25 | 9.38 | 8.05% |
| Water / Oceanic | `35139B1AAA11575C9B72955429E8678485B4FA31D699F3083C58CE1B296211FD` | 5.15 / 6.67 / 4.63 | 6.20 | 7.53% |
| Forest / Biosphere | `CE03DBBCB92A4324CF33E25F36C948B86C7CB22B7CD348F3A09DC4DB91B7DE20` | 3.04 / 4.83 / 2.22 | 4.26 | 7.52% |

## Pairwise sampled RGB delta

| Pair | MAE | RMSE | Samples with max-channel delta > 15 |
| --- | ---: | ---: | ---: |
| Ice - Desert | 6.63 | 29.73 | 7.45% |
| Ice - Water | 9.24 | 36.30 | 7.43% |
| Ice - Forest | 11.35 | 42.40 | 7.46% |
| Desert - Water | 5.42 | 25.87 | 7.37% |
| Desert - Forest | 5.81 | 27.81 | 7.39% |
| Water - Forest | 3.57 | 16.27 | 6.30% |

The changed-pixel share is localized to approximately the rendered planet
region rather than indicating a whole-frame replacement. Ice-to-Forest is the
largest sampled family distance; Water-to-Forest is the smallest, consistent
with the closer wet/living relationship. Together with canonical profile/MID
identity logs, this proves the accepted gallery fixtures do not collapse to one
fallback/default render.

This evidence does not replace the seed-424242 Water/Oceanic orbit-to-surface
parity truth, and it does not authorize changes to Ocean/Living or Land Coverage
baselines.

