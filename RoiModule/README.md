# ROI Module

BLU 빛샘 분석 툴 프로젝트의 **ROI 지정 + Mesh/메타데이터 추출** 담당 모듈. `CadImportModule`이 열어놓은 NX 세션 위에서, 사용자가 지정한 ROI(현재는 Face 클릭)의 실제 지오메트리 데이터를 **별도 JT/Mesh 파일 없이** 살아있는 세션에서 바로 뽑아낸다.

`CadImportModule`과는 물리적으로 분리된 별도 모듈이다 - 이 문서와 `CadImportModule/docs/DevGuide.md`를 함께 참고할 것.

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
├─ Core/         NXOpen 비의존 - IRoiResolver, IROIManager, ROISelection, ROIManager
├─ NxBackend/    NXOpen 의존 - NxRoiResolver (인터랙티브 Face 피킹 + Geometry 추출)
└─ tests/        Core 단위 테스트 자리 (현재는 뼈대만)
```

## 사용법

`RoiModule` 자체는 실행 파일이 없다 - `TV_BitSam/App`이 이 모듈과 `CadImportModule`을 함께 링크해서 실행한다.

```
App.exe --bookmark <path.plmxl> --pick-roi
```

조립체 트리/Geometry/Material을 다 읽은 뒤, 콘솔에 "Pick another ROI face? [y/n]"가 뜨면 `y` 입력 → NX 그래픽 창에서 Face를 클릭 → 자동으로 `ROISelection`에 결과(소속 Component/Body, BoundingBox, Area)가 채워져 저장됨 → `n`을 입력하면 종료하고 JSON Export로 진행.

**주의(블로킹)**: `NxRoiResolver::PromptSelectFace`는 사용자가 클릭할 때까지 대기하는 동기 호출이다. 완전 자동/무인 배치 처리와는 상충되므로, `--pick-roi`를 안 주면 완전 자동으로 끝까지 실행된다 (opt-in).

## 이번 범위 밖 / 아직 결정 안 됨

- **ROI 지정 방식**: 현재는 Face 클릭 한 가지뿐. 좌표 입력 방식과 Box 드래그(다중 선택) 방식도 검토 중이나 아직 미확정 - 결정되면 `IRoiResolver`에 새 메서드를 추가하는 방식으로 확장 (OCP, 기존 `PromptSelectFace`는 유지).
- **Face Area 계산**: `ROISelection::area`가 아직 `0.0` placeholder (`NxRoiResolver::BuildSelectionFromFace` 참고) - 아래 "확인 필요" API 참고.
- **Facet/Mesh(삼각형) 추출**: 지금은 BoundingBox/Area 같은 요약 메타데이터만 뽑는다. Ray Tracing이 실제로 쓸 삼각형 메쉬(정점+법선) 추출은 다음 단계.
- Point/Component 종류의 인터랙티브 지오메트리 해석 (이번엔 Face만).

## 확인 필요한 API (회사 PC에서 NX2406 헤더로 대조)

| 위치 | 확인해야 할 것 |
|---|---|
| `NxRoiResolver::PromptSelectFace` | `NXOpen::UI::GetUI()`, `ui->SelectionManager()` 정확한 접근자명/반환 타입 |
| 〃 | `Selection::SelectTaggedObject`의 정확한 오버로드(파라미터 개수/순서) - NX8에서 `SelectObject`/`SelectObjects`가 Deprecated되고 이걸로 대체됨은 확인됨 |
| 〃 | `Selection::MaskTriple`의 필드명 (`Type`/`Subtype`/`SolidBodySubtype`으로 가정) |
| `NxRoiResolver::BuildSelectionFromFace` | Face Area 추출용 UF Mass-Properties 함수 시그니처 (현재 `area = 0.0` placeholder) |

## 빌드 상태

| 프로젝트 | NX 필요 여부 | 개인 PC에서 빌드 가능? |
|---|---|---|
| Core | 불필요 | 가능 - `CadImportModule/Core`와 함께 MSBuild로 검증됨 |
| NxBackend | 필요 | 불가능 - 회사 PC 전용 |
