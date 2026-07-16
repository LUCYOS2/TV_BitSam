# CAD Import Module - 개발 가이드

이 문서는 회사 PC(NX2406 + Teamcenter 설치 환경)에서 이 모듈을 처음 빌드/디버깅할 때 필요한 절차를 정리한 것이다. 이 저장소는 NX/Teamcenter가 없는 개인 PC에서 작성되었으므로, 아래 내용 중 "확인 필요"로 표시된 부분은 실제 NX2406 SDK 헤더로 최종 대조해야 한다.

ROI 지정(Face 피킹, Mesh/메타데이터 추출) 관련 내용은 이 문서가 아니라 `RoiModule/README.md`에 있다 - 이 모듈은 도면 Import(Bookmark Open, Tree/Geometry/Material)만 다룬다.

## 1. 전체 저장소 구조와 이 모듈의 위치

```
TV_BitSam/
├─ TV_BitSam.sln              (전체 5개 프로젝트를 묶는 솔루션 - 루트에 위치)
├─ Shared/                    (두 모듈이 공유하는 빌드 설정 + 얇은 인터페이스)
│  ├─ NxOpenSdk.props
│  └─ NxContracts/            (INxSessionAccessor, NxGeometryUtils)
├─ CadImportModule/           (이 모듈)
│  ├─ Core/                   NXOpen 비의존
│  └─ NxBackend/              NXOpen 의존
├─ RoiModule/                 (ROI 지정 + Mesh/메타데이터 추출 - 별도 모듈)
│  ├─ Core/
│  └─ NxBackend/
└─ App/                       (두 모듈을 잇는 CLI 실행 파일 + JSON Export)
```

Core는 어떤 NXOpen 헤더도 포함하지 않는다. NxBackend/App만 NX Open SDK가 필요하다. 이 경계를 깨지 말 것(예: Core 헤더에 `#include <NXOpen/...>`을 추가하지 말 것) - 버전 업 대응력과 팀 협업(다른 담당자는 NXOpen 몰라도 Core만 보고 인터페이스 이해 가능)의 핵심 전제다.

**모듈 간 경계**: `NxConnector`(이 모듈)는 `Shared/NxContracts::INxSessionAccessor`를 구현한다. RoiModule의 `NxRoiResolver`는 `NxConnector`를 구체 클래스로 참조하지 않고 이 인터페이스만 본다 - RoiModule이 이 모듈의 구현 세부사항(예: `NxConnector`의 시그니처 변경)에 영향받지 않도록 하기 위함이다.

## 2. 프로젝트 생성 / Visual Studio 설정

1. **NX Command Prompt에서 Visual Studio를 실행하는 것이 Siemens 권장 방식**이다. NX 설치 폴더에서 제공되는 명령 프롬프트(또는 NX 실행 스크립트가 설정하는 환경)를 통해 VS를 띄우면 `UGII_BASE_DIR` 등 필요한 환경변수를 자동으로 상속받는다. 일반 시작 메뉴에서 VS를 바로 실행하면 이 환경변수가 없어 빌드가 실패할 수 있다.
2. 저장소 루트의 `TV_BitSam.sln`을 위 방식으로 연 VS에서 오픈한다 (CadImportModule 폴더 안에는 더 이상 솔루션 파일이 없다). Solution Explorer에 "CadImportModule"/"RoiModule" 두 개의 솔루션 폴더 아래 각각 Core/NxBackend가, 그 옆에 App이 보인다.
3. 플랫폼은 `x64`, 구성은 `Debug`/`Release` 두 가지가 기본 제공된다.

### Include / Library 경로

- Include: `%UGII_BASE_DIR%\UGOPEN\include`
- Library: `%UGII_BASE_DIR%\UGOPEN`

이 경로들은 `Shared/NxOpenSdk.props`(저장소 루트의 `Shared/` 폴더)에 정의되어 있고, 이 모듈의 `NxBackend.vcxproj`뿐 아니라 `RoiModule/NxBackend.vcxproj`, `App.vcxproj`도 이를 import한다. `Core.vcxproj`는 이 props를 import하지 않는다(의도적).

**확인 필요**: `NxOpenSdk.props`의 `AdditionalDependencies`에 넣어둔 `libNXOpenCpp.lib`, `libNXOpenCppUI.lib`, `libufun.lib`는 여러 NX 버전 문서에서 공통적으로 언급되는 이름이지만, NX2406 설치본의 `%UGII_BASE_DIR%\UGOPEN` 폴더에서 실제 파일명을 확인 후 필요하면 교체할 것.

### UGII_BASE_DIR이 자동으로 안 잡힐 경우

`NxOpenSdk.props`는 `UGII_BASE_DIR`이 비어 있으면 `NXOPEN_SDK_ROOT`라는 이름의 환경변수를 대신 사용하도록 되어 있다. 시스템 환경변수(제어판 > 시스템 > 환경 변수)에 `NXOPEN_SDK_ROOT`를 NX 설치 경로로 수동 등록해도 된다.

## 3. Internal Application vs External Application (선택한 방식: External)

| | Internal (NX가 로드하는 DLL) | External (독립 EXE) - **이 프로젝트가 선택한 방식** |
|---|---|---|
| 실행 주체 | NX 메뉴/툴바에서 실행, NX 프로세스 내부 | 별도 프로세스, 사용자가 EXE를 직접 실행 |
| 세션 | 이미 열려 있는 인터랙티브 세션에 바로 접근 | `NXOpen::Session::GetSession()`으로 **이 프로세스 자신의** 새 세션을 얻음 |
| 등록 | 메뉴 XML 등록 필요, `UGII_USER_DIR`에 배치 | 등록 불필요, EXE만 실행하면 됨 |
| 디버깅 | VS를 `nx.exe` 프로세스에 Attach | EXE를 그냥 F5로 디버깅 (아래 4번 참고) |
| 이 프로젝트 | (미사용) | `TV_BitSam/App/src/main.cpp`가 진입점. 자동화/Batch 재사용에도 유리하다는 이유로 채택 |

**중요**: `NxConnector::Connect()`는 이미 화면에 떠 있는 다른 NX 창의 세션에 개입하지 않는다. 항상 자기 프로세스 소유의 새 세션을 만들고, 그 세션에 북마크(.plmxl)로 지정된 파트를 연다.

## 4. 디버깅 방법

External EXE이므로 절차가 단순하다:
1. `App` 프로젝트를 시작 프로젝트로 설정 (솔루션 탐색기에서 우클릭 > Set as Startup Project)
2. 디버그 인자 설정: 프로젝트 속성 > Debugging > Command Arguments에 `--bookmark <테스트용 plmxl 경로> --output scene.json` 입력
3. F5로 바로 디버깅 시작 (Internal 방식과 달리 별도 프로세스에 Attach할 필요 없음)
4. NXOpen 관련 예외는 각 NxBackend 클래스의 `catch (const NXOpen::NXException& ex)` 블록에서 잡아 `ILogger`로 출력하므로, 콘솔 로그에서 1차적으로 원인을 확인할 수 있다.

## 5. 빌드 상태 요약

| 프로젝트 | NX 필요 여부 | 개인 PC에서 빌드 가능? |
|---|---|---|
| CadImportModule/Core | 불필요 | **가능 - 실제로 검증됨.** 개인 PC에 Visual Studio Build Tools(v145 toolset)가 설치되어 있어, MSBuild로 실제 빌드했고(경고/오류 0개) 스모크 테스트도 통과했다. |
| RoiModule/Core | 불필요 | 가능 (CadImportModule/Core와 같은 이유) |
| CadImportModule/NxBackend, RoiModule/NxBackend, App | 필요 | 불가능 - 회사 PC 전용 (NX2406 SDK 헤더/라이브러리가 있어야 컴파일 자체가 됨) |

두 Core 프로젝트를 로컬에서 다시 빌드하려면 NX Command Prompt가 필요 없다(NX 비의존이므로) - 저장소 루트에서 `msbuild CadImportModule\Core\Core.vcxproj /p:Configuration=Debug /p:Platform=x64`, `msbuild RoiModule\Core\Core.vcxproj /p:Configuration=Debug /p:Platform=x64`로 충분하다. `PlatformToolset`은 `v143`(VS2022)으로 고정해 두었으니, 설치된 버전이 다르면 `/p:PlatformToolset=<설치된 버전>`으로 오버라이드하거나 VS의 "Retarget Solution"을 사용할 것.

## 6. 예제 코드 위치 (이 모듈만)

| 기능 | 위치 |
|---|---|
| Session 연결 + 북마크 Open | `NxBackend/src/NxConnector.cpp` |
| Work Part 읽기 | `NxConnector::GetSessionInfo()` |
| Assembly 읽기 / Component 순회 | `NxBackend/src/NxAssemblyReader.cpp` (`BuildComponentInfo` 재귀 함수) |
| Geometry 정보 읽기 (Body/Face/Edge/BoundingBox) | `NxBackend/src/NxGeometryReader.cpp` (BoundingBox 로직은 `Shared/NxContracts/src/NxGeometryUtils.cpp`와 공유) |
| Material 읽기 | `NxBackend/src/NxMaterialReader.cpp` |

ROI 피킹(`NxRoiResolver`)과 JSON Export(`JsonSceneExporter`)는 각각 `RoiModule/NxBackend/`와 `TV_BitSam/App/src/Export/`로 이동했다 - `RoiModule/README.md` 참고.

## 7. 회사 PC로 옮긴 후 첫 빌드 체크리스트

- [ ] NX Command Prompt로 VS 실행 → `echo %UGII_BASE_DIR%`로 경로 확인
- [ ] 저장소 루트의 `TV_BitSam.sln` 오픈 (5개 프로젝트: CadImportModule::Core/NxBackend, RoiModule::Core/NxBackend, App)
- [ ] `Shared/NxOpenSdk.props`의 `AdditionalDependencies` 라이브러리 파일명이 실제 `%UGII_BASE_DIR%\UGOPEN` 폴더 내용과 일치하는지 확인
- [ ] `CadImportModule/Core`, `RoiModule/Core` 빌드 (NX 없이도 성공해야 정상 - 개인 PC에서 이미 검증됨)
- [ ] `CadImportModule/NxBackend` 빌드 → 컴파일 에러 발생 시 `NxAssemblyReader::BuildComponentInfo`의 Visibility/Suppression 접근자, `NxGeometryReader`/`NxGeometryUtils`의 UF BoundingBox 함수명, `NxMaterialReader`의 `PhysicalMaterial::GetBodies()`부터 실제 헤더와 대조
- [ ] `RoiModule/NxBackend` 빌드 → 컴파일 에러 시 `RoiModule/README.md`의 "확인 필요" API 목록부터 대조
- [ ] `App` 빌드
- [ ] 가상 북마크(.plmxl) 또는 테스트용 파트로 `App.exe --bookmark <path>` 실행하여 콘솔 출력과 `scene.json` 생성 확인
- [ ] `App.exe --bookmark <path> --pick-roi` 실행 → NX 그래픽 창에서 Face 클릭 → `scene.json`의 `roiSelections`에 `resolved:true`와 실제 좌표/Body 정보가 채워지는지 확인
