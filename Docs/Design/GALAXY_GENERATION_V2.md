# Галактики и скопления V2 — дизайн и фазы

Status 2026-10-03: phase 1 implemented in generation code only (no UI/VM/AstroGenerator edits, no assets).
The changed and new files compiled and linked in the shared editor build of 03.10 02:09 (UHT with
`-WarningsAsErrors`, `UnrealEditor-APS_ALPHA.dll`, 1 succeeded / 0 failed; that build was not started by
this session). Two later source edits (test thresholds; draw sequencing in the new profiles, §3.3) are
cl-checked only and wait for the next build. Nothing committed. Not seen in PIE; the new automation
tests have not been run yet.
Owner of phase 1: Claude (galaxy session). Phase 2 needs the shared UI/VM/AstroGenerator files.

Status 2026-10-03 03:35: phase 2 implemented (REGENERATE rolls galaxy + cluster, per-type CLASS list, STARS
slider, cluster SIZE in extent and live sample, neighbour spacing, home-system exclusion in the continuous
frame). Every changed file cl-checked (exit=0); not built by UBT, not seen in PIE, automation not run,
nothing committed. The STARS slider is capped at **50,000** placed stars until the GPU point layer exists
(§3.1). Phase 2 details: §3.6–§3.9; what Rio should look at: §9.

**Phase 2 — кратко для Rio:**
- **REGENERATE** теперь бросает и небо: тип галактики, свой для типа подкласс, размер и плотность,
  тип/размер/население/состав скопления. Примерно каждое восьмое нажатие — готовый «архетип»
  (спираль с перемычкой, кольцевая, карликовая неправильная, эллиптическая с шаровыми, звёздный взрыв
  с OB-ассоциацией, сталкивающиеся, Магелланово облако). Colossal и Tiny случайно не выпадают.
- **CLASS** показывает только подклассы текущего типа; смена типа сама переводит класс (Barred + E5 →
  SBb). Под строкой подсказка, что делает подкласс.
- **STARS** вместо MODELED STAR COUNT: 1 800 … 50 000 реально поставленных звёзд (логарифмический
  ползунок). Карточка и инфо показывают это число, а не скрытые 100 млн. Больше 50 000 — после GPU-слоя.
- **Размер скопления виден:** Tiny показывает все свои системы и выглядит маленьким, Giant — 12 000
  живых систем, Colossal — 20 000 и шире кадра.
- **Звёзды скопления не налезают** друг на друга (рисуются меньше, где соседи ближе; позиции те же).
- **Звёзды внутри домашней системы скрываются** после каждой правки планет, орбит, размера звезды и
  после REGENERATE.

Offline shape prototypes (numpy port of the C++ formulas, 1800-point preview view and 250k-point dense
view, top and edge): `F:/ChatGPT/APOSFERA/work/galaxy_v2_20261003/` (`v2_spiral.png`, `v2_barred.png`,
`v2_other.png`, `v2_pec.png`, `clusters.png`, scripts `proto2.py`, `proto_clusters.py`). They show the
intended distributions, not engine frames.

## Кратко (для Rio)

- **Подклассы теперь свои у каждого типа** и реально меняют галактику:
  эллиптические E0…E7 + cD (сверхгигант) + dE (карлик); линзовидные S0, S0/a, SB0;
  спиральные Sa, Sab, Sb, Sbc, Sc, Scd, Sd, Sm; с перемычкой SBa…SBd + SBab/SBbc/SBcd + SBm;
  неправильные Irr, Im, IBm, dIrr, I0 (звёздный взрыв); пекулярные — изогнутый диск, кольцевая,
  взаимодействующая пара, приливные хвосты (как «Антенны»), полярное кольцо.
  От Sa к Sd балдж уменьшается, рукава раскрываются, становятся моложе (голубые узлы).
- **Принятая спираль не меняется.** Твой мир 02.10 был «Spiral + E4» (две ветви). Новый Sb по
  умолчанию у спирали идёт тем же старым кодом с двумя ветвями — те же позиции и цвета звёзд
  (это проверяет новый автотест против замороженной копии старого кода; тест ещё не запускался).
  Так же SBb у спирали с перемычкой и «изогнутый диск» у пекулярной. E0, S0, Irr остались как были.
- **Старые сохранения грузятся теми же мирами** — старые значения классов идут тем же кодом, что и
  раньше, новые значения добавлены в конец перечислений.
- **Скопления:** новый размер **Colossal** (50–100 тыс. систем, больше Giant) и пять новых форм:
  молодая OB-ассоциация, движущаяся группа, сверхплотное скопление, скопление в газовых
  волокнах, двойное скопление (как h и χ Персея). Размеры остаются диапазонами.
- **Миллион звёзд:** каталог уже процедурный (100 млн «смоделировано»); ползунок задаёт,
  сколько из них реально поставить (цель 1 800 … 1 000 000; в фазе 2 пока до 50 000 — больше
  потянет только GPU-слой), это всегда начало одного порядка — при
  увеличении звёзды только добавляются, старые остаются на месте и с теми же ID. Для больших чисел
  звёзды автоматически становятся мельче и тусклее, чтобы галактика выглядела объёмной, а не
  белым пятном. Рисовать 1 млн через нынешний HISM дорого — для фазы 2 предлагаю отдельный
  лёгкий слой (ниже), для 100 млн — потоковый GPU-рендер.
- **Клик по любой звезде** (фаза 2) — через пространственную сетку каталога (луч мыши → ячейки →
  ближайшая звезда), без перебора миллиона инстансов.
- Фаза 2 сделала ползунок STARS вместо MODELED STAR COUNT и список CLASS по типу (см. начало файла);
  осталось: слой на 1 млн звёзд и выбор звезды галактики кликом.

## 1. Request (Rio, 03.10)

1. A star-count slider for the galaxy, up to 1,000,000 really placed stars (ideally 100M later) at
   ~120 FPS, filled realistically by each type's pattern; remove "MODELED STAR COUNT".
2. Subclasses are the same for every type: distribute them logically per type and make the
   subclass actually change the result.
3. Every star clickable, with its data, at good performance.
4. Star clusters: a size bigger than Giant, a few new cluster types, sizes stay ranges.
5. No regressions: no galaxy type may break; a more volumetric look for big star counts.

## 2. Current state (facts this design builds on)

- The galaxy is an indexed procedural catalogue: `FGalaxyCatalogDescriptor::ResolveStar(index)`
  (SplitMix64 per record, O(1), thread-safe). `GalaxyStarCount` (default 100,000,000) is the modeled
  catalogue size; only a prefix of one nested full-cycle permutation
  (`APSCanonicalStellarProjection::MakeNestedCatalogPermutation`) is sent to the HISM:
  1800 in the menu preview, `RuntimeMaxGalaxyInstances` = 25,000 in gameplay. A larger budget keeps
  every smaller budget's stars (same StableIds).
- Class semantics before 03.10: E0..E7 flatten the elliptical; for Spiral/BarredSpiral the class only
  sets the arm count `2 + class % 4` (any class, E-classes included: E0/E4/Sd → 2 arms, Sa → 3, Sb → 4,
  Sc → 5); Lenticular, Irregular and Peculiar ignore the class. The menu offers all 19 classes for
  every type (`GetSelectableEnumValues`). Rio's last tested world (log 02.10 18:17) is
  `GalaxyType=Spiral, GalaxyClass=E4` → the accepted look is the historic two-arm spiral.
- The galaxy seed is `Hash(PreviewGenerationSeed, GalaxySize, GalaxyStarCount, GalaxyType)`; the class
  is not part of it, so subclasses of one type re-shape the same stars.
- Clusters: sizes are ranges (Tiny 100–500 … Giant 25k–50k); eight formations; every cluster system
  is a sealed record in `UGeneratedWorld::CanonicalStellarDataset` and in the save. Saves with a Giant
  cluster (36,455 records) are ~81 MB, i.e. ~2.2 KB per record.
- Rendering: HISM, carrier mesh `XSM_APS_STAR_SPHERE_V10X2_AutoLOD`, material
  `M_SpectralStarMat_POINTS` (custom PSF HLSL in `APSStellarPointSampling.h`), 6 custom floats (8 in
  gameplay after the appearance repack). Picking exists only for cluster systems (O(N) screen
  projection per click). Galaxy stars are not pickable.
- Saves: `UGeneratedWorld` is a tagged UObject snapshot (`APSWorldSaveSnapshot`,
  `FObjectAndNameAsStringProxyArchive`): enum properties are stored by enumerator name. The dataset
  is reused only when `IsUsable` (Version 3) and `InputHash`/`DatasetHash` match; `InputHash` hashes
  enum bytes and deliberately excludes render budgets.

## 3. Design

### 3.1 Placed star count (replaces MODELED STAR COUNT)

- The slider sets the **render budget** N (1,800 … 1,000,000) = the first N stars of the nested
  catalogue order. `GalaxyStarCount` (modeled, 100M) stays as the hidden catalogue size so the galaxy
  seed and old saves are untouched. Moving the slider never reshuffles: it adds or removes stars at the
  tail of one fixed order; a clicked star keeps its StableId.
- New model field (phase 2): `UGeneratedWorld::GalaxyPlacedStarCount` (int32, 0 = historic budgets).
  It is a render budget, so it must **not** enter `BuildCanonicalStellarDatasetInputHash`.
- Phase 1 already accepts large budgets: `UGalaxyGenerator::GenerateGalaxyOctreeStars` resolves budgets
  above 25,000 in parallel 64k chunks (`APSGalaxyCatalogBatch::ResolveStars`, ~8k records per task) and
  uploads in catalogue order; budgets ≤ 25,000 run the original loop.
- **Phase 2 (done):** `UGeneratedWorld::GalaxyPlacedStarCount` and `AAstroGenerator::GalaxyPlacedStarCount`
  (copied in `ApplyWorldModel`); `UWorldGenerationViewModel::SetGalaxyPlacedStarCount` clamps to
  [`PreviewReferenceBudget` 1,800, `MaxPlacedStars`]; `GenerateGalaxy` uses the placed count as the instance
  budget in both preview and gameplay when it is > 0 and passes `DensityReferenceBudget` 1800 / 25000.
  The panel row is a log slider + spin box (`LogNumberRow`, three significant digits, applied on commit);
  the galaxy info text, the LIVE MODEL card and the overview show the placed (or rendered) count; the
  modeled 100M is no longer shown anywhere.
- **Cap `APSGalaxyMorphology::MaxPlacedStars` = 50,000** (one constant). Reason: the menu draws catalogue
  stars through the continuous preview frame, which re-projects every point on the CPU on each moving
  frame (~0.28 µs per point, derived from the 1,800-point menu frame; not profiled at 50k) — ~10–14 ms
  at 50k, ~280 ms at 1M. Static frames cost nothing (frame-key skip). 1M needs the GPU point layer (§4);
  when it lands, raise the constant and the slider follows.

### 3.2 Subclasses per type

Each type offers its own list (`APSGalaxyMorphology::GetSubclasses`); the default reproduces the
type's pre-03.10 look. Historic enumerators keep their names/bytes; new ones are appended after
`Unknown`. Sa..Sd and SBa..SBd stay valid for old saves (display name "(legacy)") but are not offered.

| Type | Offered subclasses (Hubble order) | Default | What changes along the list |
|---|---|---|---|
| Elliptical | E0 E1 E2 E3 E4 E5 E6 E7 (historic), cD, dE | E0 | E-number flattens (historic); cD: bright core in a vast old envelope; dE: diffuse flattened dwarf with a nucleus |
| Lenticular | S0 (historic), S0/a, SB0 | S0 | S0/a: faint smooth arm traces; SB0: bar; both old, thick-disk share |
| Spiral | Sa Sab **Sb** Sbc Sc Scd Sd Sm | Sb | bulge share 30% → 1%, arms 2.7 → 0.85 turns, 2 → 3 → 4 arms, smooth → knotty, old (red) → young (blue); Sm one-armed with an off-centre bar |
| Barred spiral | SBa SBab **SBb** SBbc SBc SBcd SBd SBm | SBb | bar 30% / 0.50 R → 13% / 0.28 R, arms leave the bar ends, inner ring for SBa/SBab, same age trend; SBm off-centre bar, one arm |
| Irregular | Irr (historic), Im, IBm, dIrr, I0 | Irr | Im: off-centre bar + stubby arm (LMC); IBm: strong bar + two lobes; dIrr: soft dwarf clouds; I0: M82-like cigar + bipolar outflow |
| Peculiar | **Pec warped** (historic code), ring, interacting pair, tidal tails, polar ring | Pec warped | Cartwheel ring with knots and spokes; spiral + companion + bridge + counter-tail; Antennae merger; S0 host crossed by a young polar ring |

Bold defaults are new enumerators routed to the historic code (`EForm::Historic`): SpiralSb →
historic class Sd (two arms, identical to E0/E4), BarredSBb → SBd, PecWarped → Irr (the historic
peculiar code ignores the class). Positions, spectra and per-star seeds are identical to the historic
pair; only StableIds differ because they hash the class value.

Phase 2 (done): `SWorldGenerationPanel` `EnumRow` takes an optional value provider; the CLASS row cycles
`GetSubclasses(GalaxyType)` only and shows `GetSubclassSummary` in a hint row below it.
`UWorldGenerationViewModel::SetEnumValue` coerces the class when the TYPE changes
(`CoerceSubclass`: a class of the type is kept, anything else becomes the type's default — Barred + E5 →
SBb, Spiral + E4 → Sb, the same picture). A loaded legacy pair (Spiral + E4) is shown and generated as
is; the first CLASS arrow press moves into the type's own list.

### 3.3 Morphology engine (`Generation/APSGalaxyMorphology.h/.cpp`, `Generation/APSHashStream.h`)

- `FindProfile(Type, Class)` returns a profile only for appended classes offered by that type;
  otherwise `ResolveStar` runs the historic code unchanged.
- Parametric forms: Disk (halo, bulge, bar, exponential-rim disk, Archimedean arms like the historic
  spiral, bell-shaped arm cross-section, star-forming knots fixed per galaxy seed, inner ring, thin +
  thick disk), Spheroid (core + envelope + nucleus, axis ratios), Irregular (soft lobes, offset bar,
  stub arm, knots), Starburst, Ring, Interacting, TidalTails, PolarRing.
- The population split uses the historic selector bits of the record hash and every parametric
  profile draws from one stream keyed by (seed, index), so neighbouring subclasses morph rather than
  reshuffle (halo stars stay, arms rotate).
- Population age: arms/knots/rings are young, bulges/halos/ellipticals old.
  `ApplyPopulationAge` adds hot O/B/A stars to young bodies (up to +22%) and turns hot stars of old
  bodies into K/M, from its own stream, so the following record draws are untouched. Colours, the
  clicked star's data and the visuals agree.
- Every parametric position is kept inside 1.0 R (the catalogue envelope and projection frame use
  1.05 R).
- Every random draw of the new code is its own statement: C++ leaves the order of calls inside one
  expression unspecified, and a catalogue must not depend on the compiler. (The historic code has a
  few such expressions, e.g. the Irregular lobe centre; they are frozen as MSVC compiles them — do not
  "clean them up", that would move stars of old saves.)

### 3.4 Volumetric look at large counts

- Soft edges everywhere in the new profiles (bell noise, soft disk rim, thick-disk share, halo).
- `APSGalaxyMorphology::GetDensityCompensation(N, Reference)`: identity for N ≤ Reference; above it each
  star's radius × (Ref/N)^0.25 (≥ 0.18) and emission × (Ref/N)^0.8, so total light grows as N^0.2
  (1M vs 1800: ≈ 3.5×) instead of 555×. The existing area compensation in
  `GetFarStarVisualEmission` keeps per-star flux constant when the radius shrinks.
  `GenerateGalaxyOctreeStars(..., DensityReferenceBudget = 0)`: 0 keeps the historic presentation for
  every budget; phase 2 passes 1800 (preview) / 25000 (gameplay) together with the placed count.
  Exponents are first estimates to calibrate on frames.

### 3.5 Clusters (`Generation/StarClusterGenerator.*`, `Core/Enums/StarCluster*.h`)

- `EStarClusterSize::Colossal` (appended after Unknown): 50,000–100,000 systems.
- Five formations appended after Hourglass, each a pure function of (seed, index, star mass)
  (`UStarClusterGenerator::SampleSeededFormation`), independent of the global RNG and of the render
  prefix (the first 1600 records already show the whole shape), inside ±Bounds/2:

| Type | Bounds (x, y, z) | Shape |
|---|---|---|
| Young Association | 220k, 160k, 90k | 5–8 loose unbound sub-groups in a flattened field population |
| Moving Group | 260k, 110k, 80k | sparse elongated co-moving cloud, faint core, gentle S-bend |
| Super Star Cluster | 60k, 60k, 60k | truncated Plummer core (a = 0.035), mass segregation |
| Embedded / Filaments | 120k, 120k, 90k | dense hub + 4–6 curved gas filaments traced by young stars |
| Double Cluster | 200k, 100k, 80k | two round open clusters (h and χ Persei) in a shared halo |

- Historic formations are untouched (only new `case`s were added; the global RNG draw order of the
  historic types is unchanged).
- Save size: a Colossal cluster stores 50k–100k sealed records, i.e. **~110–220 MB per save** at today's
  ~2.2 KB/record. Acceptable for a first step; the scalable fix is phase 3 (§7).

### 3.6 Cluster SIZE in extent and live sample (phase 2)

Before: every size drew ~1,600 systems (Tiny 450) in the same Giant-sized volume, so Tiny 451 and
Colossal 93,884 looked alike. Now (`UStarClusterGenerator` static helpers):

| Size | `GetSizeExtentFactor` (× table bounds) | `GetPreviewFormationBudget` (live systems) | Range (systems) |
|---|---|---|---|
| Tiny | 0.30 | 500 (all of them; was 450) | 100–500 |
| Small | 0.40 | 1,500 (all of them; was 800) | 500–1,500 |
| Medium | 0.55 | 3,000 (was 1,100) | 1,500–5,000 |
| Large | 0.78 | 6,000 (was 1,400) | 5,000–25,000 |
| Giant | **1.00** (accepted default unchanged) | 12,000 (was 1,600) | 25,000–50,000 |
| Colossal | 1.26 | 20,000 (was 1,000) | 50,000–100,000 |

- The extent factor (roughly constant star density, extent ~ count^⅓, softened so Tiny stays readable)
  scales only **new** cluster bounds (`GenerateStarCluster`, non-reuse branch); a sealed dataset keeps its
  stored bounds.
- `ComposeCanonicalStellarProjection` now measures `ClusterToGalaxyScale` against the type's **unscaled**
  table extent (`GetLogicalHalfExtent(GetStarClusterBoundsByRange(type), type)`, the exact formula of
  `GenerateStarCluster`), so a scaled cluster occupies a proportionally smaller/larger share of the
  galaxy. Save safety: §6.7.
- CLUSTER focus frames every size against its type's Giant extent
  (`GetContinuousPreviewPhysicalFocus`), so Tiny reads small and Colossal fills past the frame.
- The preview budget no longer follows `PreviewMaxInstances` for clusters (the helper is the cap).
  Each live system costs its HISM instance plus ~0.2 µs per moving continuous frame (estimate).

### 3.7 Star spacing in dense clusters (phase 2, presentation only)

At the compressed menu scale a dense core (Supercluster, Super Star Cluster, Colossal) can hold stars
closer than their drawn radii. `LimitPointRadiiToNeighbourSpacing` (`AstroGeneratorPreviewFrame.cpp`,
X-sorted two-way sweep, O(N log N) typical) caps each continuous-frame point's drawn radius at **45 % of
the distance to its nearest neighbour**, once per resolved star layer. Positions, StableIds and star
data are untouched; the historic formation code is unchanged. Log:
`[APS.Preview.Cluster] Neighbour spacing: L of N star points drawn smaller so none overlap`.

### 3.8 Home-system exclusion in the continuous frame (phase 2)

The canonical HISM path already suppresses catalogue stars inside the home system
(`FinalizeCanonicalStellarProjectionBuild`, `[APS.CanonicalProjection] SYSTEM exclusion ...`), but the
menu draws the continuous frame, which ignored it. Now `ApplyContinuousPreviewFrame` measures the home
sphere every applied frame from the live home bodies (max of body distance + body radius, ×
`SystemProxyExclusionPadding`) and zero-scales every catalogue point (galaxy and cluster views) whose
sphere touches it, except the home record itself. The sphere is part of the frame key, so planet, orbit
and star-size edits, `SetPreviewSystemEditOverride` and REGENERATE re-apply it without a camera move;
static frames still skip. Log once per change and view:
`[APS.Preview.Exclusion] <view>: N catalogue stars hidden inside the home system (radius R cm)`.
Gameplay keeps the generation-time HISM exclusion, computed after the (edited) home system is built;
the home system does not change during gameplay. Assumption: home star/planet actors are static between
edits (no orbital animation exists today); if presentation code ever moves them every frame, quantize
the radius in the key.

### 3.9 REGENERATE rolls the sky (phase 2, `UI/MainMenu/APSWorldRoll.cpp`)

`APSWorldRoll::Apply` (System scope; PlanetOnly is untouched) calls `RollSky(World, RollSeed)` after the
home system is rolled. It uses its **own stream** (`HashCombineFast(GetTypeHash(RollSeed), 0x534b5952)`),
so every home-system draw of the same roll seed stays where it was. Deterministic per roll seed; names
follow the new generation seed (`APSBodyNames`; REGENERATE already clears typed names). The STARS budget
(`GalaxyPlacedStarCount`) is the player's render setting and is not rolled.

- **12 % of presses** pick a hand-made archetype, otherwise independent weighted draws.
- Galaxy type: Spiral 30, Barred 25, Elliptical 15, Irregular 12, Lenticular 10, Peculiar 8.
- Subclass (only the type's own list, then `CoerceSubclass`): E0/E1/E2 12, E3 11, E4 10, E5 9, E6 7,
  E7 5, cD 10, dE 12 · S0 45, S0/a 30, SB0 25 · Sa/SBa 8, Sab 10, Sb 20, Sbc 16, Sc 18, Scd 12, Sd 9,
  Sm 7 · Irr 30, Im 25, IBm 15, dIrr 20, I0 10 · Pec warped 20, ring 22, interacting 22, tidal 20,
  polar 16.
- GALAXY SIZE by class: dE/dIrr 110–180, Im/IBm/Sm/SBm 140–220, Irr/I0 160–260, cD 380–520,
  E0–E7 240–420, Sa–Sb/SBa–SBb 240–380, others 210–340. STAR DENSITY 7–14 (default 10).
- Cluster type depends on the galaxy (all 12 formations possible everywhere): spirals lead with
  Ring/Arc 14, Young Association 12, Nebula 11; ellipticals/lenticulars with Globular 18, Supercluster
  14; irregulars with Young Association 16, Embedded 14, Super Star Cluster 12; peculiars with Super
  Star Cluster 16, Embedded/Stream/Young Association 12.
- Cluster size, **Small..Giant only**: loose formations (Moving Group, Young Association, Double, Open)
  22/32/28/18; dense (Super Star, Globular, Supercluster) 6/20/34/40; others 15/25/30/30. Tiny reads as
  an empty sky and Colossal stores 50–100k sealed systems per save (~110–220 MB), so neither is rolled;
  both stay deliberate choices on the STAR CLUSTER page.
- Population and composition follow the formation: young formations main-sequence/protostars and blue;
  globulars giants/dwarfs and orange-red; others mostly the full spectrum (single colours ≤ 1 %).
- Archetypes (weights): barred grand design 18 (SBb/SBbc 340–440 + Giant Ring/Arc or Nebula), dwarf
  irregular 16 (dIrr/Im 110–170 + Small/Medium Moving Group, Open or Young Association), globular-rich
  elliptical 16 (cD/E1/E2 + Giant Globular, giants, orange-red), ring galaxy 14 (Pec ring + Large Young
  Association, blue), starburst association 14 (I0 + Large Young Association or Embedded, protostars,
  blue), colliding galaxies 12 (interacting / tidal tails + Giant Super Star Cluster), Magellanic cloud 10
  (Im/IBm + Medium/Large Double Cluster or Young Association).
- The REGENERATE log line ends with
  `galaxy=<type>/<class> size=<n> density=<d> stars=<n> gpop=<p> gcomp=<c> "<name>" cluster=<type>/<size> pop=<p> comp=<c> "<name>" sky=<archetype|standard>`.
- Since 03.10 04:20 REGENERATE also rolls STARS (log-normal, median ~15k: dE/dIrr 6k, Im/IBm/Sm 9k,
  Irr/I0 10k, cD 24k, × √(size/250), 1,800..50,000) and the galaxy POPULATION / COMPOSITION (§3.12).

### 3.10 Crash fix and resolved-star budget (03.10 04:15)

Crash `InStateBucketId < (1 << 14) - 1` (NaniteMaterials.h): at GALAXY SIZE 1 the galaxy radius is ~720
solar radii (positions scale with SIZE × 50,000 units of 1e9 cm, star radii are physical), the 16%-scaled
home cluster collapsed into overlapping giants and all 26,821 cluster stars asked for a photosphere +
corona pair (own MIDs, one Nanite material bin each) — 3.6 s in one frame, then the assert.
- `aps.Preview.ResolvedStarCap` = 48 pairs at most, largest on screen first (×1.25 hysteresis for pairs
  already shown); the rest stay optical points of the same size. Pool preparation is capped the same way.
- At most 16 new pairs per applied frame, and only within 3 ms after the first four.
- `APSGalaxyMorphology::MinGalaxySize` = 50 (VM setter, panel row, roll). Old saves are not clamped.
- `aps.Preview.MaxPointPixels` = 64: a star beyond the pair cap is drawn as a point of at most 64 px radius
  (both catalogue paths).

### 3.11 Moving-frame cost of the continuous preview (03.10 04:30–05:00)

Per moving frame the menu re-projected every catalogue point serially (~220–250 ns per point incl.
`BatchUpdateInstancesTransforms`' per-instance FTransform → relative → matrix work): ~17–19 ms for 50k galaxy +
~27k cluster points (Rio: 30–40 fps orbiting, 32 ms at STARS 50k). New, all behind console variables with
the serial path kept:
- `aps.Preview.ParallelResolve` (1): the per-frame resolved-star test and its flight forecast run as
  ParallelFor; candidates are re-tested serially in catalogue order (identical result).
- `aps.Preview.FastCatalog` (0 until A/B): one ParallelFor computes each instance's matrix, optics and
  change flag; rows are sent as precomputed matrices with `BatchUpdateInstancesData` (one call when >25%
  changed). Materialized stars keep the serial evaluation. Estimate: ~10–20 ns per point on the game
  thread plus ~12 ns per sent row (~2–3 ms for 77k points).
- `aps.Preview.CatalogCull` (0 until A/B): points outside the real view frustum (+32 px guard) are
  zero-scaled and not sent again until they return.
- `PresentPhysicalMesh` no longer calls `SetAbsolute` (a forced transform propagation) once a mesh is absolute.
- `[APS.Preview.Slow]` shows the resolved-pair count and whether the fast path ran.

### 3.12 Galaxy POPULATION / COMPOSITION (03.10 05:20)

The cluster's presets for the galaxy catalogue (`UGeneratedWorld::GalaxyStarPopulation` /
`GalaxyStarComposition`, `EStarClusterPopulation` / `EStarClusterComposition`; GALAXY rows POPULATION and
COMPOSITION; LIVE MODEL card; REGENERATE).
- `APSGalaxyMorphology::ApplyStarMix` runs after every historic draw of `ResolveStar`, from its own stream.
  All Sequences + All Spectral (zero, the default and every older save) leave the record untouched.
- POPULATION draws a stellar type with the cluster's weights and sets `FGalaxyCatalogStarRecord::RadiusScale`
  (sub-giant 2, giant 8, bright giant 20, supergiant 50, hypergiant 100, sub-dwarf 0.6, white dwarf 0.006
  of class A/B/F, protostar 2.5; brown dwarfs, neutron stars, pulsars and black holes take their own
  class). Every galaxy radius consumer multiplies by it, luminosity by its square (bounded 1e-4..1e4).
- COMPOSITION re-draws the spectral class from the cluster's colour tables (except remnant/brown-dwarf types).
- Kill switch `aps.Galaxy.StarMix` (1). Not part of any InputHash (the catalogue is never stored).
- Roll: ellipticals/lenticulars 50% historic, else giants/dwarfs and red/orange; irregulars and peculiars
  young/blue; spirals 60–70% historic; archetypes pick their own (globular-rich elliptical: giants +
  orange-red; starburst / colliding: protostars + blue; ring: main sequence + blue-white).
- Galaxy.h carries the new members as non-reflected fields on existing lines, so UHT line numbers stay.

### 3.13 Presentation spacing in dense cluster cores (03.10 05:05)

`aps.Preview.ClusterSpacingPixels` (0 = off until A/B; suggested 3). When the live cluster sample is built,
system centres are spaced for presentation only: a shell density cap around the cluster centre (direction and
distance order kept, ≤ one system per 5·s³) and up to six relaxation steps on a spatial hash (pairs closer
than s pushed apart half each; home fixed). s = pixels × CLUSTER framing distance × pixel tangent. Points,
visited systems (`GetContinuousPreviewSystemCenter`), labels and picking read the same offset
(`APSPreviewClusterSpacing::GetOffsetCm`), so nothing jumps on a visit. Catalogue, saves and gameplay never
move. Log: `[APS.Preview.Cluster] Spacing … N of M systems moved apart`.

## 4. Rendering 1M stars (phase 2) and 100M (phase 3)

HISM is the wrong primitive above ~100k: per star it holds a double FMatrix (128 B), custom data,
reorder tables, our `RenderedProxyBaseTransforms` (FTransform, 96 B) and catalogue index (8 B), the
render-thread copy and GPU-Scene data — roughly 450 B/star (~450 MB at 1M) plus a CPU cluster-tree
traversal per view. The sphere carrier costs tens of triangles per star.

| Option | 1M cost (estimates) | Pros | Cons |
|---|---|---|---|
| A. Bulk ISM layer (recommended for phase 2) | GPU ~1–3 ms (8 tris/star), CPU ~160 MB + GPU ~100 MB, upload ~0.5 s | Pure C++; same material/PSF (accepted look); no new module/assets (octahedron carrier built at runtime via `UStaticMesh::BuildFromMeshDescriptions`); every HISM consumer untouched | Memory heavy; not a path to 100M |
| B. Niagara GPU sprites from array DI | GPU ~0.5–1 ms, ~70 MB | Lean | Needs Niagara module in Build.cs, a system asset and a Niagara variant of the point material → second material path to keep identical |
| C. Custom point renderer (SceneViewExtension + global shader, vertex pulling from an SRV) | GPU ~0.3–0.5 ms, 16 B/star (16 MB) | Fastest, scales to 10M+ with GPU culling/LOD | Shader plumbing (separate PostConfigInit module for the shader directory), PSF ported to a global shader |

Recommendation: **phase 2 = A**. `AGalaxy` keeps its HISM for the first `HistoricBudget` stars (1800
menu / 25000 gameplay — every existing consumer: native star pairs, far glyphs, appearance repack,
probes, cluster/galaxy hashes) and gets a runtime-created `UInstancedStaticMeshComponent` (no UPROPERTY,
owned component) for ordinals [HistoricBudget, N). The bulk layer needs no per-instance arrays: ordinal
= HistoricBudget + instance index, catalogue index = `Order.Resolve(ordinal)`. Generation: chunked
`APSGalaxyCatalogBatch::ResolveStars` (≈ 20–40 ms per 1M on 8+ cores, estimate), one `AddInstances` and
one custom-data fill. Hide the bulk layer below galaxy scale in gameplay (it is background there).
**Phase 3 = C** with view-dependent streaming for 100M: an octree over the catalogue in canonical space;
cells near the camera resolved procedurally at full density, far cells drawn as aggregated glow
impostors (integrated colour and flux); resident set ≤ ~1–2M stars.

Performance targets to verify with frames + `[APS.Perf]` (menu currently 8.3 ms, GPU 4.5 ms at 120 fps):
1M placed stars must keep the menu at ≥ 110 fps and add ≤ 3 ms GPU.

## 5. Picking every star (phase 2)

- At generation keep a compact `TArray<FVector3f>` of proxy-space positions of the placed set (12 B/star)
  and a uniform grid over it (~N/8 cells; cell start offsets + ordinals sorted by cell, counting sort,
  ≈ 5 B/star; ~20–40 ms per 1M, parallel).
- Click (and optionally hover ≤ 30 Hz): deproject the mouse to a ray, transform into galaxy component
  space, march the grid with a 3D DDA; at distance t visit the cells within the pick cone
  (MaxPixelDistance × t × pixel tangent); among candidates take the smallest screen distance, ties by
  depth (same rule as `FindPreviewClusterSystemAtScreenPosition`). Cost independent of N, < 0.2 ms.
- Result: ordinal → catalogue index (HISM prefix: `RenderedCatalogIndices`; bulk layer: permutation)
  → `ResolveStar` → `FGalaxyCatalogStarRecord` (StableId, spectral class/subclass, position, potential
  system flag) → UI card. No HISM hit tests, no collision, no per-instance arrays.
- Fallback / test oracle: brute-force `ParallelFor` projection of the position array (~2–4 ms per 1M
  per click) — used by an automation test to validate the grid result.

## 6. Save compatibility — why old saves load identical worlds

1. **Enums:** the snapshot stores enum properties by name. All new enumerators are appended after the
   existing ones (`EGalaxyClass` after `Unknown` = 18, `EStarClusterSize` after `Unknown` = 5,
   `EStarClusterType` after `Hourglass` = 7). No existing name or byte changed; only display names of
   Sa..SBd gained "(legacy)", and display names are not serialized. The legacy summary path
   (`FGeneratedWorldData`) uses the same enums.
2. **Dataset validation:** `FAPSCanonicalStellarDataset` (Version 3), `IsUsable`, `InputHash` and
   `BuildCanonicalStellarManifestHash` are untouched; they hash enum bytes and fields whose values did
   not change, so every old dataset validates exactly as before (no new rejects).
3. **Galaxy:** stars are not stored; they are re-resolved from (seed, count, size, density, type, class).
   For every pre-03.10 (type, class) pair `FindProfile` returns nullptr, so `ResolveStar` runs the
   historic code with `HistoricClass == GalaxyClass` — same arithmetic, same stream draw order, same
   StableIds, same spectra (`ApplyPopulationAge` is skipped when the age is 0). The render prefix uses
   the same permutation and budgets; `DensityReferenceBudget` defaults to 0 (identity) and budgets
   ≤ 25,000 run the original upload loop. Test `APS.Generation.GalaxyV2.LegacyCatalogUnchanged`
   compares 12,768 records (2 seeds incl. Rio's 02.10 world × 7 types × 19 historic classes × 48
   indices) with a frozen copy of the 02.10 algorithm.
4. **Clusters:** sealed records store positions and star models, so generator changes cannot reach a
   loaded cluster. The historic formation code is unchanged (only new cases); test
   `APS.Generation.ClusterV2.LegacyFormationsUnchanged` compares all eight historic formations against a
   frozen copy under the same global RNG seed. Unsealed previews of historic settings consume the global
   RNG exactly as before.
5. **Random models** (`GenerateRandomGalaxyModel`, `GetRandomStarClusterModel`) are unchanged, so the
   authored/legacy random routes are unchanged.
6. Not guaranteed (as usual): a world saved with a new subclass/size/formation cannot be opened by a
   build that predates these enumerators.
7. **Phase 2:**
   - `GalaxyPlacedStarCount` is a new tagged UPROPERTY added after `GalaxyStarCount`; an old snapshot has
     no tag, the field stays 0 → historic budgets (1800 / 25000) and identity density → the same picture.
     It is a render budget and is not part of `InputHash` or the manifest.
   - Cluster extent: the size factor scales bounds only in the non-reuse branch; a reused dataset keeps
     `CanonicalDataset->ClusterBounds`. `ClusterToGalaxyScale` now divides by the type's table extent
     instead of the cluster's own extent. Every loadable dataset (Version 3, since 2026-09-11) stores
     exactly the table bounds — the table of the seven historic types is identical at `14aba261`
     (2026-09-11) and today — and `GetLogicalHalfExtent` mirrors `GenerateStarCluster`'s formula, so the
     two extents are equal and every root-space position of an old save is unchanged. Giant (factor 1.0)
     new worlds are unchanged too.
   - The preview formation budget and neighbour spacing only change what the menu draws; sealed records
     and gameplay are unaffected. The continuous-frame exclusion only hides points.
   - REGENERATE writes ordinary model values (only on a press); loading never rolls.

## 7. Phases

- **Phase 1 (done, this session):** enums, per-type tables, morphology engine, routing, population age,
  density compensation hook, parallel batch resolver, Colossal + five formations, tests, this doc.
- **Phase 2 status (03.10 03:35, cl-checked, not built):** done — REGENERATE sky rolls (§3.9), per-type
  CLASS + coercion + hint (§3.2), STARS slider/model field/budget plumbing/info/card (§3.1, cap 50k),
  cluster SIZE extent + live sample + CLUSTER framing (§3.6), Colossal preview budget, neighbour spacing
  (§3.7), continuous-frame home exclusion (§3.8), tests (§9). **Not done:** the bulk/GPU star layer
  (so no 1M yet) and galaxy star picking (pick grid, §5); the galaxy click path is unchanged.
- **Phase 2 plan (shared files; edit points as planned, kept for reference):**
  - `UI/MainMenu/SWorldGenerationPanel.cpp`
    - `EnumRow` (≈ lines 810–900) uses `GetSelectableEnumValues(Enum)` in both arrow lambdas
      (≈ 846, 882): add a variant taking a value provider; line ≈ 1945 (CLASS) →
      provider `APSGalaxyMorphology::GetSubclasses(W->GalaxyType)`, tooltip
      `APSGalaxyMorphology::GetSubclassSummary`.
    - line ≈ 1947 (MODELED STAR COUNT) → STARS slider bound to `GalaxyPlacedStarCount`
      (1,800…1,000,000, preferably log-mapped) with setter `SetGalaxyPlacedStarCount`.
    - cluster rows ≈ 1955–1958 need no change (new values appear after UHT); optional formation hint.
  - `UI/MainMenu/WorldGenerationViewModel.h/.cpp`
    - `SetEnumValue` (≈ line 131): when `EnumClass == StaticEnum<EGalaxyType>()`, set
      `GalaxyClass = APSGalaxyMorphology::CoerceSubclass(GalaxyType, GalaxyClass)` before
      `RequestPreview()` (≈ 212). Spiral + E4 then becomes SpiralSb — the same picture.
    - new `SetGalaxyPlacedStarCount(double)`; keep `SetGalaxyStarCount` (Blueprint
      `AstroGenerationMenu` still binds it).
    - info text ≈ 1723–1726 ("MODELED STARS / RENDERED SAMPLE") and summary ≈ 1835, card
      `WorldGenerationViewModelCard.cpp` ≈ 130–138, 303: show the placed count.
    - picking: the click path (≈ 1280 `SelectPreviewClusterSystemAtScreenPosition`, ≈ 1349/1380
      focus) routes to a galaxy pick when the preview focus is Galaxy.
  - `Core/Model/GeneratedWorld.h/.cpp`: `UPROPERTY(EditAnywhere, Category = "Galaxy") int32
    GalaxyPlacedStarCount{0};` (0 = historic budgets). Not in `InputHash`.
  - `Generation/AstroGenerator.h/.cpp`
    - member `GalaxyPlacedStarCount`, copied in `ApplyWorldModel` (after ≈ 9668).
    - `GenerateGalaxy` (≈ 11755–11761): if placed > 0, budget = clamp(placed, 1800, 1,000,000) (interim
      HISM-only cap ~250k until the bulk layer exists), pass `DensityReferenceBudget` = 1800 (preview) /
      25000 (gameplay) to `GenerateGalaxyOctreeStars` (≈ 11819); else unchanged.
    - `GenerateStarCluster` preview `FormationBudget` switch (≈ 9765–9774): add
      `case EStarClusterSize::Colossal: FormationBudget = 1800;` (otherwise it falls to the default 1000).
    - picking: `FindPreviewGalaxyStarAtScreenPosition` next to `FindPreviewClusterSystemAtScreenPosition`
      (≈ 9165) plus a selected-star summary like `GetSelectedPreviewClusterSystemSummary` (≈ 9102).
  - `Actors/Astro/Galaxy.*`: bulk ISM layer + pick grid (§4, §5).
- **Phase 3:** custom GPU point renderer with streaming LOD for 100M; procedural cluster records
  (store only edited records; regenerate the rest from the seeded formations) to shrink Colossal saves
  from ~200 MB to kilobytes — possible for the seeded formations, RingArc and Nebula; the historic
  global-RNG formations keep sealed records.

## 8. Risks

- Until a build containing phase 2 runs, the CLASS row cycles through all 47 selectable class values
  (historic + new) for every type — true in the editor DLL built at 02:09. A subclass offered for another
  type (e.g. cD on Spiral) resolves with the historic code (its arm count follows `2 + class % 4`), so
  nothing breaks, but the list is long and partly meaningless until then.
- Colossal saves ~110–220 MB; preview regeneration builds all records (~0.3 s at 100k, estimate).
- Phase 2: STARS at 50k costs ~10–14 ms per **moving** menu frame on the CPU (estimate; static frames
  free) — check `[APS.Perf]` while orbiting the galaxy at 50k. Giant clusters now draw 12k live systems
  instead of 1,600: the accepted default Giant Ring/Arc looks denser and costs more per moving frame;
  if Rio prefers the old density, lower `GetPreviewFormationBudget(Giant)` only.
- Neighbour spacing is presentation-only and one-sided (radius cap, no repositioning); at extreme
  densities stars become tiny points rather than overlap.
- The home exclusion is per frame in the menu only; gameplay relies on the generation-time HISM
  exclusion. Not yet seen on frames.
- Density-compensation exponents and new-profile parameters were tuned offline, not on engine frames;
  the PSF material clamps tiny stars to a few pixels, so dense views must be judged on real frames.
- HISM above ~100k stars is memory-heavy (§4); do not raise the HISM budget to 1M without the bulk layer.
- Bit-exactness under `/fp:fast` is only guaranteed within one binary; tests use a 1e-9 R tolerance.
- The editor DLL built at 02:09 predates the draw-sequencing fix (§3.3) in a few new profiles
  (cD/dE cores, Im/IBm/dIrr lobe centres, I0 core/outflow, tidal knots, polar-ring host) and in two new
  cluster formations (Young Association groups, Embedded hub). Worlds saved with those new values from
  that DLL may shift slightly after the next build; historic values are unaffected.

## 9. Test plan

Automation (new, `Tests/APSGalaxyGenerationV2Tests.cpp`):
- `APS.Generation.GalaxyV2.LegacyCatalogUnchanged` — frozen 02.10 `ResolveStar` vs live for all
  historic pairs; Sb/SBb/Pec-warped defaults equal the historic two-arm / warp output.
- `APS.Generation.GalaxyV2.SubclassTables` — per-type lists, defaults, coercion (Spiral + E4 → Sb),
  legacy Sa..SBd not offered, profiles only for appended values.
- `APS.Generation.GalaxyV2.SubclassesChangeShape` — every pair of subclasses of a type moves ≥ 20% of
  stars by > 2% R; logical trends (bulge Sa > Sbc > Sd, SBa > SBc > SBd; late types younger; E7 flatter
  than E0; cD concentrated; dE flat; ring galaxy hollow; polar ring out of plane); all inside 1.05 R.
- `APS.Generation.GalaxyV2.BatchResolveAndDensity` — parallel batch equals serial, nested windows,
  density compensation identity/limits.
- `APS.Generation.ClusterV2.NewFormations` — bounds, determinism vs global RNG, seed dependence,
  core density ordering, Colossal range > Giant, Giant unchanged.
- `APS.Generation.ClusterV2.LegacyFormationsUnchanged` — eight historic formations vs frozen copy.
- `APS.Generation.ClusterV2.SizeShowsInExtentAndLiveSample` (phase 2) — extent factors grow with size,
  Giant = 1.0, budgets grow with size, Tiny's budget covers its whole range, the logical half extent
  matches `GenerateStarCluster`'s globular envelope formula.
- `APS.Generation.GalaxyV2.RegenerateRollsGalaxyAndCluster` (phase 2) — 300 roll seeds: deterministic,
  class belongs to its type, cluster size Small..Giant, galaxy size 100–600, all 6 galaxy types and
  ≥ 10 cluster types incl. the new ones, archetype share plausible, PlanetOnly leaves galaxy/cluster.

Existing tests that must stay green: `APS.Gameplay.Generation.GalaxyCatalogVolume`,
`APS.UI.MainMenu.GrandDesignMorphology`, the canonical projection / continuous preview / handoff
smoke suites (they use Spiral/Giant/RingArc defaults).

Frames (Rio judges; phase 2, menu preview, galaxy focus, same camera per type, 1800 and 1M placed):
- Spiral: Sa, Sb (must equal today's Spiral + E4), Sc, Sd, Sm. Barred: SBa, SBb (= today's), SBc, SBm.
- Elliptical: E0 (= today's default), E7, cD, dE. Lenticular: S0 (= today's), S0/a, SB0.
- Irregular: Irr (= today's), Im, I0. Peculiar: warped (= today's), ring, interacting, tidal tails,
  polar ring.
- Clusters: Giant RingArc (= today's default), Colossal Super Star Cluster, Young Association, Double.
- Old save (e.g. a Giant RingArc save from 02.10) loads, `[APS.CanonicalDataset]` reports no reject and
  the galaxy/cluster frames match the pre-03.10 build.
- Perf: `[APS.Perf]` in the menu with 1M placed stars ≥ 110 fps; `[APS.GalaxyPreview]` now logs
  `class=` and the density factors.

What Rio should look at after the next build (phase 2):
1. GALAXY page: STARS slider 1,800 → 50,000 (log); the LIVE MODEL card and the info text show the same
   number; more stars = denser, dimmer per star, more volumetric. Orbit the camera at 50k and read
   `[APS.Perf]`.
2. TYPE change: Barred + E5 becomes SBb; CLASS arrows cycle only that type's list, with a hint line.
   Compare Sa/Sb/Sc/Sd/Sm, SBa/SBb/SBc/SBm, E0/E7/cD/dE, S0/S0a/SB0, Irr/Im/I0 and the Pec forms.
3. STAR CLUSTER page, CLUSTER focus: Tiny shows all its systems in a small volume; Giant 12k; Colossal
   20k and wider than the frame. GALAXY focus: the cluster's share of the galaxy follows SIZE (new worlds).
4. Colossal Supercluster close-up: no overlapping star spheres; log `[APS.Preview.Cluster] Neighbour
   spacing: ...`.
5. Edit planet count / orbits / star radius, then REGENERATE: `[APS.Preview.Exclusion] ...` changes and
   no catalogue star sits inside the home system.
6. REGENERATE a few times: the galaxy and cluster change too; the log line ends with
   `galaxy=... cluster=... sky=...`; the panel shows the rolled values.
