# CAD Import Module

BLU 빛샘 분석 툴 프로젝트의 **CAD Import** 담당 모듈. NX/Teamcenter에서 Assembly Tree / Geometry 요약 / Material 이름을 안정적으로 읽어와 JSON Scene Manifest로 내보내는 것이 목적이며, Ray Tracing 기능은 포함하지 않는다 (설계 배경은 `docs/DevGuide.md` 참고).

## 배포 형태

External EXE. 이미 열려 있는 NX 창에 붙는 게 아니라 **자체 NX 세션을 새로 생성**해서 커맨드라인으로 지정한 북마크(.plmxl)를 연다.

```
CadImportModule.exe --bookmark <path.plmxl> [--output scene.json] [--pick-roi]
```

`--pick-roi`를 주면, Assembly/Geometry/Material을 다 읽은 뒤 NX 그래픽 창에서 Face를 직접 클릭해 ROI를 지정할 수 있다 - **별도 JT/Mesh 파일 없이** 그 자리에서 소속 Component/Body, BoundingBox, Area를 뽑아 `scene.json`의 `roiSelections`에 채운다. 사용자 입력을 기다리는 블로킹 단계라 opt-in으로 뒀다 (자세한 내용은 `docs/DevGuide.md` 7절).

## 폴더 구조

```
CadImportModule/
├─ Core/        NXOpen 비의존 (인터페이스, 데이터 모델, Logger, ROI 기록, JSON Export)
├─ NxBackend/   NXOpen 의존 (Core 인터페이스의 실제 구현)
├─ App/         실행 파일 (CLI 진입점)
├─ docs/        DevGuide.md - VS/NX Open 환경설정, 디버깅, 확장 방법
└─ tests/       Core 단위 테스트 자리 (현재는 뼈대만, 실제 테스트는 다음 단계)
```

## 빌드 상태

| 프로젝트 | 필요 환경 | 현재(개인 PC) 상태 |
|---|---|---|
| Core | 없음 (순수 C++) | 실제 빌드 검증됨 (VS Build Tools, MSBuild) - 스모크 테스트 통과 |
| NxBackend | NX2406 SDK | 회사 PC 전용 |
| App | NX2406 SDK (NxBackend 경유) | 회사 PC 전용 |

회사 PC(NX2406 + Teamcenter)에서 이어서 빌드/검증하는 것을 전제로 작성됨. 첫 빌드 체크리스트는 `docs/DevGuide.md` 9절 참고.

## 이번 범위 밖 (의도적으로 제외)

- Face ROI의 Area 계산 (현재 `0.0` placeholder - UF Mass-Properties 시그니처 확정 필요, `docs/DevGuide.md` 7/8절)
- Point/Component 종류의 인터랙티브 지오메트리 해석 (이번엔 Face만)
- 가상 GAP(부품 Move 기반) 로직 - 별도 섹션에서 추후 진행
- 광학 Property(반사율 등) 매핑 - Material Engine 모듈 담당
- Ray Tracing

## 인터페이스 (다른 모듈과의 데이터 경계)

`Core::JsonSceneExporter`가 만드는 JSON Scene Manifest가 Material Engine / Ray Tracing 모듈에 제안하는 1차 인터페이스다. 스키마는 `docs/DevGuide.md`와 `Core/include/Core/Export/JsonSceneExporter.h`를 참고.
