# 회사 PC 검증 체크리스트 (본 프로젝트 머징 전)

이 문서는 `CadImportModule`/`RoiModule`을 개인 PC(NX 미설치)에서 작성한 뒤, 회사 PC(NX2406 + Teamcenter 설치 환경)로 옮겨 검증할 때 쓰는 체크리스트다. AI 에이전트가 이어서 작업할 수 있도록 각 항목에 정확한 파일 경로/명령/판정 기준을 명시한다.

**전체 목표**: 아래 6단계를 순서대로 통과시키는 것. 각 단계는 이전 단계가 통과된 상태를 전제로 한다. 실패 시 "실패 시 조치"에 적힌 파일부터 수정한다.

---

## Phase 0 — 사전 확인

- [ ] 이 저장소(`TV_BitSam/`)가 회사 PC에 clone되어 있다
- [ ] NX2406, Teamcenter(TCE Managed Mode)가 설치되어 있고 라이선스가 유효하다
- [ ] Visual Studio(NX2406과 호환되는 버전 - 통상 VS2022)가 설치되어 있다

---

## Phase 1 — 빌드 검증

**판정 기준**: 5개 프로젝트가 모두 경고 없이(또는 최소 오류 없이) 빌드된다.

1. [ ] **NX Command Prompt**에서 Visual Studio 실행 (일반 시작 메뉴로 열면 `UGII_BASE_DIR` 환경변수가 없어 실패함)
2. [ ] `echo %UGII_BASE_DIR%`로 경로가 실제로 잡히는지 확인
3. [ ] 저장소 루트의 `TV_BitSam.sln` 오픈
4. [ ] `Shared/NxOpenSdk.props`의 `AdditionalDependencies`(`libNXOpenCpp.lib`, `libNXOpenCppUI.lib`, `libufun.lib`)가 실제 `%UGII_BASE_DIR%\UGOPEN` 폴더의 파일명과 일치하는지 확인, 다르면 교체
5. [ ] `CadImportModule/Core` 빌드 → 성공해야 정상 (NX 비의존, 개인 PC에서 이미 검증됨)
6. [ ] `RoiModule/Core` 빌드 → 성공해야 정상 (마찬가지로 NX 비의존)
7. [ ] `CadImportModule/NxBackend` 빌드 → 실패 시 Phase 2 표의 해당 항목부터 수정
8. [ ] `RoiModule/NxBackend` 빌드 → 실패 시 Phase 2 표의 해당 항목부터 수정
9. [ ] `App` 빌드 → 실패 시 위 4개가 먼저 통과했는지 확인 (프로젝트 참조 문제일 가능성)
10. [ ] VS가 "Retarget Solution"을 제안하면 수락 (PlatformToolset이 `v143`으로 고정되어 있는데 설치된 VS 버전이 다를 수 있음)

---

## Phase 2 — 알려진 API 불확실성 (컴파일 에러 시 여기부터 대조)

개인 PC에는 실제 NX Open SDK가 없어 리서치 근거로 작성했다. 아래 표의 시그니처/접근자명이 실제 NX2406 헤더와 다르면 수정한다.

| 파일 | 위치(함수) | 확인할 것 |
|---|---|---|
| `CadImportModule/NxBackend/src/NxConnector.cpp` | `Connect()` | `PartCollection::OpenBaseDisplay`의 정확한 오버로드(파라미터 개수/순서, 특히 `PartLoadStatus**`) |
| 〃 | `GetSessionInfo()` | Revision/Unit을 읽는 정확한 접근자 (`BasePart`가 아니라 `Part`/`PartUnits`일 가능성) |
| `CadImportModule/NxBackend/src/NxAssemblyReader.cpp` | `ReadTree()` | `workPart->ComponentAssembly()` 호출에 `dynamic_cast<NXOpen::Part*>` 다운캐스트가 필요한지 |
| `CadImportModule/NxBackend/src/NxAssemblyReader.cpp` | `BuildComponentInfo()` | Visibility(`IsBlanked()` 등)/Suppression 상태를 읽는 정확한 접근자 (지금은 항상 `true`/`false` 고정값) |
| `CadImportModule/NxBackend/src/NxGeometryReader.cpp` | `ReadGeometry()` | `Body::OwningComponent()`가 실제 존재하는 이름인지 |
| `Shared/NxContracts/src/NxGeometryUtils.cpp` | `ComputeBoundingBoxForTag()` | UF_MODL 함수명(`AskBoundingBox` vs `AskBoundingBoxExact`)과 파라미터 순서 |
| `CadImportModule/NxBackend/src/NxMaterialReader.cpp` | `ReadMaterial()` | `Part::PhysicalMaterials()`, `PhysicalMaterial::GetBodies()`가 실제 존재하는 이름인지 - 없으면 `UF_SF_ask_body_material` 저수준 API로 대체 |
| `RoiModule/NxBackend/src/NxRoiResolver.cpp` | `PromptSelectFace()` | `NXOpen::UI::GetUI()`, `ui->SelectionManager()` 정확한 접근자/반환 타입 |
| 〃 | 〃 | `Selection::SelectTaggedObject`의 정확한 오버로드(파라미터 개수/순서) |
| 〃 | 〃 | `Selection::MaskTriple`의 필드명 (`Type`/`Subtype`/`SolidBodySubtype`으로 가정) |
| `RoiModule/NxBackend/src/NxRoiResolver.cpp` | `BuildSelectionFromFace()` | Face Area 추출용 UF Mass-Properties 함수 시그니처 (현재 `area = 0.0` placeholder - Phase 5에서 채워 넣기) |

각 항목은 코드 안에 `TODO(office-PC verify)` 주석으로도 표시되어 있다 - `TODO(office-PC verify)`로 전체 검색하면 이 표와 1:1로 매칭된다.

---

## Phase 3 — 테스트 데이터 준비

- [ ] NX에서 간단한 가상 조립체 생성 (컴포넌트 2~3개, Body 1~2개 수준으로 최소화 - 문제 생겼을 때 원인 추적 쉽게)
- [ ] 이 조립체를 북마크(.plmxml)로 저장, 예: `C:\test\sample.plmxml`
- [ ] 조립체 안에 최소 1개 Body에 Material을 할당해둔다 (Material 읽기 검증용)

---

## Phase 4 — CadImportModule 기능 검증

명령: `App.exe --bookmark C:\test\sample.plmxml --output scene.json`

- [ ] 콘솔에 에러 없이 "Session connected. Model=... Part=..." 로그가 뜬다
- [ ] Assembly Tree 콘솔 출력이 NX Assembly Navigator에서 보이는 실제 구조와 일치한다
- [ ] 각 Component의 Body/Face/Edge 개수가 NX에서 직접 확인한 값과 일치한다
- [ ] Material을 할당해둔 Body의 `materialName`이 `scene.json`에 올바르게 나온다
- [ ] `scene.json` 파일이 생성되고, JSON으로서 유효하다 (파싱 에러 없음)

**실패 시**: Phase 2 표에서 해당 기능(Tree/Geometry/Material)의 행부터 재확인.

---

## Phase 5 — RoiModule 기능 검증

명령: `App.exe --bookmark C:\test\sample.plmxml --output scene.json --pick-roi`

- [ ] Assembly/Geometry/Material을 다 읽은 후 NX **그래픽 창이 실제로 뜬다** (External EXE가 자체 세션을 만들었다는 뜻)
- [ ] 콘솔에 "Pick another ROI face? [y/n]"가 뜨면 `y` 입력 시 Face 클릭 대기 상태가 된다
- [ ] 화면에서 Face를 클릭하면 정상적으로 진행된다 (취소 시 "User cancelled..." 경고 후 다시 물어봄)
- [ ] `scene.json`의 `roiSelections`에 `resolved:true` 항목이 생기고, `componentName`/`bodyId`가 실제로 클릭한 Face의 소속과 일치한다
- [ ] `boundingBox`가 그 Face 하나의 범위로 합리적인 값이다 (전체 Body 크기가 아님)
- [ ] **이번 기회에 Face Area 계산 로직을 채워 넣는다**: `NxRoiResolver::BuildSelectionFromFace`의 `selection.area = 0.0;` 부분을 실제 UF Mass-Properties 호출로 교체하고, `area`가 0이 아닌 합리적인 값으로 나오는지 확인

**실패 시**: Phase 2 표의 `NxRoiResolver` 관련 행부터 재확인. 그래픽 창 자체가 안 뜨면 `NxConnector::Connect()`가 세션을 정상적으로 얻었는지(Phase 4가 먼저 통과했는지) 확인.

---

## Phase 6 — 머징 전 최종 확인

- [ ] Phase 4, 5의 `scene.json`을 다른 팀원(Material Engine 등)이 합의한 JSON 스키마와 대조 - 스키마는 `CadImportModule/README.md`의 "인터페이스" 절과 `App/src/Export/JsonSceneExporter.h` 참고
- [ ] Phase 2 표의 모든 항목이 실제 코드에서 확정되었는지 재확인 (더 이상 `TODO(office-PC verify)`가 남아있지 않아야 이상적)
- [ ] `git status`로 확인 후, 회사 PC에서 수정한 내용을 커밋 (`TODO(office-PC verify)` 해결 커밋은 별도로 남기면 리뷰하기 좋음)
- [ ] 본 프로젝트(전체 BLU 툴)의 브랜치/구조에 머징하기 전, 이 저장소가 서브모듈로 들어가는지 코드가 통째로 합쳐지는지 방식을 먼저 정할 것 (이 문서 밖의 결정 사항)
