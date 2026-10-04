# Галактика, фаза 3: GPU-звёзды и свечение (APSStarRenderer)

Статус 2026-10-03: код написан в `Plugins/APSStarRenderer` (новые файлы, плагин **выключен**:
`"EnabledByDefault": false`, в `.uproject` его нет). UE/UBT не запускались, в движке ничего не собрано и не
показано. Что проверено: все 7 `.cpp` прошли синтаксическую проверку `cl /Zs` с заголовками и флагами UE 5.4
(PCH игрового модуля, только чтение), все 16 вариантов шейдеров компилируются `dxc` (SM6.6, HLSL 2018/2021) и
`fxc` (SM5) в макете окружения UE, а алгоритм свечения и фотометрия проверены numpy-портом (ниже, §9).
Ничего не закоммичено; папка `/Plugins/` целиком в `.gitignore` (§6, шаг 0).

## Кратко (для Rio)

- **Что это.** Отдельный плагин рисует звёзды галактики на видеокарте: (1) **точки** — десятки миллионов звёзд
  каталога, каждая 8 байт, вычислительным шейдером; планеты, корабли и станции их закрывают (проверка глубины),
  яркие звёзды дают блум; (2) **свечение** — светящийся «туман» из всех звёзд, которые не нарисованы точками,
  с тёмными пылевыми полосами; правильно и снаружи (меню, галактика целиком), и изнутри (полоса Млечного Пути
  в игре).
- **Звёзды те же самые.** Позиции, цвета и ID берутся из того же каталога (`ResolveStar` на процессоре), видеокарта
  только рисует. Клик, сохранения, StableId не меняются. Порядок тот же (вложенный порядок каталога), поэтому
  при загрузке галактика «проявляется» равномерно, а бюджет отрезает хвост — получается равномерная выборка.
- **Ожидаемые цифры (оценки, не замеры, RTX 5080, 1440p):** 10 млн точек ≈ 0,5–1,2 мс; 16 млн (бюджет по
  умолчанию) ≈ 1–2 мс; свечение ≈ 0,15–0,3 мс (половинное разрешение); сведение ≈ 0,2–0,3 мс. Память
  8 байт на звезду (10 млн = 80 МБ). Нынешний HISM-путь в меню тратит ~0,28 мкс процессора на точку в
  каждом кадре движения камеры — у GPU-пути этого нет (одна матрица на кадр).
- **Пока не включишь — ничего не меняется.** Плагин выключен; даже включённый, он не компилирует шейдеры без
  `aps.Stars.CompileShaders=1`; рисование выключено (`aps.Stars.GpuPoints 0`, `aps.Stars.GalaxyGlow 0`).
  Ошибка в шейдере при первом включении не уронит редактор повторно: защита запоминает неудачный старт.
- **Как посмотреть (после сборки, §8):** запустить редактор с
  `-dpcvars=aps.Stars.CompileShaders=1,r.AreShaderErrorsFatal=0`, в меню (PIE) открыть галактику и в консоли:
  `aps.Stars.GpuDebug 2000000` (тестовая спираль поверх галактики), `aps.Stars.GpuPoints 1`,
  `aps.Stars.GalaxyGlow 1`. Сравнение: `aps.Stars.GpuDebugView 1` — только точки, `2` — только свечение,
  `0` — всё; выключить — `aps.Stars.GpuPoints 0` / `aps.Stars.GalaxyGlow 0` (кадр сразу прежний).
  `stat gpu` — строка «APS Stars»; `aps.Stars.GpuStats 1` и `aps.Stars.GpuReport` — сколько точек проверено и
  нарисовано. Изнутри: `aps.Stars.GpuDebug 2000000 inside`.
- **Подключение к настоящей галактике** — после фазы 2 (меню и генератор сейчас правит другой агент): точные
  шаги в §6. До этого виден только тестовый режим `aps.Stars.GpuDebug`.
- **Что не проверено:** сборка, компиляция шейдеров самим UE, кадры, производительность. Известное ограничение
  свечения: тусклое гало высоко над диском изнутри примерно в 0,4 раза слабее точной оценки (§10).

## 1. Request and boundaries

Rio 03.10: "do phase 3 right away: GPU points and the glow" — tens of millions of individually placed stars at
120 fps, plus a volumetric glow for everything unresolved, so the galaxy population can later be realistic
(billions: stars exist only as the deterministic formula; the nearest/brightest are points, the rest glow).
Phase 2 (another agent, now) owns `Actors/Astro/Galaxy.*`, `Generation/GalaxyGenerator*`,
`Generation/StarClusterGenerator*`, `Generation/APSGalaxyMorphology*`, `Generation/AstroGenerator*`,
`UI/MainMenu/*`. This work only adds `Plugins/APSStarRenderer/**` and this document.

## 2. Design

### 2.1 Points come from the CPU catalogue (a GPU buffer), not GPU-side generation

Chosen: the caller resolves records with the existing `APSGalaxyCatalogBatch::ResolveStars` (parallel, ordinal
order) and packs each into 8 bytes (`APSStarRenderer::PackStar`: 16-bit position per axis inside the set's local
box, 8-bit palette colour, 8-bit log intensity). Why not port `ResolveStar` to HLSL:

1. Not bit-exact by construction. Positions use double `Pow/Sqrt/Sin/Cos`, a 12-try rejection sampler
   (`RandomUnitVector`) and float thresholds that select populations; a GPU float port flips rare branches and
   then every following stream draw shifts, i.e. a star lands somewhere else than picking/saves/StableIds say.
   SM6 has `uint64_t` (the SplitMix64 part ports exactly) but no double transcendentals.
2. Two copies of the morphology. `APSGalaxyMorphology` (phase 1, ~700 lines, 9 forms) is being edited by phase 2;
   a HLSL twin would drift. With the buffer the CPU stays the only truth.
3. The budget fits. 8 B/star: 10M = 80 MB, 50M = 400 MB on a 16 GB card; resolve ≈ 20–40 ms per 1M on 8+ cores
   (phase-1 estimate) → 10M ≈ 0.2–0.4 s on a background task. Uploads are spread over frames
   (`aps.Stars.GpuUploadPointsPerFrame`, 1M = 8 MB per frame), so a set fills in progressively in catalogue
   order — always a uniform sample.

Quantisation: 16 bits over ±1.05 R → 3.2e-5 R per step (≈1/40 px with the galaxy across 1500 px). Near views
(inside the galaxy) should use smaller sets (e.g. octree cells) — each set has its own box, so the step shrinks
with the cell. The nearest few thousand stars stay on the existing HISM/glyph path; the GPU set should cover the
ordinals after the HISM prefix (no double drawing).

### 2.2 Compute rasterizer

Per view, after TSR (§2.3), at the output resolution:

1. `ClearCS` — a transient per-pixel key buffer (`R64_UINT` with `Atomic64Compatible`, as Nanite's VisBuffer64;
   or `R32_UINT`).
2. `RasterCS` per set (grid-stride loop, 64 threads, ≤ 8192 groups): decode → relative to the observer → render
   position (with the APOSFERA far envelope, §2.4) → clip with the un-jittered projection → frustum → depth test
   against the scene depth (reverse Z; planets, ships, stations in front hide the star) → brightness
   `I * IntensityScale / d_local^2 / Ω_px * PreExposure` → **LOD cut**: below `aps.Stars.GpuPointMinPixel` the
   point is skipped (its light belongs to the glow) → atomic max of the key:
   - 64-bit (`ImageInterlockedMaxUInt64`, SM6.6, `NaniteAtomicsSupported()`): high = float bits of the brightness,
     low = 8-bit sub-pixel x/y + colour index;
   - 32-bit fallback (`InterlockedMax`): 16-bit log brightness (1/1024 stop) + 4-bit sub-pixel x/y + colour.
   The key keeps the brightest star of each pixel *and its sub-pixel position*.
3. `CompositeCS` gathers each pixel's neighbours (radius 2) and spreads every stored star with a pixel-integrated
   PSF (core σ 0.55 px; a halo 0.9→1.6 px with up to 25% of the light for bright stars). Because the PSF is
   integrated over the pixel at the true sub-pixel centre, a slowly moving star keeps its total light (no crawl).
   Writes a **new** texture: the TSR output is also its history and must not change.
   Rio 03.10 08:17: this was a pixel shader drawn with `AddDrawScreenPass`. It read only `SV_POSITION`, but
   `FScreenPassVS` outputs `TEXCOORD0` first, so D3D12 rejected the PSO ("Semantic 'SV_Position' is defined for
   mismatched hardware registers"; E_INVALIDARG). UE treats a PSO failure at draw time as fatal, so the run
   crashed. The composite is now a compute pass with a typed UAV store, and the plugin creates no graphics PSO. Every
   compute PSO is created once with `RHICreateComputePipelineState` before its first dispatch; that call returns
   null on failure instead of hitting the fatal path. A failure turns the feature off and the frame passes
   through unchanged.

Budget: `aps.Stars.GpuPointBudget` points tested per view (priority order, then catalogue prefix).
Debug: `RDG_EVENT_SCOPE "APS.Stars"`, GPU stat "APS Stars" (`stat gpu`), counters via read-back.

### 2.3 Where in the frame: after motion blur (post-TSR, before bloom)

`SubscribeToPostProcessingPass(MotionBlur)`. In UE 5.4 that callback runs after TSR and before the bloom/eye
adaptation chain, and the engine regenerates the half/quarter-resolution scene colour after it
(`PostProcessing.cpp`: "Invalidate half and quarter res"), so stars bloom and feed exposure. Rejected
alternatives: after the base pass (TSR would smear points that write no depth/velocity during the menu orbit),
`PrePostProcessPass` (both drawbacks). Consequence: translucent atmosphere shells do not dim the points or the
glow — exactly like today's catalogue points, daylight is handled by the game:
`SetWorldVisibility(World, APSGameplayStellarDay::PointVisibility(GameplayDaylightFactor))`.
Opaque geometry occludes through the depth test; scene captures, reflection captures and orthographic views are
skipped (`bDrawInSceneCaptures` per set to opt in).

### 2.4 APOSFERA far envelope

The menu and gameplay place stars with `FAPSContinuousPreviewFrame::ProjectSphere`:
`render = (P - Observer) * S / (1 + S |P - Observer| / Far)` — directions exact, distances compressed into 1e9 cm.
An affine transform cannot express that, and the HISM path re-projects every point on the CPU per moving frame.
The API takes `FFarEnvelope {ObserverPhysical, ObserverRender, RenderPerPhysical, FarEnvelope}`; the raster shader
applies the same formula per point (`ApsPresentPosition`), the glow undoes it for the depth limit. One
`SetWorldFarEnvelope` call per presentation frame replaces the per-point CPU work.

### 2.5 Glow: a 2.5D map built from the same catalogue

`FGlowMapBuilder` takes catalogue samples (the same records as the points, any type/subclass) and builds a
square map in the galaxy plane (256² default):

- **Light:** star density (σ 1 texel) × smoothed light per star (σ 3 texels; the light is dominated by rare O/B
  stars). Normalised so the map integrates to the population's light.
- **Dust tracer:** per-star weight from the caller (young/hot stars ≈ 1), opacity per volume (`DustOpacity` =
  face-on optical depth; 0 for E/S0).
- **Vertical profile per column**, two components: a *thin* one (disk; shape −1 exponential … 0 Gaussian … +1 flat
  slab, fitted from E|u|/σ — equal-variance mixtures, so the fit is exact-linear) and a *thick* Gaussian one
  (bulge/halo stars beyond 2.5 robust spreads). Statistics are computed per texel without blurring raw moments,
  then smoothed in log space weighted by star counts; sparse texels fall back to a wide field.
- **Ray march** (`RaymarchCS`, half resolution, 48 steps, no temporal jitter): box test in map space, scene depth
  limit (with the far envelope undone), and per step the **exact vertical integral** of each profile
  (CDF differences), so thin disks and dust lanes cannot alias between steps. Emission–absorption: emission
  spread through each step's dust `(1-e^-τ)/τ`. Output: RGB light (pre-exposed) and transmittance T.
- **Composite:** `scene * lerp(1, mean T) + points * lerp(1, mean T) + glow`, where mean T = (1−T)/τ is the
  expected transmittance of a source uniformly inside the dusty column (`aps.Stars.GalaxyGlowDustOnScene` /
  `...DustOnPoints`).
- One glow volume per scene is drawn (the first enabled).

### 2.6 Photometry (points and glow share units)

Point pixel value = `DecodeIntensity(code) * IntensityScale / (distance in the set's LOCAL units)^2 / Ω_px`
(Ω_px = solid angle of the centre pixel, so a star does not brighten towards the frame edge — the HISM convention).
Glow radiance = `TotalIntensity * EmissionNorm / ExtentXY^2 * ∫ profile ds` in the same local units. With
`TotalIntensity = (Population − drawn points) * Map.MeanIntensity * IntensityScale`, points + glow conserve the
light of the population at any slider value (checked numerically, §9). Both are multiplied by `View.PreExposure`.

## 3. API (`Plugins/APSStarRenderer/Source/APSStarRenderer/Public/APSStarRendererAPI.h`)

Plain C++, no UCLASS/USTRUCT; every call is safe from any thread (marshalled to the game thread, then to the
render thread with `ENQUEUE_RENDER_COMMAND`, in order).

| Function | Purpose |
|---|---|
| `PackStar / UnpackStarPosition / EncodeIntensity / DecodeIntensity` | 8-byte star format |
| `RegisterPointSet(World, FPointSetDesc, TArray<FPackedStar>&&)` → handle | points moved in, uploaded progressively |
| `UpdatePointSet(handle, desc)` | transform, envelope, intensity, visibility, exclusion sphere, priority, enabled |
| `RegisterGlowVolume(World, FGlowVolumeDesc, FGlowMap&&)` / `UpdateGlowVolume` | glow volume |
| `SetTransform / SetFarEnvelope / SetVisibility / SetEnabled / Remove(handle)` | per set or volume |
| `RemoveAll(World)` (also automatic on world clean-up), `SetWorldFarEnvelope`, `SetWorldVisibility` | per world |
| `SetColorPalette(256 colours)`, `GetDefaultPaletteIndex(Kelvin)`, `GetDefaultPaletteColor` | colours (default: black body 1500–40000 K) |
| `FGlowMapBuilder(res, extent).AddStar / Merge / Build(FGlowMap&)` | glow input, background-thread friendly |
| `GetStats()`, `AreShadersEnabled()` | status |

`FPointSetDesc`: `LocalToWorld` (uniform scale), `FarEnvelope`, `LocalBounds` (packing box), `IntensityScale`,
`Visibility`, `ExclusionCenterLocal/RadiusLocal` (home-system exclusion like the HISM), `Priority`, `Population`,
`bEnabled`, `bDrawInSceneCaptures`, `DebugName`. Max 64M points per set (split bigger catalogues).
`FGlowVolumeDesc`: `LocalToWorld`, `FarEnvelope`, `TotalIntensity`, `DustOpacity`, `DustHeightScale/Max`,
`Visibility`, `bEnabled`, `bDrawInSceneCaptures`.

## 4. Console variables and commands

| Name | Default | Meaning |
|---|---|---|
| `aps.Stars.CompileShaders` | 0 | read-only, start-up: 1 lets the plugin's global shaders into the shader map |
| `aps.Stars.GpuPoints` | 0 | draw point sets |
| `aps.Stars.GpuPointBudget` | 16 777 216 | points tested per view and frame |
| `aps.Stars.GpuPointIntensity` | 1 | global point brightness (calibration against HISM) |
| `aps.Stars.GpuPointMinPixel` / `MaxPixel` | 0.002 / 64 | LOD cut / clamp (pre-exposed pixel value) |
| `aps.Stars.GpuPointCoreSigma` / `Halo` / `PsfRadius` | 0.55 / 0.25 / 2 | PSF |
| `aps.Stars.GpuPointAtomic64` | 1 | 64-bit path when supported, 0 forces 32-bit |
| `aps.Stars.GpuUploadPointsPerFrame` | 1 048 576 | upload chunk per set and frame |
| `aps.Stars.GalaxyGlow` | 0 | draw the glow volume |
| `aps.Stars.GalaxyGlowIntensity` / `Resolution` / `Steps` | 1 / 2 / 48 | glow brightness, divisor (2 half, 4 quarter), steps |
| `aps.Stars.GalaxyGlowDustOnScene` / `DustOnPoints` | 1 / 1 | how much glow dust dims the scene / points |
| `aps.Stars.GpuStats` | 0 | GPU counters read-back |
| `aps.Stars.GpuDebugView` | 0 | 1 points only, 2 glow only, 3 transmittance |
| `aps.Stars.GpuDebug [count] [galaxy\|front\|inside] [glowShare]` | — | synthetic two-arm spiral (points + glow); `0` removes |
| `aps.Stars.GpuReport` | — | logs sets, resident/pending points, points per frame, GPU counters |

## 5. Safety

- **Plugin disabled** until the first coordinated build (§6 step 1): nobody's build can break on it.
- **Start-up never depends on the shaders.** UE 5.4 makes a global shader compile error fatal
  (`r.AreShaderErrorsFatal`, `ProcessCompiledGlobalShaders`). Every plugin shader's `ShouldCompilePermutation`
  returns false unless `aps.Stars.CompileShaders=1` (read lazily after the ini files). With it on, a **crash
  guard** writes `Saved/APSStarRenderer/ShaderCompileAttempt.txt` (source hash + pid) before the compile and
  deletes it at `OnPostEngineInit`; if a start with the same sources died meanwhile (pid gone), the next start
  keeps the shaders off and logs how to retry. Commandlets skip the guard.
- **Runtime:** shaders are looked up without asserting (`FGlobalShaderMap::GetShader(type, id).IsValid()`); any
  missing one → the frame passes through unchanged and the extension deactivates itself
  (`GShadersMissingAtRuntime`). 64-bit atomics are used only with `NaniteAtomicsSupported()` and
  `bSupportsUInt64ImageAtomics`; otherwise the 32-bit path. SM5-only and mobile platforms never compile them.
- **No cost when off:** `IsActiveThisFrame` is false unless a cvar is on, shaders exist and the view's scene has
  an enabled set/volume — then no callback is subscribed at all.
- **Module:** `PostConfigInit` only maps `/Plugin/APSStarRenderer` (guarded: directory exists, mapping not
  present); the view extension is created at `OnPostEngineInit`; GPU resources are released at
  `OnEnginePreExit` (render command + flush). No UObjects, no UHT.

## 6. Integration after phase 2 (exact steps)

0. **Git:** `.gitignore` has `/Plugins/` (the whole folder, also AtmoScape, is untracked). To commit the plugin:
   replace `/Plugins/` with `/Plugins/*` and add `!/Plugins/APSStarRenderer/` (Binaries/Intermediate stay ignored
   by the existing patterns). `.gitignore` is currently modified by someone else — coordinate.
1. **Enable** (one coordinated build): `APS_ALPHA.uproject` → `"Plugins": [..., {"Name": "APSStarRenderer",
   "Enabled": true}]` (or flip `EnabledByDefault`). Build `APS_ALPHAEditor`.
2. **Game module dependency:** `Source/APS_ALPHA/APS_ALPHA.Build.cs` → `PrivateDependencyModuleNames.Add("APSStarRenderer");`.
3. **Shaders on:** `Config/DefaultEngine.ini` `[SystemSettings]` → `aps.Stars.CompileShaders=1` (after the first
   verified start, §8). Keep `aps.Stars.GpuPoints=0` / `aps.Stars.GalaxyGlow=0` until Rio's OK.
4. **Palette** (once, e.g. in the new galaxy layer code): index = `SpectralClass * 10 + Subclass`, colour =
   `UStarGenerator::GetStarColor(class, subclass)` → `APSStarRenderer::SetColorPalette(...)`.
5. **`Actors/Astro/Galaxy.h/.cpp`** (phase-2 owner): members `APSStarRenderer::FHandle GpuPointSet{0}, GpuGlowVolume{0}`;
   `void RebuildGpuStarLayer(int32 FirstOrdinal, int32 PlacedCount)`; `void ReleaseGpuStarLayer()` (calls
   `Remove`), called from `EndPlay`/`BeginDestroy` and before every regeneration. `RebuildGpuStarLayer` on a
   background task:
   ```cpp
   const FGalaxyCatalogDescriptor Catalog = StarCatalog;       // copy for the task
   const auto Order = APSCanonicalStellarProjection::MakeNestedCatalogPermutation(Catalog.GenerationSeed, Catalog.ModeledStarCount);
   const FBox3f Bounds(FVector3f(-Catalog.CatalogHalfExtent), FVector3f(Catalog.CatalogHalfExtent));  // canonical units
   APSStarRenderer::FGlowMapBuilder Glow(256, Bounds.Max.X);
   TArray<APSStarRenderer::FPackedStar> Points;  TArray<FGalaxyCatalogStarRecord> Chunk;
   for (int32 Start = FirstOrdinal; Start < PlacedCount; Start += 65536)
   {
       APSGalaxyCatalogBatch::ResolveStars(Catalog, Order, Start, FMath::Min(65536, PlacedCount - Start), Chunk);
       for (const FGalaxyCatalogStarRecord& R : Chunk)
       {
           if (R.CatalogIndex == INDEX_NONE) continue;
           const FVector3f P(R.GalaxyLocalLocation);
           const uint8 Color = uint8(int32(R.SpectralClass) * 10 + R.SpectralSubclass);
           const float I = StarIntensity(R);   // the HISM emission / a reference, see calibration below
           Points.Add(APSStarRenderer::PackStar(P, Bounds, Color, I));
           Glow.AddStar(P, Palette[Color], I, IsHot(R.SpectralClass) ? 1.0f : 0.3f);
       }
   }
   // If PlacedCount is small, keep adding glow samples from ordinals beyond it (1-4M samples give a smooth map).
   ```
   then on the game thread: `RegisterPointSet(GetWorld(), Desc, MoveTemp(Points))` and, with `Glow.Build(Map)`,
   `RegisterGlowVolume(GetWorld(), GlowDesc, MoveTemp(Map))` with
   `GlowDesc.TotalIntensity = (ModeledStarCount - PlacedCount) * Map.MeanIntensity * IntensityScale` and
   `DustOpacity` by type (E/S0 0, Sa ~0.5, Sb/Sc ~1–1.5, Irr ~0.5). `FirstOrdinal` = the HISM budget (1800 menu /
   25000 gameplay) so the HISM stars are not drawn twice.
6. **Transforms** — `LocalToWorld` maps canonical units to the frame's "physical" cm, exactly as the preview
   points do (`AstroGeneratorPreviewFrame.cpp` ≈561: `CenterCm = Frame.GetCanonicalRootPositionCm(local) -
   Frame.CanonicalAnchorCm`):
   `FTransform(FQuat::Identity, Frame.LayerOriginCanonicalUnits * Frame.CanonicalCmPerUnit - Frame.CanonicalAnchorCm,
   FVector(Frame.LayerToRootPositionScale * Frame.CanonicalCmPerUnit))` with `Frame = Galaxy->CanonicalProjectionFrame`.
   Per presentation frame, in `AAstroGenerator::ApplyContinuousPreviewFrame()` (`AstroGeneratorPreviewFrame.cpp`
   ≈904, after `ContinuousPreviewFrame` is updated): `APSStarRenderer::SetWorldFarEnvelope(GetWorld(), {true,
   ContinuousPreviewFrame.ObserverCm, PreviewCamera->GetComponentLocation(), ContinuousPreviewFrame.RenderCmPerPhysicalCm,
   ContinuousPreviewFrame.FarEnvelopeCm})`; the home-system exclusion (`HomeExclusionCenterCm/RadiusCm`, same
   function) → `ExclusionCenterLocal/RadiusLocal` (divide by the local scale). Gameplay: the same with the gameplay
   stellar frame (`APSGameplayStellarProjection.h`, `FarEnvelopeCm = 1e9`); without an envelope set
   `LocalToWorld` to the world transform and push it on world-origin shifts.
7. **Daylight:** at the end of `UAPSStellarVisualSubsystem::UpdateGameplayDaylightStars`
   (`Core/Rendering/APSGameplayStellarView.cpp` ≈240): `APSStarRenderer::SetWorldVisibility(GetWorld(),
   APSGameplayStellarDay::PointVisibility(GameplayDaylightFactor))`.
8. **Menu slider / bulk layer:** with the GPU layer on, the phase-2 bulk ISM layer for [HistoricBudget, N) can stay
   as the fallback (cvar off) — keep both behind `aps.Stars.GpuPoints` for A/B. Picking does not change (the
   CPU grid uses the same records; GPU index i = ordinal FirstOrdinal + i).
9. **Calibration:** draw the HISM prefix also on the GPU once (`FirstOrdinal = 0`, debug) and match brightness
   with `IntensityScale` / `aps.Stars.GpuPointIntensity`; the HISM look follows the same inverse-square law
   (`GetFarStarVisualEmission` keeps flux per star), so one factor should do.

## 6b. Gameplay sky (Rio 03.10: "turn it on by default, and in the gameplay sky too")

- Hook: `UAPSStellarVisualSubsystem::UpdateGameplayStellarView`, consumed full-scale branch, one call
  `APSGalaxyGpuStars::PresentGameplayFrame(World, Home, Attached)`; kill switch `aps.Stars.GameplayGpu` (1),
  points `aps.Galaxy.GameplayGpuStars` (4 000 000; the menu keeps `aps.Galaxy.GpuStars` 8 000 000).
- Mapping: the sky draws the galaxy HISM in 3D under the generator root (scale 1/PositionScale, at the home system);
  its centres are `GalaxyFrame.ProjectCanonicalUnits(local)`. The GPU layer uses
  `FTransform(I, ProjectCanonicalUnits(0), EquivalentComponentScale) x StarMeshInstances->GetComponentTransform()`,
  read from the component every frame and at `APSWorldShiftEvents::BindPostShift`. No far envelope.
  Each build logs a self-check: 256 ISM centres re-derived through the frame against the HISM base transforms.
- Precision: 6 point sets, nested cubes around the home system (half size halves per level). The 16-bit
  quantisation stays below 3e-5 of the distance (0.1 px).
- Exclusions: the home system and up to 7 nearest other `AStarSystem` actors (`StarSystemRadius` x 1.10). The
  plugin takes up to `MaxExclusionSpheres` = 8 per set.
- Photometry: the gameplay ISM glyphs keep their brightness at any distance and rank stars by
  `GetLuminosityGain` (L^0.12, 0.5-1.2). GPU points use that ranking and a brightness floor of one galaxy radius
  (`BrightnessFloorDistanceLocal`: I / max(d, floor)^2), and the glow uses the same floor in its ray march
  (`min(1, t^2/floor^2)`). Lambda shares as in the menu: at the overlay a GPU point equals its ISM glyph at
  `GameplayOverlayPixelValue` (2.0, to calibrate); a star after the prefix gets about 1 % of that (25 000 ISM, 4 M GPU).
- Daylight: `SetWorldVisibility` from `UpdateGameplayDaylightStars` (PointVisibility, or the switch-off with
  `aps.Stars.DayFade 0`). At visibility 0 the passes are skipped. Occlusion: scene depth (reverse Z) for points
  and glow.
- Elliptical profile: historic E0-E7 are uniform-density ellipsoids in the catalogue (`Galaxy.cpp` ResolveStar,
  r = U^(1/3) R; `APSGalaxyMorphology.cpp`: "E0..E7 stay historic"). Points and glow follow the catalogue, so
  an E0 looks evenly bright (projected density ~ sqrt(1 - R^2/R0^2)). A centrally concentrated E needs a
  catalogue change: new appended classes with a Spheroid profile (as cD), keeping E0-E7 for old saves.

## 7. Expected costs (estimates, RTX 5080, 1440p; to be measured)

| Part | Cost |
|---|---|
| Raster, per 10M points tested | 80 MB read, ≈0.3–0.6 ms; atomics add ≈0.2–0.6 ms when most points are visible |
| Clear (64-bit buffer 1440p, 30 MB) | ≈0.03 ms |
| Composite CS (25 taps, mostly empty) | ≈0.2–0.3 ms (4K ≈0.5–0.8 ms) |
| Glow half-res, 48 steps (0.9M rays) | ≈0.15–0.3 ms; quarter res ≈0.05–0.1 ms |
| Bloom chain re-downsample (engine) | ≈0.05 ms |
| GPU memory | 8 B/star (10M = 80 MB, 50M = 400 MB) + transient 30 MB (key buffer) + 7 MB (glow) + 1.5 MB maps |
| CPU | resolve+pack 10M ≈0.2–0.5 s on a task; glow build (1–4M samples, 256²) ≈0.1–0.3 s; upload 8 MB/frame |

## 8. First build and run — what must be checked

1. Build (after §6 steps 1–2): the 7 plugin files compile and link in `APS_ALPHAEditor` (no unresolved externals;
   UBT uses the plugin's own PCH, the syntax check here used the game module's).
2. Editor start **without** `aps.Stars.CompileShaders`: log `[APS.GpuStars] shaders not compiled ...` and
   `[APS.GpuStars] ready: shaders off`; nothing else changes (compare a menu frame and `stat unit`).
3. Start **with** `-dpcvars=aps.Stars.CompileShaders=1,r.AreShaderErrorsFatal=0` (first time only with
   `r.AreShaderErrorsFatal=0`): log `compiling the GPU star shaders (sources ...)`; no
   `GlobalShaderCompileError` lines mentioning `APSStar`/`APSGalaxyGlow`; `ready: shaders compiled`;
   `Saved/APSStarRenderer/ShaderCompileAttempt.txt` is gone after start-up. If there are errors: send the log,
   the frame stays unchanged and the next start (same sources) keeps the shaders off.
4. Menu (PIE), galaxy focus: `aps.Stars.GpuDebug 2000000` → log `debug galaxy: 2000000 points ...`;
   `aps.Stars.GpuPoints 1` → points over the galaxy; `aps.Stars.GalaxyGlow 1` → glow; `aps.Stars.GpuDebugView 1/2/3`;
   orbit the camera (no smearing, no flicker of slow stars); `stat gpu` "APS Stars"; `aps.Stars.GpuStats 1` +
   `aps.Stars.GpuReport` (written > 0, resident = 2000000 after ~2 frames); `stat unit` ≥ 110 fps; repeat with
   `aps.Stars.GpuDebug 10000000` and `aps.Stars.GpuPointAtomic64 0` (same picture).
5. Occlusion: a body/preview mesh in front hides points and glow (depth test).
6. Inside: `aps.Stars.GpuDebug 2000000 inside` → band across the sky with a dark lane; fly/rotate.
7. Robustness: window resize, `r.ScreenPercentage 50/100`, motion blur on/off, PIE stop/start (log `removed N
   sets/volumes of world ...`), editor shutdown without errors.

## 9. Verification done here (no engine)

- `cl /Zs` (MSVC 14.50, UE 5.4 headers, `/W4 /we4456 /we4458 /we4459`, C++20) on all 7 `.cpp`: pass. Engine
  symbols used are exported (`RENDERER_API`/`RENDERCORE_API`/`RHI_API`/`ENGINE_API`) or inline — checked by hand.
- `dxc` (SM6.6; HLSL 2018 and 2021) and `fxc` (SM5) on every entry point and atomics permutation, with a mock of
  the few `Common.ush` pieces used (`View` members, `ConvertFromDeviceZ`, `UlongType` helpers copied from
  `D3DCommon.ush`): pass. Names were checked against the `Common.ush` include closure: no collisions.
- numpy port of builder + ray march + point raster + composite (1M stars of the debug spiral):
  `F:/ChatGPT/APOSFERA/work/galaxy_gpu_stars_20261003/` (`glow_points_prototype.png`, `glow_proto.py`,
  `inside_flux_test.py`). Map integral 0.999–1.001. Flux glow/points with the same population: face-on 1.05,
  tilted 1.04, edge-on 1.01 (the ~5% is the 50° FOV pixel-size effect of the test); from inside the disk, in-plane
  1.05–1.18 (historic spiral) and 0.99–1.01 (smooth exponential disk); the halo seen perpendicular to the disk
  0.4×. Not engine frames.

## 10. Risks and limitations

- Nothing was built or run; shader compile inside UE (its preprocessor, generated uniform buffers, parameter
  binding validation) is only approximated by the mock.
- Translucent atmosphere shells do not dim GPU stars/glow (post-TSR composite): needs `SetWorldVisibility`
  (same model as today's catalogue points).
- Brightest-per-pixel: several stars in one pixel keep only the brightest (dense cores lose some point light; the
  glow carries the bulk). Budget/LOD-culled points are not added to the glow (small: light is dominated by rare
  O/B stars).
- Glow: one volume per scene; 2.5D (good for disks, approximate for strongly 3D shapes such as tidal tails or
  polar rings); faint high-latitude halo ≈0.4× from inside; uniform scale only; edge-on arms at grazing angles
  are sampled every ~0.04 map units.
- Wide FOV: points follow the HISM convention (no brightening towards edges), the glow is physical per solid
  angle — at 100°+ FOV the glow is relatively brighter at the frame edges.
- 16-bit positions: near views need smaller sets (cells) for sub-pixel accuracy.
- Composite runs at output resolution; very large resolutions (8K captures) raise the PSF cost linearly.
- The plugin directory is git-ignored (§6 step 0).

## 11. Files

`Plugins/APSStarRenderer/`: `APSStarRenderer.uplugin`; `Shaders/Private/APSStarCommon.ush`, `APSStarPoints.usf`,
`APSStarComposite.usf`, `APSGalaxyGlow.usf`; `Source/APSStarRenderer/APSStarRenderer.Build.cs`;
`Public/APSStarRendererAPI.h`, `Public/APSGalaxyGlowBuilder.h`; `Private/APSStarRendererPrivate.h`,
`APSStarSettings.cpp`, `APSStarShaders.h/.cpp`, `APSStarRegistry.h/.cpp`, `APSStarViewExtension.h/.cpp`,
`APSGalaxyGlowBuilder.cpp`, `APSStarRendererModule.cpp`, `APSStarDebug.cpp`.
