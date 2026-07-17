# NX_RoiSelection - BLU 빛샘 분석 툴

TV/모니터 BLU(Back Light Unit) 구조의 빛샘(Light Leakage) 분석용 경량 광학 시뮬레이션 프로젝트. 이 저장소는 그중 **ROI 지정 + Mesh/메타데이터 추출** 파트(NX Open API, C++)를 담당한다.

## 구조

```
NX_RoiSelection/
├─ NX_RoiSelection.sln     솔루션 파일 (6개 프로젝트: 아래 참고)
├─ external/NxCadCore/     git submodule - CadImportModule(북마크 Open, Assembly Tree/
│                          Geometry/Material 읽기) + Shared(INxSessionAccessor, NxGeometryUtils)
│                          여러 프로젝트가 공유하는 라이브러리라 별도 저장소로 분리돼 있음
│
├─ RoiModule/              ROI 지정(Face/Box/Body/좌표) + Mesh/메타데이터 추출
│  ├─ Core/                NXOpen 비의존 - IRoiResolver, IROIManager, ROISelection, FacetMesh,
│  │                       ROIManager, RoiPickingWorkflows
│  ├─ NxBackend/           NXOpen 의존 - NxRoiResolver
│  ├─ tests/CoreTests/     ROIManager + RoiPickingWorkflows 단위 테스트 (Mock 기반, 25개)
│  └─ README.md            모듈 경계, 사용법, JSON 스키마, 확인 필요 API
│
└─ App/                    NxCadCore + RoiModule을 잇는 CLI 실행 파일
   └─ src/
      ├─ main.cpp
      └─ Export/           JsonWriter, JsonSceneExporter (두 모듈의 출력을 하나의 JSON으로 결합)
```

`external/NxCadCore`는 git submodule이라 clone 후 `git submodule update --init --recursive`가 필요하다 (회사 PC처럼 네트워크 git이 막힌 환경이면 `docs/OfficeVerificationChecklist.md` Phase 0의 대안 절차 참고).

RoiModule은 NxCadCore의 **Core(안정적인 인터페이스/데이터 모델)에만** 의존하고, `NxConnector` 같은 구체 구현에는 의존하지 않는다 (`external/NxCadCore/Shared/NxContracts::INxSessionAccessor`를 통한 접점). 자세한 설계 배경은 `RoiModule/README.md` 참고.

## 빠른 시작

```
App.exe --bookmark <path.plmxml> [--output scene.json] [--pick-roi] [--pick-roi-box] [--roi-point x,y,z] [--roi-scope] [--roi-mesh]
```

각 플래그의 의미는 `RoiModule/README.md`의 "사용법" 절 참고.

- 개인 PC(NX 미설치): `external/NxCadCore/CadImportModule/Core`, `RoiModule/Core`, `RoiModule/tests/CoreTests`만 로컬에서 빌드/실행 가능 (실제로 검증됨 - CoreTests 25개 통과)
- 회사 PC(NX2406 + Teamcenter): 전체 6개 프로젝트 빌드 - **본 프로젝트에 머징하기 전, `docs/OfficeVerificationChecklist.md`의 단계별 검증 체크리스트를 먼저 통과시킬 것** (빌드 → 알려진 API 불확실성 대조 → 실제 조립체로 기능 검증 → JSON 스키마 확인)

## 이번 범위 밖

- 가상 GAP(부품 Move 기반), 광학 Property 매핑, Ray Tracing - 다른 챕터/모듈 담당
- ROI Front/Back 뷰 강제, Mesh 품질 파라미터 CLI 노출, 시각화/Hide(리포팅 단계 관심사) - `RoiModule/README.md`의 "이번 범위 밖" 절 참고
