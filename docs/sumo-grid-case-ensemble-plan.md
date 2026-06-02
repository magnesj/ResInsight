# Sumo grid-case ensemble (ROFF geometry) for ResInsight

## Context

ResInsight already consumes the local Sumo bridge API (`http://localhost:8000`) for
**summary** data: `RiaSumoConnector` fetches a blob URL from the bridge, downloads the
blob from Sumo (via redirect), and `RimSummaryEnsembleSumo` parses it. The Python bridge
now also exposes **grid** endpoints (`grid_access.py` / `grids/router.py`):

- `GET /cases/{case}/ensembles/{ens}/grid_info_list` → `list[GridInfo]`, where
  `GridInfo = { name: str, realizations: list[int] }` (each grid name plus the realization
  numbers available for it).
- `GET /cases/{case}/ensembles/{ens}/grids/{grid}/realizations/{real}/blob_url`

The grid blobs are **ROFF** files. We want the grid-side analog of the Sumo summary
feature: an **ensemble of grid cases** (`RimEclipseCaseEnsemble`) whose members are a new
`RimEclipseCase` subclass that downloads and parses its ROFF grid geometry from Sumo
instead of from disk — i.e. a "Sumo version of `RimEclipseResultCase`".

**Decisions (confirmed with user):** geometry only (no cell properties yet); an *ensemble*
of grid cases; parse the downloaded blob **in memory** via `std::istream` (no temp file).
A **GUI lets the user pick a grid and multi-select realization numbers** from that grid's
available realizations; one `RimEclipseCaseSumo` is created per selected realization.

## Approach

Four pieces: (1) make the ROFF grid reader stream-capable, (2) add grid discovery + download
to the connector, (3) a new Sumo grid case class, (4) a GUI data source + command that lets
the user select realizations and builds the ensemble.

### 1. ROFF reader: parse from a stream

`RifRoffFileTools::openGridFile(const QString& fileName, ...)`
(`ApplicationLibCode/FileInterface/RifRoffFileTools.cpp:97`) currently opens an
`std::ifstream` and feeds it to `roff::Reader reader( stream )` (`:132`). `roff::Reader`
already accepts any `std::istream&` (`ThirdParty/roffcpp/src/Reader.hpp:35`).

- Extract the body into a new overload `openGridFile( std::istream& stream,
  RigEclipseCaseData* eclipseCase, QString* errorMessages )`.
- Keep the existing file-path overload as a thin wrapper that opens the `ifstream` and
  delegates. No behavior change for existing ROFF file import.
- Declare both in `RifRoffFileTools.h`.

### 2. Grid discovery + download in `RiaSumoConnector`

Files: `ApplicationLibCode/Application/Tools/Cloud/RiaSumoConnector.{h,cpp}`.
Mirror the existing summary blob path (`requestBlobIdForEnsemble` `:362`,
`requestParquetDataBlocking` `:500`). The actual download/redirect chain
(`requestBlobDownload` `:403` → `requestBlobByRedirectUri` `:450`) is **already generic**
and returns raw bytes — reuse it unchanged.

Add a small struct (in `RiaSumoConnector.h`, next to `SumoCase`):
`struct SumoGridInfo { QString name; std::vector<int> realizations; };`

Add methods:
- `requestGridInfoForEnsembleBlocking( caseId, ensembleName )` → GET `.../grid_info_list`,
  parse the JSON array of `{name, realizations:[int]}` into `m_gridInfos`; accessor
  `std::vector<SumoGridInfo> gridInfos()`. (Parse with `QJsonDocument`, as `parseCases`/
  `parseVectorNames` do.)
- `requestGridBlobIdForEnsemble( caseId, ensembleName, gridName, realization )` → GET
  `.../grids/{grid}/realizations/{real}/blob_url`; reuse `parseBlobUrl` to store into
  `m_blobUrl`.
- `QByteArray requestGridDataBlocking( caseId, ensembleName, gridName, realization )` —
  same shape as `requestParquetDataBlocking`: request blob id (blocking), extract blob id
  from the URL, `requestBlobDownload`, return bytes from `m_redirectInfo`.

Note: the bridge base URL is hard-coded as `http://localhost:8000` in the existing summary
methods; reuse the same literal for consistency (a follow-up could centralize it).

### 3. New grid case: `RimEclipseCaseSumo`

New files `ApplicationLibCode/ProjectDataModel/RimEclipseCaseSumo.{h,cpp}` (sibling of
`RimRoffCase`). `class RimEclipseCaseSumo : public RimEclipseCase`. Model after
`RimRoffCase::openEclipseGridFile()` (`RimRoffCase.cpp:66`) but source bytes from Sumo:

- Fields: `caseId`, `ensembleName`, `gridName`, `realization` (int); `QPointer<RiaSumoConnector>`
  obtained via `RiaApplication::instance()->makeSumoConnector()` (as
  `RimSummaryEnsembleSumo` does, `RimSummaryEnsembleSumo.cpp:57`).
- `openEclipseGridFile()` override:
  - early-return if `eclipseCaseData()` already set;
  - `setReservoirData( new RigEclipseCaseData( this ) )`;
  - `QByteArray bytes = connector->requestGridDataBlocking( caseId, ensembleName, gridName, realization )`;
  - wrap in `std::istringstream`( `bytes.toStdString()` ) and call the new
    `RifRoffFileTools::openGridFile( stream, eclipseCaseData(), &errors )`;
  - then mirror the post-grid steps from `RimRoffCase`: `setFlipAxis`, `computeCachedData()`,
    `createPlaceholderResultEntries()`, optional `computeDepthRelatedResults()`,
    `computeCellVolumes()`. **Skip** `RifRoffFileTools::createInputProperties` (geometry only).
- `locationOnDisc()` → empty/synthetic (no file on disk); disable grid-file IO fields not
  relevant. Provide `setCaseInfo`-style setters for the Sumo coordinates.
- Register `CAF_PDM_SOURCE_INIT` and add to `ProjectDataModel/CMakeLists_files.cmake`.

### 4. GUI: grid data source with realization selection + creation command

Two new objects, modeled on the summary pair (`RimSummarySumoDataSource` +
`RicCreateSumoEnsembleFeature`).

**a) `RimSumoGridDataSource`** — new files
`ApplicationLibCode/ProjectDataModel/Sumo/RimSumoGridDataSource.{h,cpp}`
(or alongside `RimSummarySumoDataSource` under `Summary/Sumo/`). `public RimNamedObject`.
This is the GUI the user interacts with. Fields:
- `m_caseId`, `m_caseName`, `m_ensembleName` — read-only (set on creation).
- `m_gridName` (`PdmField<QString>`) — dropdown; options from `calculateValueOptions` via
  `connector->requestGridInfoForEnsembleBlocking(...)` then `gridInfos()` names.
- `m_realizations` (`PdmField<std::vector<int>>`) — **multi-select** with
  `caf::PdmUiTreeSelectionEditor` (same idiom as `m_sumoEnsembleNames` in
  `RimCloudDataSourceCollection`). `calculateValueOptions` returns the realization numbers
  of the currently selected `m_gridName` from `connector->gridInfos()`.
- `appendMenuItems()` → `menuBuilder.addCmdFeature( "RicCreateSumoGridEnsembleFeature" )`
  (mirrors `RimSummarySumoDataSource::appendMenuItems` `:151`).
Provide getters `caseId()/ensembleName()/gridName()/selectedRealizations()` for the command.

The data source is created from the existing cloud selection. Extend
`RimCloudDataSourceCollection` (`RimCloudDataSourceCollection.cpp`): add a child array
`m_sumoGridDataSources` and an **"Add Grid Data Source(s)"** push button (mirror
`m_addDataSources`/`addDataSources()` `:266`) that creates a `RimSumoGridDataSource` for the
selected case + each selected ensemble, calling `requestGridInfoForEnsembleBlocking` to
prime options.

**b) `RicCreateSumoGridEnsembleFeature`** — new files
`ApplicationLibCode/Commands/Sumo/RicCreateSumoGridEnsembleFeature.{h,cpp}` (sibling of
`RicCreateSumoEnsembleFeature`). Context-menu action on a selected `RimSumoGridDataSource`.
Mirror `RicCreateGridCaseEnsemblesFromFilesFeature::importSingleGridCaseEnsemble`
(`RicCreateGridCaseEnsemblesFromFilesFeature.cpp:104`):
- `new RimEclipseCaseEnsemble`, `setName(ensembleName)`;
- **for each selected realization number** in `dataSource->selectedRealizations()`:
  `new RimEclipseCaseSumo`, set Sumo coordinates (caseId, ensembleName, gridName,
  realization), `ensemble->addCase(case)`;
- `oilfield->analysisModels()->caseEnsembles.push_back( ensemble )`;
  `updateConnectedEditors()`.

## Files to create / modify

- **Create:** `ProjectDataModel/RimEclipseCaseSumo.{h,cpp}`,
  `ProjectDataModel/Summary/Sumo/RimSumoGridDataSource.{h,cpp}`,
  `Commands/Sumo/RicCreateSumoGridEnsembleFeature.{h,cpp}`.
- **Modify:** `FileInterface/RifRoffFileTools.{h,cpp}` (stream overload);
  `Application/Tools/Cloud/RiaSumoConnector.{h,cpp}` (`SumoGridInfo` + grid methods);
  `ProjectDataModel/Cloud/RimCloudDataSourceCollection.{h,cpp}` (grid data-source array +
  "Add Grid Data Source(s)" button);
  the `CMakeLists_files.cmake` files for `ProjectDataModel`, `Summary/Sumo`, and
  `Commands/Sumo` for the new files.

## Verification

1. Build ResInsight (CMake) — confirm new files compile and link (Arrow/ROFF already linked).
2. Run the Python bridge: `uvicorn sumo_bridge_api.primary.main:app --port 8000`.
3. In ResInsight: authenticate in the Cloud Data node, select field/case/ensemble,
   **Add Grid Data Source(s)**.
4. Select the new grid data source: pick a **grid name**, then **check the realization
   numbers** to import; right-click → **Create Grid Ensemble**.
5. Confirm a `RimEclipseCaseEnsemble` appears with exactly one grid case per selected
   realization; open a 3D view on a case and verify the grid geometry renders.
6. Check the bridge console shows `grid_info_list` + `blob_url` hits, and ResInsight log
   shows the ROFF parse ("Opening roff …"/grid dimensions) for the in-memory stream.
7. Regression: import a local `.roff` grid file to confirm the refactored file-path
   `openGridFile` overload still works unchanged.

## Out of scope (future)

Cell properties (needs new bridge endpoints for property names + per-property blob URL),
statistics across the ensemble, and centralizing the hard-coded `localhost:8000` base URL.
