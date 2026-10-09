# Брифы на 3D-объекты: три происхождения и лестница

Claude, 2026-10-07. Третий документ набора: [концепт](GAME_CONCEPT_ORIGIN_2026-10-07.md) → [контент-план](CONTENT_PLAN_ORIGINS_2026-10.md)
→ **брифы**. Основа — [опись ассетов](../Audit/2026-10-07-asset-inventory.md) и [заметки о качестве](../Audit/2026-10-07-asset-quality-notes.md).

Пайплайн Rio (07.10): описания отсюда → Codex рисует концепт-картинки → Hunyuan3D генерирует меш → Codex доводит в
Blender (ретопология, UV, PBR, интерьеры, коллизии) → импорт в UE с OK Rio. Это не промты, а концептуальные
описания; промты для картинок Codex пишет сам. В каждом брифе есть короткое визуальное описание на английском —
его можно класть в промт как есть.

## 0. Общие правила

### 0.1 Единый язык формы и материала — «APS Industrial»

Эталон принят Rio 06.10 на грузовике `S_P3_01`: «clean manufactured hull and cellular radiator grilles… do not
redesign». Все новые объекты людей (колония, постройки, станции, верфь, техника) говорят на этом языке:

- **Панели** белой/светло-серой эмали с мягкими фасками, швы редкие и честные (как у промышленной техники, не
  «грибл»), крепёж мелкий и точечный.
- **Акцент** — один: оранжевый (сигнальные полосы, рамы люков, поручни, съёмные панели). Никаких второго и третьего
  цветов. Эмиссия: тёплый белый свет полосами и в окнах, редкие оранжевые маркеры. Без неона и без teal у людей.
- **Стыки и силовые части** — тёмная сталь/графит: рамы, опоры, шарниры, радиаторы (сотовые решётки), баки.
- **Стекло** — большие остеклённые плоскости в плотных рамах, тёмно-синее снаружи; в генерации рисуется
  непрозрачным, стекло ставится в Blender.
- **Состояния** одним и тем же мешем: «стройка» (каркас/матовый), «живой» (эмиссия, свет), «мёртвый» (эмиссия 0,
  пыль, иней/нагар на выбор) — через параметры материала, без дублей геометрии.
- **Читаемость**: у каждого объекта один узнаваемый силуэт и один «огонь», чтобы его было видно с 5–50 км
  (постройки на орбите), с 500 м (поверхность) и в упор на 0,5–1 м (Rio смотрит крупный план).
- **Масштаб честный**: люди 2 м, двери 2,2 × 1,0 м, перила 1,1 м, ступени 0,18 м. Техника и корабли должны
  соотноситься с модулями по размеру.

**Для всего, что в космосе** (туманности, кольца гигантов, астероиды, планеты с орбиты, звёзды) визуальный
ориентир — снимки NASA, см. [VISUAL_NORTH_STAR](VISUAL_NORTH_STAR.md) (Rio 08.10): правда, а не «космос из игр».
Объекты людей и Строителей — по языкам ниже.

**Два других языка**, которые нельзя смешивать с APS Industrial:
- **Строители (Древние):** чёрный гранёный камень, тонкие тил-белые светящиеся швы-глифы, ступенчатые массы,
  наклоны 1,5–5°, ничего круглого, кроме колец; никакой техники и труб.
- **Пропавшая экспедиция (предтеча ADRIFT):** тот же APS Industrial, но постаревший: серая выцветшая эмаль, нагар у
  двигателей, вмятины, пыль, редкие аварийные красные лампы. Это «мы же, но раньше».

### 0.2 Правила картинок для генерации (для Codex)

- Один объект на картинку, ракурс три четверти спереди-слева, чуть сверху; нейтральный серый фон; ровный студийный
  свет; без людей, текста, логотипов, земли и неба. Для масштаба — в брифе цифры, не фигурки.
- Стекло непрозрачное тёмное; тонкие тросы, антенны-иглы, сетки, провода не рисовать (генератор их теряет),
  добавляются в Blender.
- Все части соединены, ничего не висит в воздухе; симметрия там, где она естественна.
- Для китов: сначала картинка **ядра** модуля, потом каждая навеска отдельной картинкой с тем же ядром в кадре
  (для консистентности), генерировать навески отдельными объектами.
- Интерьеры картинками не генерировать: Hunyuan делает оболочки. Интерьеры собираются в Blender из интерьерного
  кита (§4.1) по брифу.
- На одну модель — 2–3 варианта картинки, Rio выбирает (версии хранить рядом, не перезаписывать).

### 0.3 Доводка в Blender (для Codex)

- Масштаб по брифу в метрах (генератор отдаёт нормализованные ~0,6 м — как Pack_1); пивот в основании (наземное) или
  в центре стыковочной оси (орбитальное); ось Z вверх, X — «перед».
- Ретопология до бюджета (в брифе), Nanite для всего ≥ 50k tris; LOD1/LOD2 для рассыпки и китов.
- UV: атлас 2k на модуль, 4k на станцию; PBR по домашнему стилю; эмиссия отдельной маской.
- Коллизия: UCX по бюджету (модуль 1–3, постройка 3–8, станция ≤ 300, корабль-эталон 208); проходы — капсула 2,05 м,
  кольца по контуру, отчёт clearance.
- Сокеты (Empty): `SOCK_Dock_N`, `SOCK_Door`, `SOCK_Light_N`, `SOCK_Vehicle` — по брифу.
- Выход: `F:\Rio\3D\<Kit>\out\<Asset>_vN\` — FBX, текстуры, `<Asset>_Spec.json` (габариты, сокеты, tris, UCX),
  превью Solid+Texture с 0,5–1 м и с 20 м, README с версией. Файлы >100 МБ — LFS. Импорт в UE только с OK Rio, новая
  версия рядом со старой.
- Имена: `SM_APS_<Kit>_<Part>` (`SM_APS_Colony_Habitat`, `SM_APS_Orbital_Core`, `SM_APS_Station_Hub`), BP —
  `BP_APS_<Kit>_<Part>`, папка `Content/APS/APS_ALPHA/Assets/<Kit>/`.

### 0.4 Приёмка

Кадры с трёх дистанций → clearance 2,05 м → бюджет tris/UCX → Nanite/LOD → материалы в стиле → в git без ссылок на
игнорируемые паки → перф-A/B против базы, если объект в первом часе → Rio смотрит в игре.

### 0.5 Что именно выдаёт Codex (формат, один запрос на всё)

Список всех объектов в машиночитаемом виде — [`asset_briefs_manifest.json`](asset_briefs_manifest.json) рядом с
этим документом: id, имя, партия, габариты, стиль, визуальное описание (EN), сколько и каких картинок нужно.
Запрос Codex один: «пройти по манифесту и выдать архив по правилам ниже».

**Раунд 1 — варианты (всё за один запрос).**
- Папка `F:\ChatGPT\APOSFERA\work\asset_concepts_<дата>\` и она же архивом `APS_AssetConcepts_<дата>.zip`.
- Структура: `<batch>/<id>_<slug>/v1.png`, `v2.png`, `v3.png` — три варианта одного объекта, ракурс ¾
  спереди-слева, чуть сверху. Для объектов с пометкой `reference` (интерьеры, язык стиля) — 3–4 mood-картинки без
  требований к ракурсу.
- Картинка: PNG 1536 × 1536 (минимум 1024), ровный серый фон `#808080`, объект по центру, занимает ~80 % кадра,
  студийный свет, без текста, людей, земли, неба, подписи масштаба. Стекло непрозрачное тёмное. Для китов (C-*,
  O-*, ST-*) — у всех вариантов одной партии один и тот же референс стиля (эталон — корпус `S_P3_01`), у навесок
  O-01…O-10 ядро O-00 в кадре.
- В папке объекта `prompt.txt`: промт, негативный промт, модель, сид каждого варианта. Промт собирается из
  `visual_en` манифеста + общего стиля §0.1; негатив: `text, watermark, people, background scenery, transparent
  glass, thin wires, cables, floating parts, multiple objects`.
- На каждую партию `contact_sheet_<batch>.jpg`: сетка всех объектов партии, три варианта в ряд, подпись id под
  каждым — по нему Rio выбирает.
- `manifest_out.json`: копия входного манифеста с заполненными `files` и `status` (`done` / `skipped: причина`).
- `README.md`: модель, дата, что не получилось.

**Выбор.** Rio (или я по его слову) заполняет `selection.json`: `{ "C-01": "v2", "C-02": "v1", … }`. Можно
«v2, но крыша как в v3» — тогда это заметка Codex на раунд 2.

**Раунд 2 — виды для 3D (один запрос по `selection.json`).**
- В ту же папку объекта: `final_front.png`, `final_left.png`, `final_back.png`, `final_top.png` выбранного варианта,
  тот же фон, свет и масштаб в кадре — для многовидовой генерации в Hunyuan3D. Если надёжной многовидовой генерации
  нет — только `final_front.png` в ¾, Hunyuan работает и с одной картинкой.
- Для объектов из нескольких частей (C-07 ворота, ST-04 ворота ангара) — дополнительно `final_open.png` с
  открытым состоянием, если состояние задаётся геометрией, а не материалом.

**Раунд 3 — меш (Hunyuan3D → Blender), по §0.3.** В папке объекта: `hunyuan/<id>_raw.glb` (как есть, для истории),
`blender/<id>_vN.blend`, и выход в `F:\Rio\3D\<Kit>\out\<Asset>_vN\` (FBX, текстуры, `_Spec.json`, превью с 0,5–1 м и
20 м, README). Один объект — одна папка на всех трёх раундах, чтобы было видно путь от картинки до меша.

---

## 1. Этап 0–1: первый час (нужен любому происхождению) — партия Codex № 1

### 1.0 Ковчег (A-00) — решение Rio 07.10: отдельный новый корабль, не `S_P3_01`

| Id | Объект | Габариты | Описание | Visual (EN) |
|---|---|---|---|---|
| A-00 | **Корабль-ковчег класса S** | 32 × 20 × 9 м, клиренс под брюхом 2,2 м на четырёх опорах | **Лендер, а не грузовик:** приземистый широкий корпус на четырёх посадочных опорах с широкими башмаками, нос-мостик с панорамным остеклением в плотной раме, широкий брюшной трап под кормой (ширина 3 м, уклон ≤ 1 : 4), два раскладных крыла с радиаторами-сотами и солнечными панелями по бокам, внешние кассеты грузовых контейнеров (совместимы с `CargoCrates`), один оранжевый шлюз в борту, короткие двигательные блоки (они мертвы — без свечения). Интерьер (Blender): мостик с голостолом и двумя креслами, коридор, трюм с ящиками и кран-балкой, кубрик на две койки, шлюз. Состояния: мёртвый (всё тёмное, одна консоль) → живой по секциям. Ковчег никогда не летает: это база ступеней 0–2 | A squat planetary lander ark, the size of a small freighter: a wide low white enamel hull standing on four heavy landing legs with broad pads, a glazed bridge nose in a heavy dark frame, a wide belly ramp at the rear, two fold-out side wings with cellular radiator grilles and solar panels, external cargo container cassettes along the flanks, one orange airlock door, short dark engine blocks, clean manufactured sci-fi industrial design, no antennas or cables |

### 1.1 Пропсы интеракции (A12)

| Id | Объект | Габариты | Описание | Visual (EN) |
|---|---|---|---|---|
| P-01 | Рубильник питания ковчега | 0,6 × 0,3 × 1,2 м, настенный | Вертикальная панель в тёмной стали с одним большим оранжевым рычагом (ход 40°), тремя индикаторами и защитной скобой. Ставится на стену мостика и в станции (ADRIFT) | A wall-mounted industrial power switch panel: dark graphite steel frame, one large orange safety lever, three small round indicator lamps, white enamel cover plate, chunky bolts, clean manufactured look |
| P-02 | Алтарь Строителей (пешая зона) | 2,4 м в поперечнике, 0,9 м высотой | Шестигранная плита чёрного камня с утопленным кольцом глифов; в центре — гранёный диск, который светится при активации. Используется у подножия монумента и под порталом | A hexagonal black faceted stone altar, low and massive, with a recessed ring of thin glowing teal-white glyph grooves and a faceted central disc, ancient alien monument style, no machinery |
| P-03 | Деталь привода (переносимая) | 0,8 × 0,4 × 0,4 м | Чёрный гранёный блок Строителей с одним светящимся швом, в сером кожухе-рамке APS с ручками (чтобы нести). Нужен для ремонта корабля в ДОЛГОМ ПАДЕНИИ | A portable alien drive core: a black faceted stone block with one glowing seam, mounted inside a grey industrial carry frame with two orange handles |
| P-04 | Рубильник секции станции | как P-01, но с поворотным колесом | Вариант P-01 для оживления мёртвой станции: поворотный штурвал + рычаг | Same family as the lever panel, with an added steel hand-wheel valve, aged grey enamel, dust |

### 1.2 Кит колонии (A1) — наземные модули

Общее: каждый модуль стоит на **плинте** (общая деталь: бетонно-серая плита 0,4 м с оранжевой каймой и четырьмя
анкерами; код уже сажает модули на плинт). Модули в APS Industrial, белая эмаль + оранжевые рамы люков. Сокеты
`SOCK_Door` у каждого обитаемого.

| Id | Модуль | Габариты (м) | Силуэт и детали | Состояния | Visual (EN) |
|---|---|---|---|---|---|
| C-01 | **Habitat** | 9 × 5 × 4 | Лежачий цилиндр со скруглёнными торцами на раме, одна торцевая дверь-шлюз с оранжевой рамой, два круглых иллюминатора сбоку, короб климатики сверху с сотовой решёткой | стройка/живой/мёртвый (окна) | A small planetary habitat module: a horizontal white enamel cylinder with rounded ends on a dark steel frame, an end airlock door with an orange frame, two round side windows, a rooftop HVAC box with a cellular grille, clean manufactured sci-fi industrial design |
| C-02 | **Greenhouse** | 12 × 5 × 4,5 | Арочный туннель с большими остеклёнными секциями в белых рамах, торцевой тамбур, баки воды по боку | живой: тёплый свет внутри | A planetary greenhouse module: an arched tunnel with large dark glazed panels in white enamel frames, a small entry vestibule at one end, two water tanks along one side, orange accent trims |
| C-03 | **Storage** | 7 × 4 × 3,5 | Прямоугольный склад с большой роллетной дверью, навес, стеллажи видны через проём; совместим с `Props\CargoCrates` | — | A compact cargo storage module: a boxy white enamel hut with one large orange roller door, a short overhang roof, steel corner posts |
| C-04 | **Floodlight mast** | 9 высота, база 1,5 | Телескопическая мачта на треноге с блоком из четырёх прожекторов и солнечной панелью сзади | живой: прожекторы | A tall tripod floodlight mast: telescopic grey steel pole, a head with four rectangular floodlights, a small solar panel on the back |
| C-05 | **SolarArray** | 8 × 5 × 2,5 | Два ряда панелей на наклонной раме с одним следящим шарниром, короб инвертора с решёткой | — | A ground solar array: two rows of dark panels on a tilted white steel frame with a single tracking hinge, an inverter box with a cellular grille |
| C-06 | **CommsMast** | 14 высота | Ферменная мачта с тарелкой 2,5 м наверху и двумя панельными антеннами, растяжки не рисовать | живой: красный маркер наверху | A communications mast: a lattice steel tower with a 2.5 m dish and two panel antennas at the top, an equipment cabinet at the base |
| C-07 | **FAB (новый)** | 14 × 9 × 5,5 | Ангар-мастерская с воротами 4 × 3,5 м (въезд ровера), кран-балка внутри, станок 3D-печати как «сердце» (видно через ворота), дымоход с решёткой. Сокет `SOCK_Vehicle` перед воротами | стройка/живой | A planetary fabrication garage: a white enamel hangar with one large orange roll-up gate, a crane beam visible inside, a boxy fabricator unit, an exhaust stack with a cellular grille, dark steel frame |
| C-08 | **LandingPad** | диск 28, высота 0,6 | Восьмиугольная площадка из плит с оранжевой каймой, четыре посадочных огня, стойка-мачта с ветроуказателем-маячком | живой: огни | An octagonal landing pad: segmented grey plates with an orange edge marking, four corner landing lights, one small beacon post |
| C-09 | **Plinth** | 6 × 6 × 0,4 | Общий плинт модулей | — | A square concrete-grey foundation slab with an orange rim and four anchor bolts |

### 1.3 Кит колонии — орбитальные модули (крепятся к станции/штабу)

| Id | Модуль | Габариты | Описание | Visual (EN) |
|---|---|---|---|---|
| C-10 | **SolarWing** | 14 × 3,5 | Складное крыло из 6 панелей на ферме с шарниром-стыком | A foldable orbital solar wing: six dark panels on a thin white truss with a single docking hinge |
| C-11 | **CargoPods** | 5 × 5 × 5 | Гроздь из четырёх цилиндрических контейнеров на крестовине с оранжевыми торцами | A cluster of four cylindrical cargo pods on a cross-shaped steel rack, orange end caps |
| C-12 | **NavBeacon** | 2,5 | Короткий шпиль с кольцом огней и отражателем | A short navigation beacon spire with a ring of lights and a reflector dish |

### 1.4 Наземный кит построек (A2, первые три — P0)

| Id | Постройка | Габариты | Описание | Visual (EN) |
|---|---|---|---|---|
| S-01 | **MiningOutpost** (буровая) | 14 × 12 × 16 | Четырёхногая буровая вышка над шахтным оголовком, конвейер к бункеру-воронке 8 м, кабина оператора, оранжевые полосы на опорах; 6 M/мин должно быть «видно» — кучи породы у бункера | A planetary mining rig: a four-legged drilling derrick over a wellhead, a conveyor arm feeding a tall funnel-shaped ore bunker, a small operator cabin, white enamel panels with orange stripes on dark steel legs |
| S-02 | **SolarCollector** (поле) | 24 × 24 × 3 | Массив 4 × 4 панелей на общей следящей раме с одной центральной опорой, трансформаторный короб | A solar collector field: a 4×4 array of dark panels on a shared tracking frame with one central pylon, a transformer cabinet at the edge |
| S-03 | **DeepObservatory** | тарелка 16, база 8 × 8 × 6 | Радиотарелка на двухосной вилке над бетонно-серым блоком с куполом-обсерваторией сбоку | A deep-space observatory: a 16 m radio dish on a two-axis yoke over a square grey base building with a small optical dome beside it |
| S-04 | Garrison | 22 × 16 × 6 | Низкий бункер с скошенными стенами, двумя башенками сенсоров, воротами | A low military garrison bunker with sloped walls, two sensor turrets and a wide gate, white enamel over dark steel |
| S-05 | MassDriver | рельс 140, пилоны 10 | Наклонный электромагнитный рельс на пилонах с зарядным зданием у основания | A planetary mass driver: a long inclined electromagnetic rail on steel pylons rising from a loading building |
| S-06 | CivicForum | купол 26, высота 12 | Низкий купол с кольцом остекления и входной галереей | A civic forum: a low wide dome with a continuous glazed ring and a colonnaded entrance gallery |

### 1.5 Монумент Строителей — пешая зона

Монументы сами процедурные (код). Нужен один авторский элемент для пешего шага **AT THE FOOT OF IT**: **дайс** —
восьмиугольный постамент 30 м в поперечнике, 2 м высотой, с процессионной лестницей и алтарём P-02 в центре (чёрный
камень, швы). Генерируется как один объект; Visual (EN): *An octagonal black faceted stone dais, 30 m across, with a
wide ceremonial stair on one side and a hexagonal altar at the centre; thin glowing teal-white grooves run along
the edges; ancient alien monument, no machinery.*

---

## 2. Этап 2: орбита и система — партия Codex № 2

### 2.1 LaunchYard (A3)

Габариты: площадка 40 м, гантри 26 м высотой. Описание: восьмиугольная площадка (как C-08, но больше) с двумя
портальными гантри-башнями на рельсах, кабель-мачта, заправочные баки в кожухе, здание управления 10 × 6 м с
остеклением на площадку. Сокеты: `SOCK_Pad` (центр), `SOCK_Door`. Visual (EN): *A planetary launch yard: a large
octagonal launch pad with two tall white gantry towers on rails, a tank farm in a grey enclosure, a small control
building with a glazed front, orange hazard stripes on the pad edge and gantry legs.*

### 2.2 Орбитальный кит построек (A2): ядро + навески

Принцип: одно **ядро** и 10 навесок; тип постройки = ядро + 1–2 навески (сборка в BP, без новых мешей на каждый
тип). Ядро стыкуется к станции и к другим ядрам.

| Id | Часть | Габариты | Описание | Visual (EN) |
|---|---|---|---|---|
| O-00 | **Core** | 14 × 7 × 7 | Цилиндр с четырьмя плоскими стыковочными гранями (`SOCK_Dock_1..4`), торцевой шлюз, кольцо радиаторов-сот, оранжевые стыковочные рамки | An orbital core module: a short white enamel cylinder with four flat docking faces and orange docking frames, an end airlock, a ring of cellular radiator grilles |
| O-01 | Survey boom (SurveyOutpost) | штанга 18 | Ферма с блоком сенсоров и двумя тарелками | A sensor boom attachment: a lattice arm with a sensor cluster and two small dishes |
| O-02 | Probe bay (ProbeBay) | 10 × 6 × 6 | Открытый стеллаж с 6 зондами в направляющих | A probe launch rack: an open frame holding six small probes in launch rails |
| O-03 | Gas scoop + tanks (GasHarvester) | 16 × 8 | Раструб-заборник и два сферических бака в кожухе | A gas harvester attachment: a wide intake scoop and two spherical tanks in a steel cradle |
| O-04 | Refinery stack (OrbitalRefinery) | 12 × 8 × 10 | Печь-цилиндр с радиаторами и три бака | An orbital refinery attachment: a furnace cylinder with large radiator fins and three product tanks |
| O-05 | Lab ring (ResearchStation) | кольцо 24 | Кольцевая лаборатория с остеклением и двумя тарелками | A research ring attachment: a glazed torus laboratory with two antenna dishes |
| O-06 | Habitat torus (OrbitalHabitat) | тор 44 | Жилой тор с окнами-полосой и спицами к ядру | An orbital habitat torus: a white enamel ring with a continuous window band and four spokes to the core |
| O-07 | Container racks (CargoHub) | 20 × 10 × 10 | Стеллажи под 12 контейнеров (`CargoCrates`) | A cargo hub attachment: open container racks holding stacked cargo containers |
| O-08 | Dock frame (RepairDock) | 46 × 24 × 20 | Открытая рама-эллинг с кран-балками и прожекторами, корабль S должен помещаться | A repair dock attachment: an open rectangular frame with gantry cranes and floodlights, large enough to hold a small ship |
| O-09 | Tank cluster (SupplyDepot) | 14 × 10 | Шесть баков на крестовине | A supply depot attachment: six cylindrical tanks on a cross rack |
| O-10 | Turret ring (DefencePlatform) | кольцо 16 | Кольцо с четырьмя башнями сенсоров/орудий (без стволов-игл) | A defence platform ring with four compact turret pods |

### 2.3 Реле и узлы системы (A2, StarSystem placement)

| Id | Тип | Габариты | Описание | Visual (EN) |
|---|---|---|---|---|
| R-01 | **SurveyBeacon** / SensorPicket | 8 | Шпиль-маяк с кольцом рефлекторов и солнечными крыльями, один оранжевый маяк | A survey beacon: a slim spire with a ring of reflectors, two small solar wings and one orange beacon light |
| R-02 | DeepSpaceScanner | тарелка 34 | Большая тарелка на ферме с блоком охлаждения | A deep-space scanner: a 34 m dish on a lattice frame with a radiator block behind it |
| R-03 | AdministrationHub | 60 × 30 | Три модуля-ядра крестом с остеклённым залом-ротондой | An administration hub: three core modules joined in a cross around a glazed rotunda hall |
| R-04 | HyperspaceRelay | кольцо-антенна 80 | Кольцо-антенна на спицах с ядром и двумя крыльями | A hyperspace relay: an 80 m antenna ring on spokes around a core module with two solar wings |
| R-05 | **JumpGate (свои ворота)** | кольцо 420, 6 пилонов | Кольцо из шести сегментов на шести пилонах-генераторах, внутренняя кромка с оранжевыми полосами, ядро питания сбоку; «люди повторили кольцо Строителей из стали» | A human-built jump gate: a 420 m ring of six white enamel segments joined by six dark steel generator pylons, orange emitter strips on the inner rim, a power core module attached to one side |

### 2.4 Модульная станция (A4) — хиро-ассет

Одна станция для всех ролей: первая станция ARK, мёртвая станция ADRIFT (предтеча — постаревший материал), секция
кольца RING (тот же интерьер, другой корпус). Части:

| Id | Часть | Габариты | Описание | Visual (EN) |
|---|---|---|---|---|
| ST-01 | **Hub ring** | кольцо 120, сечение 10 | Кольцо-коридор с полосой окон внутрь и наружу, четыре узла стыковки | A station hub ring: a 120 m torus corridor with a continuous window band, four docking nodes on the outer rim |
| ST-02 | **Core spindle** | 80 длина, 16 диаметр | Веретено с залом-ротондой в середине (остекление) и реакторным блоком с радиаторами внизу | A station core spindle: a vertical white enamel cylinder with a glazed rotunda hall at mid-height and a reactor block with radiator fins at the bottom |
| ST-03 | **Dock arm** | 60 × 12 | Рукав с двумя стыковочными узлами и шлюзами | A docking arm: a straight corridor arm with two docking collars and orange airlock frames |
| ST-04 | **Hangar** | 44 × 30 × 22 | Ангар с раздвижными воротами (корабль S входит), кран, площадка | A station hangar: a boxy bay with large sliding doors, an internal crane beam and a landing deck |
| ST-05 | Corridor segment / Airlock | 20 × 6 × 6 / 6 × 4 × 4 | Коридорная секция и шлюз-кубик | A straight station corridor segment with window strips; a cube-shaped airlock module with an orange door frame |
| ST-06 | Ring section (RING) | дуга 300, сечение 60 × 40 | Сегмент орбитального кольца Строителей со встроенной «человеческой» секцией внутри: чёрный камень снаружи, APS-интерьер внутри | A segment of a colossal ancient orbital ring: black faceted stone exterior with glowing seams, a cut-out revealing a modern white enamel habitat section inside |

Интерьеры ST-01..ST-05 — в Blender из интерьерного кита (§4.1), проходы 2,05 м, маршруты ко всем выходам, знаки
(как wayfinding у Codex на кораблях). Для «мёртвого» состояния — материал экспедиции (§0.1), аварийные лампы,
сцена оживления: P-04 в реакторном блоке, свет по секциям.

### 2.5 Дерелики (A5)

| Id | Объект | Габариты | Описание | Visual (EN) |
|---|---|---|---|---|
| D-01 | Зонд экспедиции | 4 × 2 × 2 | Разбитый зонд: цилиндр с раскрытыми панелями, тарелка, обгоревший кожух | A crashed survey probe: a small cylinder with deployed panels, a bent dish, scorched grey enamel, dust |
| D-02 | Материал «повреждённый» | — | Для XXS/XS корпусов: серая выцветшая эмаль, нагар, вмятины (декали), свет выкл. | — |

### 2.6 Интерьер корпуса Строителей (A6) — для BOARD IT и ДОЛГОГО ПАДЕНИЯ

Только Blender (интерьер). Три пространства в масштабе корпуса 2–4 км, но проходимые человеком:
- **Ангар стыковки** 90 × 45 × 30 м: чёрный камень, ступенчатые стены, «посадочная» площадка под корабль S, швы
  света ведут к проходу.
- **Палуба** 200 × 10 × 7 м: длинный коридор с гранёными рёбрами, пол из плит, швы.
- **Зал-мостик** сфера 40 м: ступенчатый амфитеатр к центру, пьедестал (P-02) и «окно» — проём, из которого видно
  небо (сюда игрок приходит смотреть, как уходит дом).
Visual (EN) для референса (не для генерации): *interior of a colossal ancient alien vessel carved from black
faceted stone, stepped walls, thin glowing teal-white seams tracing the floor, no pipes or machinery, cold and
monumental.*

---

## 3. Этап 3–4: скопление и галактика — партия Codex № 3

| Id | Объект | Габариты | Описание | Visual (EN) |
|---|---|---|---|---|
| E-01 | Вход лифта (A7) | зал 24 × 24 × 10 в подножии башни | Портал в основании башни Строителей: ступенчатый проём, зал ожидания с кабиной | An entrance hall at the base of a colossal black stone tower: a stepped doorway, a hall with a docked elevator cabin |
| E-02 | Кабина лифта (A7) | 6 × 6 × 5 | Гранёная чёрная кабина с одним швом света по периметру и прозрачной (в Blender) стеной | A faceted black stone elevator cabin with one glowing seam around its base and a wide window wall |
| E-03 | Коллектор роя Дайсона (A8) | панель 100 (в коде масштабируется) | Шестигранная панель-зеркало на спице с радиатором на тыльной стороне | A hexagonal solar collector panel on a central spine with radiator fins on the back, white frame |
| E-04 | Астероиды (A9) | 5 штук, 50–500 | Угловатые камни с кратерами и прожилками руды | Irregular rocky asteroids with craters and faint metallic veins, grey-brown stone |
| E-05 | Кольцо гиганта (A9) | материал | Полупрозрачное кольцо из пыли и льда с тёмными щелями | — |
| E-06 | Точки интереса (A10) | 4–20 | Выход руды (жила с кристаллами металла), кристальное поле, вентиляция (трубы-конусы с паром), обломок зонда (D-01) | An ore outcrop: a rock ridge with exposed metallic veins; a field of translucent crystal shards; a cluster of natural cone-shaped thermal vents |
| E-07 | Обломки корпуса (ДОЛГОЕ ПАДЕНИЕ) | код + 3 детали | Три авторских «разлома» (кромки) для процедурного корпуса, лежащего на поверхности | A jagged broken edge section of black faceted stone hull, cracked and partially buried |
| E-08 | Финальный монумент | игла + кольцо | Процедурно из существующих форм (код), своя подсветка — ассета нет | — |
| X-01 | **Корабль поколений** (A13, EXODUS, после галактических режимов) | 3 200 × 700 × 600 м; жилое кольцо Ø 600 м | Класс Titan. Длинный хребет-ферма с жилым кольцом на трети длины (кольцо вращается — на картинке статично), мостик-башня в носу с панорамным остеклением, ангарная палуба с воротами под кормой (входят S-корабли и лендер A-00), блок из четырёх двигателей с огромными радиаторами-сотами, баки и грузовые кассеты вдоль хребта. Материал — APS Industrial, постаревший за века: выцветшая серая эмаль, заплаты другого оттенка, нагар у двигателей, редкие тёплые окна кольца. Интерьер (Blender): мостик, сегмент кольца как «улица» с жильём и садами, ангар | A colossal generation ship, three kilometres long: a long lattice spine with a rotating habitat ring one third along it, a glazed bridge tower at the bow, a hangar deck with large doors under the stern, a block of four engines with huge cellular radiator fins, tanks and cargo cassettes along the spine; faded grey enamel with patches of different shades, scorch near the engines, warm window bands on the ring; aged industrial sci-fi, no cables |
| E-09 | Ховер и дрон свои (A11, после EA) | 6 × 3 × 1,6 / 2 × 2 × 0,8 | Ховер: низкая платформа с двумя наклонными импеллерами и кабиной на двоих; дрон: квадрокоптер-«кирпич» с кожухами | A two-seat hover vehicle with a low white enamel body, two tilted ducted fans and a glazed cabin, orange trims; a boxy quadcopter drone with ducted rotors |

---

## 4. Интерьеры для существующих станций (отдельные итерации)

Rio 07.10: у нынешних станций корпуса в целом неплохие, но нет интерьеров; выбрасывать не хочется. Кто делает —
Codex в Blender (он уже сделал проходимые интерьеры на M5). Принцип: не резать корпус заново, а «вставлять» интерьер
в существующий объём через интерьерный кит, как на кораблях.

### 4.1 Интерьерный кит (общий для станций, секций кольца и A4)

Модули 1 : 1, стыкуются по сетке 2 м: коридор прямой 4 м, поворот 90°, Т-узел, шлюзовой тамбур 4 × 4, зал 12 × 12 с
колоннами, лестница/пандус (ступень 0,18 м, пандус ≤ 1 : 8), дверной проём 2,2 × 1,0 с оранжевой рамой, перила 1,1 м,
панель-светильник (тёплый белый), знак маршрута (как wayfinding Codex), пол-решётка, потолок с коробом вентиляции,
консоль-пульт, кресло, шкафчик, койка. Стиль APS Industrial; для «мёртвых» — материал экспедиции.

### 4.2 Список станций и что внутри

| Станция (BP / меш) | Корпус | Что нужно внутри | Примечание |
|---|---|---|---|
| `BP_SpaceStation_R2` (`SM_MERGED_StaticMeshActor_124/388`, 38–46 МБ) | кит-баш, но силуэт ок; по умолчанию у всех новых миров | стыковочный тамбур → коридор → зал-ротонда с окном на планету → верфь-балкон; маршрут к 2 выходам | первой, потому что игрок видит её сразу; одновременно привести материалы к APS Industrial |
| `BP_SpaceStation_R1`, `_H2` | merged-меши | то же, меньший объём (тамбур + коридор + зал) | второй очередью |
| `AI_Stations\03`, `03_01`, `03_02` (41–46 МБ, без BP) | «станция с платформами и подвесным шпинделем», три варианта материалов | BP + интерьер: ангар-док в платформе, коридор в шпинделе, зал наверху | лучший кандидат на **мёртвую станцию ADRIFT** (готовый чужой силуэт) и на вторую станцию систем |
| `AI_Stations\04` (три кольца на шпинделе) | без BP | BP + интерьер в среднем кольце | кандидат на `OrbitalHabitat`/`ResearchStation` крупного размера |
| `AI_Stations\01` (дисковый хаб) | используется как SpaceHub ×10 000 | зал-док в диске (посадка на хаб) | нужен объём гравитации, как у станций (код S) |
| `BP_STATION_LaunchStation` | интерьер есть, из иг-паков | заменить иг-детали на кит, проверить 2 м | только если остаётся в игре |
| `BP_SpaceHeadquarters_Alpha` v12 | интерьер есть | орреи 1,83–1,99 м (голова), траншея хаба, плиты 5–16 см | иг-меш 143 МБ → LFS; доводка по списку из `2026-10-03-hq-alpha-passages.md` |
| `Pack_1\SpaceHeadquarters_P1_10`, `SpaceShipyard_P1_04` | только на авторской карте | BP + интерьер зала | резерв для `AdministrationHub` |
| `P1_19` колесо, `P1_21` цитадель | мегаструктуры-резерв | интерьер по требованию ступени 4 | после EA |

Для каждой: отчёт clearance 2,05 м, маршруты ко всем выходам, UCX ≤ 300, Nanite на корпусе, версия рядом со старой.

---

## 5. Порядок партий для Codex

1. **Партия 1 (первый час):** P-01…P-04, C-01…C-12, S-01…S-03, дайс монумента. Это 25 объектов, параллельно с кодом
   режимов.
2. **Партия 2 (орбита):** LaunchYard, O-00 + O-01…O-10, R-01…R-05, ST-01…ST-06, D-01, интерьерный кит §4.1,
   интерьер R2.
3. **Партия 3 (скопление/галактика):** E-01…E-07, интерьеры `AI_Stations` 03/04, A6 корпус Строителей.
4. **После EA:** E-09, остальные станции §4.2, S-04…S-06.

Каждая партия: Codex → картинки (2–3 на объект) → Rio выбирает → Hunyuan3D → Blender → спецификация и превью →
OK Rio → импорт. Версии не перезаписывать.
