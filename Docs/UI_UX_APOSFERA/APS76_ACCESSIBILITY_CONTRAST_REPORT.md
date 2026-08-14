# APS-76 accessibility contrast checkpoint

Date: 2026-08-14
Scope: opaque APOSFERA UI chrome tokens only; astronomical viewport rendering is excluded.

The ratios below use the sRGB token values from `FAPSUIStyle` against each profile's panel fill. Alpha compositing over a moving scene can only be accepted after same-camera runtime QA, so this is a source-level gate rather than a visual PASS.

| Semantic foreground | Balanced 2.2 | Cinematic 2.4 | Bright 2.0 | Source gate |
|---|---:|---:|---:|---|
| Primary text | 16.59:1 | 17.30:1 | 14.29:1 | PASS 4.5:1 |
| Secondary text | 9.64:1 | 9.58:1 | 9.32:1 | PASS 4.5:1 |
| Focus cyan | 10.26:1 | 10.36:1 | 9.39:1 | PASS 3:1 |
| Action amber | 8.57:1 | 8.76:1 | 7.99:1 | PASS 3:1 |
| Success | 11.28:1 | 11.11:1 | 10.03:1 | PASS 4.5:1 |
| Warning | 11.12:1 | 11.35:1 | 10.32:1 | PASS 4.5:1 |
| Danger | 6.31:1 | 6.53:1 | 5.91:1 | PASS 4.5:1 |

## Non-text boundary note

- Balanced quiet border: 3.18:1 against panel, 3.03:1 against control.
- Cinematic quiet border: 2.60:1 against panel, 2.50:1 against control.
- Bright quiet border: 4.09:1 against panel, 3.85:1 against control.

The Cinematic quiet border is deliberately subordinate and does not pass the 3:1 non-text threshold. It must never be the only focus, selection or status signal. Interactive focus uses cyan, selected hierarchy uses an amber rail, and readiness uses explicit status text plus semantic color.

## Runtime acceptance still required

- keyboard/controller focus visible on every interactive target;
- no state communicated by color alone;
- 200% UI scale and 1280x720 retain labels and exact values;
- reduced motion removes decorative timing without changing navigation;
- transparent root and moving astronomical content do not reduce effective readability;
- same-camera A/B for Generation and Strategic Map before visual PASS.
