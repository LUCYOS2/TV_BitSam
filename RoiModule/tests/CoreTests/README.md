# CoreTests (placeholder)

RoiModule::Core는 NXOpen에 의존하지 않으므로, 실제 NX 없이도 단위 테스트가 가능하다 (예: `ROIManager`가 선택을 올바르게 기록/보관하는지, `AddResolvedSelection`이 기존 필드를 그대로 유지하는지).

이번 패스에서는 테스트 코드를 작성하지 않았다 - 폴더만 마련해 둔다. `CadImportModule/tests/CoreTests/README.md`와 동일한 패턴(별도 실행 파일 또는 Catch2)을 따르면 된다.

NxBackend(`NxRoiResolver`)는 실제 NX 세션이 있어야 의미 있게 테스트할 수 있으므로 이 폴더의 대상이 아니다.
