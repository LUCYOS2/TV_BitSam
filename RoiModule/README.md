# ROI Module

BLU 빛샘 분석 툴 프로젝트의 **ROI 지정 + Mesh/메타데이터 추출** 담당 모듈. `CadImportModule`이 열어놓은 NX 세션 위에서, 사용자가 지정한 ROI(Face 클릭 / Box 드래그 다중 선택 / 좌표 직접 지정 / Body 단위 로컬 스코프)의 실제 지오메트리 데이터(BoundingBox/Area/Facet Mesh)를 **별도 JT/Mesh 파일 없이** 살아있는 세션에서 바로 뽑아낸다.

`CadImportModule`과는 물리적으로 분리된 별도 모듈이다 - 이 문서와 `external/NxCadCore/CadImportModule/docs/DevGuide.md`를 함께 참고할 것.

**경로 안내**: `CadImportModule`/`Shared`는 여러 프로젝트가 함께 쓰는 공용 라이브러리라 `external/NxCadCore`(별도 저장소, git submodule)로 빠져 있다 - 아래 프로즈에 나오는 `CadImportModule/...`, `Shared/...`는 실제로는 `external/NxCadCore/CadImportModule/...`, `external/NxCadCore/Shared/...` 경로다 (의존 방향/설계 의도 설명이 목적이라 표기는 짧게 유지).

## 모듈 경계 (왜 NxConnector를 직접 참조하지 않는가)

`NxRoiResolver`는 `CadImportModule`의 `NxConnector`를 구체 클래스로 참조하지 않고, `Shared/NxContracts/include/NxContracts/INxSessionAccessor.h`(세션 접근용 최소 인터페이스, `RawSession()`/`IsAvailable()`)만 본다. `NxConnector`가 이 인터페이스를 구현한다.

```
RoiModule/NxBackend  --(depends on)-->  Shared/NxContracts::INxSessionAccessor
                                                    ▲
                                                    │ implements
                                         CadImportModule/NxBackend::NxConnector
```

이 덕분에:
- `NxConnector`의 시그니처가 바뀌어도 RoiModule은 영향받지 않는다 (인터페이스만 유지되면 됨)
- 나중에 세션을 얻는 다른 방식(테스트용 Mock 등)으로 교체 가능
- `RoiModule/NxBackend.vcxproj`는 `CadImportModule/NxBackend.vcxproj`를 전혀 참조하지 않는다 (Additional Include Directories에도 없음) - 오직 `RoiModule/Core`, `CadImportModule/Core`(BoundingBox3D/OperationResult 재사용), `Shared/NxContracts`만 참조

`RoiModule/Core`(인터페이스/데이터 모델)는 `CadImportModule/Core`에 의존한다 - 이건 의도된 단방향 의존이다 (RoiModule → CadImportModule::Core, 그 반대는 없음).

## 폴더 구조

```
RoiModule/
├─ Core/         NXOpen 비의존 - IRoiResolver, IROIManager, ROISelection, FacetMesh, ROIManager,
│                RoiPickingWorkflows(피킹 오케스트레이션 - App과 CoreTests가 공유)
├─ NxBackend/    NXOpen 의존 - NxRoiResolver (인터랙티브 Face/Body/Box 피킹, 좌표 resolve, Facet 추출)
└─ tests/        CoreTests - ROIManager + RoiPickingWorkflows(MockRoiResolver 기반) 단위 테스트
                 (25개, MSBuild로 개인 PC 빌드/실행 검증됨)
```

## 사용법

`RoiModule` 자체는 실행 파일이 없다 - `TV_BitSam/App`이 이 모듈과 `CadImportModule`을 함께 링크해서 실행한다. ROI 지정 방식은 4가지이며 서로 독립적으로 조합 가능하다 (`IRoiResolver` 참고):

```
App.exe --bookmark <path.plmxl> --pick-roi                    # Face 클릭 (1개씩, 인터랙티브)
App.exe --bookmark <path.plmxl> --pick-roi-box                 # Box 드래그 (여러 Face 한번에, 인터랙티브)
App.exe --bookmark <path.plmxl> --roi-point 10.5,20.0,5.25     # 좌표 직접 지정 (비-인터랙티브, 반복 가능)
App.exe --bookmark <path.plmxl> --roi-point 10.5,20.0,5.25:top-corner   # note 포함
App.exe --bookmark <path.plmxl> --roi-scope                    # Body 단위 로컬 스코프 (여러 개, 인터랙티브)
App.exe --bookmark <path.plmxl> --pick-roi --roi-mesh                  # 위 방식 아무거나 + Facet Mesh도 함께 추출
```

- `--pick-roi`: 조립체 트리/Geometry/Material을 다 읽은 뒤, 콘솔에 "Pick another ROI face? [y/n]"가 뜨면 `y` 입력 → NX 그래픽 창에서 Face를 클릭 → 자동으로 `ROISelection`에 결과(소속 Component/Body, BoundingBox, Area)가 채워져 저장됨 → `n`을 입력하면 종료.
- `--pick-roi-box`: 동일한 y/n 루프지만, 한 번의 드래그로 박스 안의 모든 Face를 한꺼번에 resolve.
- `--roi-point`: 이미 알고 있는 좌표(설정 파일/계산값 등)를 그 자리에서 즉시 resolve - 그 좌표를 포함(또는 없으면 가장 가까운) Body를 찾아 `ROISelection`(kind=Point, `coordinate` 필드에 원본 좌표 보존)으로 채운다. 세션만 있으면 되므로 그래픽 창 입력을 기다리지 않는다.
- `--roi-scope`: **"하단 코너 빛샘 확인 → 후면 중앙부 빛샘 확인"처럼 국부 영역을 하나씩 돌아가며 검토하는 워크플로우 전용.** "Add another ROI scope? [y/n]" → 스코프 라벨 입력(예: `bottom-corner`) → NX를 Front/Back 표준 뷰로 맞춘 뒤 그 영역 위에 박스를 드래그 → 박스에 걸린 모든 Body가 `ROISelectionKind::Component`로 resolve되어 그 라벨(`ROISelection::scopeId`)로 태깅됨 → 반복. 각 스코프는 완전히 독립적이다(이전 스코프에 뭘 골랐는지 다음 스코프에 영향 없음).
- `--roi-mesh`: 위 방식들로 resolve된 각 선택에 대해 추가로 `IRoiResolver::ExtractFacetMesh`를 호출해 `ROISelection::mesh`(정점+법선+삼각형 인덱스)를 채운다. Face 선택은 그 Face 하나만, Point/Component(좌표·Body) 선택은 소속 Body의 모든 Face를 합쳐서 추출한다. 비용이 크므로 기본은 꺼져 있음(opt-in) - 실패해도 나머지 필드(BoundingBox/Area 등)는 그대로 유효.

**주의(블로킹)**: `PromptSelectFace`/`PromptSelectFacesInBox`/`PromptSelectBodiesInBox`는 사용자가 클릭/드래그할 때까지 대기하는 동기 호출이다. 완전 자동/무인 배치 처리와는 상충되므로, 해당 플래그를 안 주면 완전 자동으로 끝까지 실행된다 (opt-in). `ResolvePointAtCoordinate`/`ExtractFacetMesh`는 좌표·선택이 이미 주어져 있으므로 블로킹하지 않는다.

### ROI 스코프 - 설계 배경 (왜 NX Hide를 안 쓰는가)

"국부 영역을 하나씩 돌아가며 검토"하는 워크플로우에서, 처음엔 확인하지 않은 나머지를 NX에서 Hide(Blank)해서 재선택을 빠르게 하는 방안을 검토했다. 하지만 각 스코프가 서로 **완전히 다른/독립된 영역**이라는 게 확정되면서(드릴다운이 아님) 그 방안은 기각했다:
- 다음 스코프를 고르기 전에 매번 Show All부터 해야 해서, Hide로 얻는 이득이 그대로 상쇄됨
- "지금 뭐가 숨겨져 있었는지" NX 세션 전역 상태를 계속 추적해야 하는 부담만 남음
- 광학(Ray Tracing) 분석 관점에서도, 실제 표면을 인위적으로 Trim하면 존재하지 않는 절단면이 생겨 반사/굴절 계산이 왜곡될 위험이 있음

그래서 범위 제한은 **NX 화면 상태가 아니라 export되는 데이터**에서 한다: 각 스코프의 결과(포함된 Body들 + 메타데이터)를 `scopeId`로 묶어서 export하고(아래 JSON 스키마 참고), 다운스트림 Ray Tracing이 스코프 단위로 데이터를 받아 그 범위만 계산한다. NX Hide/Blank API는 이 모듈에서 아예 호출하지 않는다 - 시각화는 분석이 끝난 뒤 리포팅 단계의 관심사로 명시적으로 분리했다 (RoiModule 밖).

## JSON 스키마 (scene.json)

`roiSelections`(스코프 없는 단발 픽 - `--pick-roi`/`--pick-roi-box`/`--roi-point`)와 `roiScopes`(스코프 있는 픽 - `--roi-scope`, `scopeId`별로 그룹화)로 나뉜다:

```jsonc
{
  "roiSelections": [ { "kind": "Face", "scopeId": "", ... } ],
  "roiScopes": [
    { "scopeId": "bottom-corner", "selections": [ { "kind": "Component", ... }, ... ] },
    { "scopeId": "back-center",   "selections": [ ... ] }
  ]
}
```

## 이번 범위 밖 / 아직 결정 안 됨

- **Mesh 품질 파라미터 고정**: `NxRoiResolver::FacetFace`의 chord-height(0.01mm)/각도 허용치(15°)가 하드코딩됨 - CLI 옵션으로 노출할지는 미정.
- **Front/Back 뷰 강제 여부**: `--roi-scope`는 사용자가 미리 Front/Back 뷰로 맞췄다고 가정만 하고 코드로 강제/검증하지 않는다 - 필요하면 `NXOpen::View`의 현재 방향을 읽어 검증하는 단계 추가 검토.
- **시각화/Hide**: 위 "설계 배경" 참고 - 의도적으로 이번 범위 밖.

## 확인 필요한 API (회사 PC에서 NX2406 헤더로 대조)

| 위치 | 확인해야 할 것 |
|---|---|
| `NxRoiResolver::PromptSelectFace` | `NXOpen::UI::GetUI()`, `ui->SelectionManager()` 정확한 접근자명/반환 타입 |
| 〃 | `Selection::SelectTaggedObject`의 정확한 오버로드(파라미터 개수/순서) - NX8에서 `SelectObject`/`SelectObjects`가 Deprecated되고 이걸로 대체됨은 확인됨 |
| 〃 | `Selection::MaskTriple`의 필드명 (`Type`/`Subtype`/`SolidBodySubtype`으로 가정) |
| `NxRoiResolver::PromptSelectFacesInBox` | `Selection::SelectTaggedObjects`(복수형) 존재 여부/정확한 오버로드 - 박스 드래그 다중 선택용으로 가정 |
| `NxRoiResolver::PromptSelectBodiesInBox` | `MaskTriple::SolidBodySubtype = UF_UI_SEL_FEATURE_SOLID_BODY`("전체 Body" 마스크) 정확한 상수명 - `UF_UI_SEL_FEATURE_ANY_FACE`와 대응되는 Body용 상수로 가정 |
| `NxRoiResolver::ResolvePointAtCoordinate` | `UFSession::Modl.AskPointContainment` 시그니처, `UF_MODL_POINT_*` enum 값 |
| `NxContracts::BodiesOfComponent` | ⚠️ `docs/OfficeVerificationChecklist.md` Phase 2-A 참고 - `Component::Prototype()`이 참조된 `BasePart*`를 직접 반환하는지, 한 단계 더 거쳐야 하는지 |
| `NxContracts::CollectAllBodiesInWorkPart` | `Part::ComponentAssembly()`가 `BasePart`에 없고 `Part`에만 있는지 (`NxAssemblyReader::ReadTree()`와 동일 전제 공유) |
| `NxRoiResolver::ComputeFaceArea` | `UFSession::Modl.AskMassProps3d` 정확한 파라미터 목록/순서/출력 배열 크기 (`uf_modl.h`의 `UF_MODL_ask_mass_props_3d` 대조) |
| `NxRoiResolver::FacetFace` | ⚠️ **이번 항목 중 확인 우선순위 최상위.** `UFSession::Modl.AskFaceFacets` 자체의 존재 여부/정확한 이름과 파라미터 목록(chord-height/각도 단위, 출력 배열이 정말 UF 힙 할당 flat `double*`/`int*`인지)이 `AskBoundingBox`/`AskMassProps3d`/`AskPointContainment`보다 훨씬 불확실함. `UF_free`가 실제 해제 함수명인지도 미확인 - 잘못되면 이름이 컴파일 자체가 안 되거나(함수 없음) 메모리 누수/이중 해제로 이어짐. 회사 PC에서 `uf_modl.h`/`uf_facet.h` 직접 검색으로 먼저 확인할 것. |

## 빌드 상태

| 프로젝트 | NX 필요 여부 | 개인 PC에서 빌드 가능? |
|---|---|---|
| Core | 불필요 | 가능 - `CadImportModule/Core`와 함께 MSBuild로 검증됨 |
| NxBackend | 필요 | 불가능 - 회사 PC 전용 |
