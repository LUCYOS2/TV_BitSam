# CoreTests (placeholder)

CadImportModule::Core는 NXOpen에 의존하지 않으므로, 실제 NX 없이도 단위 테스트가 가능하다 (예: 데이터 모델의 기본값, `ConsoleLogger`의 출력 포맷).

이번 패스에서는 테스트 코드를 작성하지 않았다 - 폴더만 마련해 둔다. 다음 단계에서 추가할 때는:
- 별도 `.vcxproj`(ConfigurationType=Application)로 Core를 링크하는 실행 파일을 만들거나
- 가벼운 테스트 프레임워크(예: Catch2 단일 헤더) 도입을 검토

NxBackend/App은 실제 NX 세션이 있어야 의미 있게 테스트할 수 있으므로 이 폴더의 대상이 아니다. `ROIManager`/`JsonSceneExporter` 관련 테스트는 각각 `RoiModule/tests/`, `App`(현재 테스트 폴더 없음)에 속한다.
