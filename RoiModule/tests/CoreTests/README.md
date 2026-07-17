# CoreTests

`RoiModule::Core`(NXOpen 비의존)에 대한 단위 테스트. 외부 프레임워크 없이 `src/MiniTest.h`(헤더 하나짜리 `TEST_CASE`/`CHECK` 매크로 - 실패해도 계속 진행하고, 끝나면 실패 개수를 프로세스 종료 코드로 반환)로 작성했다.

## 실행 방법

```
MSBuild NX_RoiSelection.sln /t:RoiCoreTests /p:Configuration=Debug /p:Platform=x64
build\Debug\x64\CoreTests.exe
```

개인 PC(NX 미설치)에서도 빌드/실행 가능 - `RoiModule/Core`와 `CadImportModule/Core`만 링크한다.

## 커버리지

`src/main.cpp` 기준, 두 그룹으로 나뉜다:

**`ROIManager` (직접)**
- `SelectPoint`/`SelectFace`/`SelectComponent`: 각각 올바른 `kind`로 미해결(`resolved=false`) 선택을 기록하는지
- 여러 종류를 섞어 호출했을 때 삽입 순서가 유지되는지
- `AddResolvedSelection`: 이미 채워진 geometry 필드(`componentName`/`bodyId`/`area`/`boundingBox`/`coordinate`)를 그대로 보존하고, 다른 선택엔 손대지 않는지
- `ClearSelections`: 목록을 비우는지
- 로깅(`src/SpyLogger.h`로 캡처): `SelectFace`가 Debug 레벨로, `AddResolvedSelection`이 Info 레벨로 targetId/componentName을 포함해 로깅하는지
- `logger == nullptr`일 때 크래시하지 않는지

**`RoiPickingWorkflows` (`src/MockRoiResolver.h`로 `IRoiResolver`를 흉내 내서 검증 - App/src/main.cpp의 피킹 루프가 실제로 호출하는 오케스트레이션 로직 그 자체)**
- `TryAddBodySelectionsInBox`: 여러 Body 결과가 전부 같은 `scopeId`로 태깅되는지, 서로 다른 두 스코프가 섞이지 않는지, resolver 실패 시 아무것도 안 쌓이고 0을 반환하는지, `attachMesh` 플래그에 따라 `ExtractFacetMesh` 호출 여부가 정확히 갈리는지
- `TryAddFaceSelection`/`TryAddBoxSelections`/`TryAddPointSelection`: 성공/실패 경로, note 전달, (Face/Point 경로는) scopeId를 건드리지 않는지
- `TryAttachFacetMesh`: 성공 시 mesh만 갱신, 실패 시 selection 전체가 그대로인지

`NxRoiResolver`(NX 의존, 즉 `IRoiResolver`의 실제 구현)는 여전히 이 폴더의 대상이 아니다 - 실제 NX 세션이 있어야 의미 있게 테스트 가능하므로, 회사 PC에서 별도로 검증한다 (`docs/OfficeVerificationChecklist.md` 참고). `MockRoiResolver` 기반 테스트가 검증하는 건 "그 결과를 받았을 때 우리 오케스트레이션 코드가 올바르게 동작하는가"이지, NX 자체의 동작이 아니다.
