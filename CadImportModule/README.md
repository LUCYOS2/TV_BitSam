# CAD Import Module

BLU 빛샘 분석 툴 프로젝트의 **CAD Import** 담당 모듈. NX/Teamcenter에서 Bookmark(.plmxml)를 열어 Assembly Tree / Geometry 요약 / Material 이름을 안정적으로 읽어오는 것이 목적이며, ROI 지정과 Ray Tracing은 포함하지 않는다 (설계 배경은 `docs/DevGuide.md` 참고).

이 저장소는 `TV_BitSam/` 루트 아래 CadImportModule과 나란히 있는 **RoiModule**(ROI 지정 + Mesh/메타데이터 추출)과 물리적으로 분리되어 있다. 둘을 잇는 실행 파일은 `TV_BitSam/App/`에 있다 - 최상위 구조는 저장소 루트 `README.md` 참고.

## 배포 형태

External EXE(`TV_BitSam/App`). 이미 열려 있는 NX 창에 붙는 게 아니라 **자체 NX 세션을 새로 생성**해서 커맨드라인으로 지정한 북마크(.plmxl)를 연다.

```
App.exe --bookmark <path.plmxl> [--output scene.json] [--pick-roi]
```

`--pick-roi`(RoiModule 기능)를 주면, Assembly/Geometry/Material을 다 읽은 뒤 NX 그래픽 창에서 Face를 직접 클릭해 ROI를 지정할 수 있다. 자세한 내용은 `RoiModule/README.md` 참고.

## 폴더 구조

```
CadImportModule/
├─ Core/        NXOpen 비의존 (인터페이스, 데이터 모델, Logger)
├─ NxBackend/   NXOpen 의존 (Core 인터페이스의 실제 구현: NxConnector/NxAssemblyReader/NxGeometryReader/NxMaterialReader)
├─ docs/        DevGuide.md - VS/NX Open 환경설정, 디버깅, 확장 방법
└─ tests/       Core 단위 테스트 자리 (현재는 뼈대만, 실제 테스트는 다음 단계)
```

`NxConnector`는 `Shared/NxContracts::INxSessionAccessor`도 구현한다 - RoiModule이 이 인터페이스만 보고 세션에 접근하며, `NxConnector`를 구체 클래스로 참조하지 않는다 (모듈 경계, 자세한 내용은 `docs/DevGuide.md` 참고).

## 빌드 상태

| 프로젝트 | 필요 환경 | 현재(개인 PC) 상태 |
|---|---|---|
| Core | 없음 (순수 C++) | 실제 빌드 검증됨 (VS Build Tools, MSBuild) - 스모크 테스트 통과 |
| NxBackend | NX2406 SDK | 회사 PC 전용 |

회사 PC(NX2406 + Teamcenter)에서 이어서 빌드/검증하는 것을 전제로 작성됨. 첫 빌드 체크리스트는 `docs/DevGuide.md` 참고.

## 이번 범위 밖 (의도적으로 제외)

- ROI 지정, Face Mesh/메타데이터 추출 - **RoiModule** 담당 (`RoiModule/README.md`)
- 가상 GAP(부품 Move 기반) 로직 - 별도 섹션에서 추후 진행
- 광학 Property(반사율 등) 매핑 - Material Engine 모듈 담당
- Ray Tracing

## 인터페이스 (다른 모듈과의 데이터 경계)

`App::JsonSceneExporter`(`TV_BitSam/App/src/Export/`)가 이 모듈(Component/Geometry/Material)과 RoiModule(ROISelection)의 출력을 합쳐 JSON Scene Manifest를 만든다 - Material Engine / Ray Tracing 모듈에 제안하는 1차 인터페이스다. 이 조합 로직이 App에 있는 이유는 두 모듈 중 어느 쪽 Core에 둬도 서로를 참조하는 역방향 의존이 생기기 때문이다.
