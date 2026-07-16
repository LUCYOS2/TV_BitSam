# TV_BitSam - BLU 빛샘 분석 툴

TV/모니터 BLU(Back Light Unit) 구조의 빛샘(Light Leakage) 분석용 경량 광학 시뮬레이션 프로젝트. 이 저장소는 그중 **CAD Import + ROI 지정** 파트(NX Open API, C++)를 담당한다.

## 구조

```
TV_BitSam/
├─ TV_BitSam.sln          솔루션 파일 (5개 프로젝트: 아래 참고)
├─ Shared/                두 모듈이 공유하는 빌드 설정 + 얇은 인터페이스
│  ├─ NxOpenSdk.props      NX Open Include/Lib 경로 (모든 NX 의존 프로젝트가 import)
│  └─ NxContracts/         INxSessionAccessor(세션 접근 인터페이스), NxGeometryUtils(BoundingBox 헬퍼)
│
├─ CadImportModule/        Bookmark(.plmxml) Open, Assembly Tree/Geometry/Material 읽기
│  ├─ Core/                NXOpen 비의존
│  ├─ NxBackend/           NXOpen 의존 (NxConnector, NxAssemblyReader, NxGeometryReader, NxMaterialReader)
│  └─ docs/DevGuide.md     VS/NX Open 환경설정, 디버깅, 빌드 체크리스트
│
├─ RoiModule/              ROI 지정(Face 피킹) + Geometry 추출
│  ├─ Core/                NXOpen 비의존 (IRoiResolver, IROIManager, ROISelection, ROIManager)
│  ├─ NxBackend/           NXOpen 의존 (NxRoiResolver)
│  └─ README.md            모듈 경계, 사용법, 확인 필요 API
│
└─ App/                    두 모듈을 잇는 CLI 실행 파일
   └─ src/
      ├─ main.cpp
      └─ Export/           JsonWriter, JsonSceneExporter (두 모듈의 출력을 하나의 JSON으로 결합)
```

CadImportModule과 RoiModule은 물리적으로 분리된 모듈이다: RoiModule은 CadImportModule의 **Core(안정적인 인터페이스/데이터 모델)에만** 의존하고, `NxConnector` 같은 구체 구현에는 의존하지 않는다 (`Shared/NxContracts::INxSessionAccessor`를 통한 접점). 자세한 설계 배경은 각 모듈의 README/DevGuide 참고.

## 빠른 시작

```
App.exe --bookmark <path.plmxml> [--output scene.json] [--pick-roi]
```

- 개인 PC(NX 미설치): `CadImportModule/Core`, `RoiModule/Core`만 로컬에서 빌드/테스트 가능 (실제로 검증됨)
- 회사 PC(NX2406 + Teamcenter): 전체 5개 프로젝트 빌드 - `CadImportModule/docs/DevGuide.md`의 첫 빌드 체크리스트 참고

## 이번 범위 밖

- 가상 GAP(부품 Move 기반), 광학 Property 매핑, Ray Tracing - 다른 챕터/모듈 담당
- Facet/Mesh(삼각형) 추출, ROI 좌표/Box-드래그 지정 방식 - `RoiModule/README.md` 참고
