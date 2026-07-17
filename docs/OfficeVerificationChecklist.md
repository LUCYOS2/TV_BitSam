# 회사 PC 검증 체크리스트 (본 프로젝트 머징 전)

이 문서는 `CadImportModule`/`RoiModule`을 개인 PC(NX 미설치)에서 작성한 뒤, 회사 PC(NX2406 + Teamcenter 설치 환경)로 옮겨 검증할 때 쓰는 체크리스트다. AI 에이전트가 이어서 작업할 수 있도록 각 항목에 정확한 파일 경로/명령/판정 기준을 명시한다.

**전체 목표**: 아래 단계를 순서대로 통과시키는 것. 각 단계는 이전 단계가 통과된 상태를 전제로 한다. 실패 시 "실패 시 조치"에 적힌 파일부터 수정한다.

---

## Phase 0 — 사전 확인

- [ ] 이 저장소(`TV_BitSam/`)가 회사 PC에 clone되어 있다
- [ ] NX2406, Teamcenter(TCE Managed Mode)가 설치되어 있고 라이선스가 유효하다
- [ ] Visual Studio(NX2406과 호환되는 버전 - 통상 VS2022)가 설치되어 있다

---

## Phase 1 — 빌드 검증

**판정 기준**: 6개 프로젝트가 모두 경고 없이(또는 최소 오류 없이) 빌드된다.

1. [ ] **NX Command Prompt**에서 Visual Studio 실행 (일반 시작 메뉴로 열면 `UGII_BASE_DIR` 환경변수가 없어 실패함)
2. [ ] `echo %UGII_BASE_DIR%`로 경로가 실제로 잡히는지 확인
3. [ ] 저장소 루트의 `TV_BitSam.sln` 오픈
4. [ ] `Shared/NxOpenSdk.props`의 `AdditionalDependencies`(`libNXOpenCpp.lib`, `libNXOpenCppUI.lib`, `libufun.lib`)가 실제 `%UGII_BASE_DIR%\UGOPEN` 폴더의 파일명과 일치하는지 확인, 다르면 교체
5. [ ] `CadImportModule/Core` 빌드 → 성공해야 정상 (NX 비의존, 개인 PC에서 이미 검증됨)
6. [ ] `RoiModule/Core` 빌드 → 성공해야 정상 (마찬가지로 NX 비의존)
7. [ ] `RoiModule/tests/CoreTests` 빌드 후 `build\Debug\x64\CoreTests.exe` 실행 → **"All tests passed (25)"** 가 나와야 정상. NX와 완전히 무관한 프로젝트라 회사 PC/개인 PC 구분 없이 항상 통과해야 함 - 여기서 실패하면 NX 문제가 아니라 순수 C++ 회귀이므로 가장 먼저 원인 파악할 것
8. [ ] `CadImportModule/NxBackend` 빌드 → 실패 시 Phase 2 표의 해당 항목부터 수정
9. [ ] `RoiModule/NxBackend` 빌드 → 실패 시 Phase 2 표의 해당 항목부터 수정
10. [ ] `App` 빌드 → 실패 시 위 4개가 먼저 통과했는지 확인 (프로젝트 참조 문제일 가능성)
11. [ ] VS가 "Retarget Solution"을 제안하면 수락 (PlatformToolset이 `v143`으로 고정되어 있는데 설치된 VS 버전이 다를 수 있음)

---

## Phase 2 — 알려진 API 불확실성 (컴파일 에러 시 여기부터 대조)

개인 PC에는 실제 NX Open SDK가 없어 리서치 근거로 작성했다. 아래 표의 시그니처/접근자명이 실제 NX2406 헤더와 다르면 수정한다. 각 항목은 코드 안에 `TODO(office-PC verify)` 주석으로도 표시되어 있다 - `TODO(office-PC verify)`로 전체 검색하면 이 표와 1:1로 매칭된다.

| 파일 | 위치(함수) | 확인할 것 |
|---|---|---|
| `CadImportModule/NxBackend/src/NxConnector.cpp` | `Connect()` | `PartCollection::OpenBaseDisplay`의 정확한 오버로드(파라미터 개수/순서, 특히 `PartLoadStatus**`) |
| 〃 | `GetSessionInfo()` | Revision/Unit을 읽는 정확한 접근자 (`BasePart`가 아니라 `Part`/`PartUnits`일 가능성) |
| `CadImportModule/NxBackend/src/NxAssemblyReader.cpp` | `ReadTree()` | `workPart->ComponentAssembly()` 호출에 `dynamic_cast<NXOpen::Part*>` 다운캐스트가 필요한지 |
| `CadImportModule/NxBackend/src/NxAssemblyReader.cpp` | `BuildComponentInfo()` | Visibility(`IsBlanked()` 등)/Suppression 상태를 읽는 정확한 접근자 (지금은 항상 `true`/`false` 고정값) |
| `Shared/NxContracts/src/NxGeometryUtils.cpp` | `ComputeBoundingBoxForTag()` | UF_MODL 함수명(`AskBoundingBox` vs `AskBoundingBoxExact`)과 파라미터 순서 |
| `Shared/NxContracts/src/NxGeometryUtils.cpp` | `BodiesOfComponent()` | ⚠️ **Phase 2-A와 직결, 확인 우선순위 최상위.** `Component::Prototype()`이 이 컴포넌트가 참조하는 `NXOpen::BasePart*`를 직접 반환(또는 안전하게 `dynamic_cast` 가능)하는지 - 아니면 `Prototype()->OwningPart()`처럼 한 단계 더 거쳐야 하는지 |
| `Shared/NxContracts/src/NxGeometryUtils.cpp` | `CollectAllBodiesInWorkPart()` | `Part::ComponentAssembly()`가 `BasePart`가 아니라 `Part`에만 있는지 (있으면 `dynamic_cast<Part*>(workPart)` 유지, `NxAssemblyReader::ReadTree()`와 동일 전제) |
| `CadImportModule/NxBackend/src/NxMaterialReader.cpp` | `ReadMaterial()` | `Part::PhysicalMaterials()`, `PhysicalMaterial::GetBodies()`가 실제 존재하는 이름인지 - 없으면 `UF_SF_ask_body_material` 저수준 API로 대체 |
| `RoiModule/NxBackend/src/NxRoiResolver.cpp` | `PromptSelectFace()` | `NXOpen::UI::GetUI()`, `ui->SelectionManager()` 정확한 접근자/반환 타입 |
| 〃 | 〃 | `Selection::SelectTaggedObject`(단수형)의 정확한 오버로드(파라미터 개수/순서) |
| 〃 | 〃 | `Selection::MaskTriple`의 필드명 (`Type`/`Subtype`/`SolidBodySubtype`으로 가정) |
| `RoiModule/NxBackend/src/NxRoiResolver.cpp` | `PromptSelectFacesInBox()` | `Selection::SelectTaggedObjects`(**복수형**) 존재 여부/정확한 오버로드 - 박스 드래그 다중 선택의 핵심 API |
| 〃 | `PromptSelectBodiesInBox()` | 위와 동일 API + `MaskTriple::SolidBodySubtype = UF_UI_SEL_FEATURE_SOLID_BODY`("전체 Body" 마스크) 정확한 상수명 |
| 〃 | `ResolvePointAtCoordinate()` | `UFSession::Modl.AskPointContainment` 시그니처, `UF_MODL_POINT_*` enum 값 |
| 〃 | `ComputeFaceArea()` | `UFSession::Modl.AskMassProps3d` 정확한 파라미터 목록/순서/출력 배열 크기 |
| 〃 | `FacetFace()` | ⚠️ **이 표에서 확인 우선순위 최상위.** `UFSession::Modl.AskFaceFacets` 자체의 존재 여부/정확한 이름과 파라미터 목록(chord-height/각도 단위, 출력 배열이 정말 UF 힙 할당 flat `double*`/`int*`인지), `UF_free`가 실제 해제 함수명인지 - 잘못되면 컴파일 자체가 안 되거나 메모리 누수/이중 해제로 이어짐. `uf_modl.h`/`uf_facet.h` 직접 검색으로 먼저 확인할 것 |

---

## Phase 2-A — 어셈블리 전체 Body 순회 검증 (⚠️ 가장 근본적인 위험, 최우선 확인)

**확인된 사실 (프로젝트 담당자 확인, NX 헤더 대조 아님)**: 북마크(.plmxml)로 조립체를 열면 최상위 work part 자체는 Body가 하나도 없는 빈 컨테이너이고, 실제 형상(BLU의 Reflector/LGP/Diffuser/Prism/Bezel 등 각 레이어)은 **각 컴포넌트가 참조하는 개별 Part 파일**에 들어 있다. 즉 `workPart->Bodies()`를 그냥 호출하면 사실상 빈 리스트가 나온다.

**이미 수정함**: `NxGeometryReader::ReadGeometry()`(기존 코드, `workPart->Bodies()` + `Body::OwningComponent()` 필터링 방식이었음)와 `NxRoiResolver`의 `ResolvePointAtCoordinate()`/`ExtractFacetMesh()`(이번에 추가, 마찬가지로 `workPart->Bodies()`를 직접 호출하던 방식)를 전부 `Shared/NxContracts`의 새 공유 헬퍼로 교체했다:

- `NxContracts::BodiesOfComponent(component)` - 그 컴포넌트가 참조하는 Part 자신의 Body만 (재귀 아님)
- `NxContracts::CollectAllBodies(component)` - 위를 서브트리 전체에 재귀 적용
- `NxContracts::FindComponentByName(root, name)` - 이름으로 트리에서 컴포넌트 찾기 (`NxGeometryReader`가 컴포넌트 이름 하나만 받으므로 필요)
- `NxContracts::CollectAllBodiesInWorkPart(workPart)` - 조립체면 `CollectAllBodies`로, 단일 파트(비조립체)면 `workPart->Bodies()`로 알아서 분기

(`PromptSelectFacesInBox`/`PromptSelectBodiesInBox`는 NX의 `SelectTaggedObjects`가 화면에 보이는 개체를 직접 찾아주므로 이 문제와 무관 - 애초에 `workPart->Bodies()`를 호출한 적이 없다.)

**남은 건 이 교체가 실제로 맞는지 회사 PC에서 확인하는 것뿐이다**:

- [ ] Phase 3에서 만들 테스트 조립체를, **컴포넌트들이 서로 다른 Part 파일을 참조하도록**(전형적인 실제 어셈블리처럼) 구성한다
- [ ] `App.exe --bookmark <path> --output scene.json` 실행 후 `scene.json`의 `geometry` 배열에 각 컴포넌트의 Body가 실제로 채워지는지 확인 (0개면 아래 표의 `BodiesOfComponent`/`CollectAllBodiesInWorkPart` 행부터 재확인)
- [ ] `--roi-point`/`--roi-scope`로 실제 좌표/박스 지정 시에도 Body가 정상적으로 잡히는지 함께 확인 (`ResolvePointAtCoordinate`/`ExtractFacetMesh`가 같은 헬퍼를 쓰므로 `geometry` 배열이 맞으면 이쪽도 맞을 가능성이 높지만, 별도로 한 번 확인)

---

## Phase 3 — 테스트 데이터 준비

- [ ] NX에서 간단한 가상 조립체 생성 (컴포넌트 2~3개, Body 1~2개 수준으로 최소화 - 문제 생겼을 때 원인 추적 쉽게)
- [ ] **컴포넌트들이 서로 다른 Part 파일을 참조하도록 구성** (Phase 2-A 검증용 - 최상위 파일에만 Body를 두면 그 문제를 못 잡아냄)
- [ ] 이 조립체를 북마크(.plmxml)로 저장, 예: `C:\test\sample.plmxml`
- [ ] 조립체 안에 최소 1개 Body에 Material을 할당해둔다 (Material 읽기 검증용)

---

## Phase 4 — CadImportModule 기능 검증

명령: `App.exe --bookmark C:\test\sample.plmxml --output scene.json`

- [ ] 콘솔에 에러 없이 "Session connected. Model=... Part=..." 로그가 뜬다
- [ ] Assembly Tree 콘솔 출력이 NX Assembly Navigator에서 보이는 실제 구조와 일치한다
- [ ] 각 Component의 Body/Face/Edge 개수가 NX에서 직접 확인한 값과 일치한다 (Phase 2-A 참고 - 0으로 나오면 그 문제)
- [ ] Material을 할당해둔 Body의 `materialName`이 `scene.json`에 올바르게 나온다
- [ ] `scene.json` 파일이 생성되고, JSON으로서 유효하다 (파싱 에러 없음)

**실패 시**: Phase 2 표에서 해당 기능(Tree/Geometry/Material)의 행부터, Body 개수가 0이면 Phase 2-A부터 재확인.

---

## Phase 5 — RoiModule 기능 검증

네 가지 ROI 지정 방식과 Facet Mesh 추출을 각각 검증한다. 전부 opt-in 플래그라 조합 가능 (`RoiModule/README.md` "사용법" 절 참고).

### 5-1. 단일 Face 클릭 (`--pick-roi`)

명령: `App.exe --bookmark C:\test\sample.plmxml --output scene.json --pick-roi`

- [ ] Assembly/Geometry/Material을 다 읽은 후 NX **그래픽 창이 실제로 뜬다** (External EXE가 자체 세션을 만들었다는 뜻)
- [ ] 콘솔에 "Pick another ROI face? [y/n]"가 뜨면 `y` 입력 시 Face 클릭 대기 상태가 된다
- [ ] 화면에서 Face를 클릭하면 정상적으로 진행된다 (취소 시 "User cancelled..." 경고 후 다시 물어봄)
- [ ] `scene.json`의 `roiSelections`에 `resolved:true` 항목이 생기고, `componentName`/`bodyId`가 실제로 클릭한 Face의 소속과 일치한다
- [ ] `boundingBox`가 그 Face 하나의 범위로 합리적인 값이다 (전체 Body 크기가 아님)
- [ ] `area`가 0이 아닌 합리적인 값(mm²)으로 나온다 (`ComputeFaceArea` - Phase 2 표 참고)

**실패 시**: Phase 2 표의 `NxRoiResolver` 관련 행부터 재확인. 그래픽 창 자체가 안 뜨면 `NxConnector::Connect()`가 세션을 정상적으로 얻었는지(Phase 4가 먼저 통과했는지) 확인.

### 5-2. Face 박스 선택 (`--pick-roi-box`)

명령: `App.exe --bookmark C:\test\sample.plmxml --output scene.json --pick-roi-box`

- [ ] "Pick another ROI box? [y/n]"에서 `y` → 그래픽 창에서 **왼쪽→오른쪽**으로 드래그 → 박스 안에 **완전히 들어온** Face들만 선택되는지 (Window 모드)
- [ ] 같은 위치에서 **오른쪽→왼쪽**으로 드래그 → 박스에 **조금이라도 닿은** Face들까지 선택되는지 (Crossing 모드) - 두 결과 개수가 다르게 나와야 정상
- [ ] `scene.json`의 `roiSelections`에 드래그로 고른 개수만큼 항목이 추가된다

**실패 시**: `PromptSelectFacesInBox`의 `SelectTaggedObjects`(복수형) 관련 TODO부터 확인.

### 5-3. 좌표 직접 지정 (`--roi-point`)

명령: `App.exe --bookmark C:\test\sample.plmxml --output scene.json --roi-point 10.5,20.0,5.25:corner-check`

- [ ] 그래픽 창 입력 없이(비-인터랙티브) 바로 처리된다
- [ ] `scene.json`의 `roiSelections`에 `kind:"Point"`, `coordinate`가 입력한 좌표와 일치, `note:"corner-check"` 항목이 생긴다
- [ ] 그 좌표를 포함하는 실제 Body의 `bodyId`/`componentName`이 채워진다 (좌표가 모든 Body 밖이면 가장 가까운 Body로 fallback되는지도 확인)

**실패 시**: `ResolvePointAtCoordinate`의 `AskPointContainment` 관련 TODO부터 확인.

### 5-4. ROI 스코프 (`--roi-scope`) - Body 단위 국부 영역

명령: `App.exe --bookmark C:\test\sample.plmxml --output scene.json --roi-scope`

- [ ] NX를 **Front 뷰**로 맞춘 뒤 "Add another ROI scope? [y/n]"에서 `y` → 스코프 라벨(예: `bottom-corner`) 입력 → 그 영역 위에 박스 드래그 → 박스에 걸린 Body들이 `Component` kind로 resolve되는지
- [ ] 다시 `y` → 다른 라벨(예: `back-center`, 또는 Back 뷰로 돌려서) → 다른 영역 드래그 → 첫 번째 스코프와 결과가 섞이지 않는지
- [ ] `scene.json`에 `roiSelections`가 아니라 **`roiScopes`** 배열에 `{scopeId, selections}` 형태로 그룹화되어 나온다 - 스코프별 개수가 실제 드래그한 것과 일치하는지

**실패 시**: `PromptSelectBodiesInBox`의 `UF_UI_SEL_FEATURE_SOLID_BODY` 마스크 상수부터 확인 (Phase 2-A도 함께 - Body가 하나도 안 잡히면 그쪽 문제일 수 있음).

### 5-5. Facet Mesh 추출 (`--roi-mesh`, 위 4가지 방식과 조합)

명령 예: `App.exe --bookmark C:\test\sample.plmxml --output scene.json --pick-roi --roi-mesh`

- [ ] `scene.json`의 각 `roiSelections`/`roiScopes` 항목에 `mesh.valid:true`, `mesh.vertices`/`mesh.triangles`가 채워진다
- [ ] 삼각형 개수가 합리적이다 (Face 하나 기준 수십~수백 개 - 0개거나 비정상적으로 많으면 chord-height/각도 허용치 확인)
- [ ] Component(Body) 선택의 경우, 그 Body의 모든 Face 메쉬가 합쳐져서 나오는지 (Face 개수만큼 조각이 이어붙여져 있어야 함)

**실패 시**: Phase 2 표의 `FacetFace()` 행(⚠️ 최우선 표시) 부터 확인 - 이 항목이 이번 전체 리스트 중 가장 불확실한 API임.

---

## Phase 6 — 머징 전 최종 확인

- [ ] Phase 4, 5의 `scene.json`을 다른 팀원(Material Engine, Ray Tracing 등)이 합의한 JSON 스키마와 대조 - 스키마는 `App/src/Export/JsonSceneExporter.h`와 `RoiModule/README.md`의 "JSON 스키마" 절 참고 (특히 `roiScopes`는 이번에 새로 추가된 구조라 다른 모듈이 아직 모를 수 있음 - 공유 필요)
- [ ] Phase 2 표 + Phase 2-A의 모든 항목이 실제 코드에서 확정되었는지 재확인 (더 이상 `TODO(office-PC verify)`가 남아있지 않아야 이상적)
- [ ] `git status`로 확인 후, 회사 PC에서 수정한 내용을 커밋 (`TODO(office-PC verify)` 해결 커밋은 별도로 남기면 리뷰하기 좋음)
- [ ] 본 프로젝트(전체 BLU 툴)의 브랜치/구조에 머징하기 전, 이 저장소가 서브모듈로 들어가는지 코드가 통째로 합쳐지는지 방식을 먼저 정할 것 (이 문서 밖의 결정 사항)
