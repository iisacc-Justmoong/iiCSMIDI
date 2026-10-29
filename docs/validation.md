# Native migration validation (historical 0.3.0)

The implementation is 0.3.0, using Core 0.14.0, Native 0.8.0 and iiJson 0.1.0.
The host is macOS arm64. Source/build/package checks explicitly disable Qt6.
Public headers, implementation and package templates have a mandatory boundary
check that also forbids direct file-creation backends in MIDI production code.

Source tests cover helloWorld/C++23, native authorship and SMF compatibility,
transactional Native storage and optional asynchronous decoding. Separate
installed base and Async consumers use staged packages beneath build/native-p0.
The base import asserts that neither Qt nor Core Runtime was imported. Tests use
visible disposable directories beneath each build to exercise provider automatic
presentation; provider-private hidden temporary files are intentionally excluded.

The fixed Qt fixture covers byte-preserving legacy authored SMF and metadata,
not every possible third-party SMF. The journal fixture checks only its preserved
bytes/hash. Core/iiJson suites exercise grammar, bounds, lifetime and cancellation
races separately. The aggregate P0 verification report in
../../iiSDKCore/build/p0-verification.json records final suite results and binary
dependencies. Nothing in this migration establishes Windows/Linux/mobile device
execution; native package and WASM codec evidence are recorded separately.
# 2026-10-28 SDK 독립과 앱 조합 분리

`iiCSMIDI` 본체의 선택 Async 컴포넌트와 Core Runtime 연결을 제거했다. 기존
`MidiAsync.h` 및 비동기 검사는 별도 `example/CoreAsync` 소비자로 이동하고,
새 `iiCSMIDIAsync` 패키지가 기존 `iiCSMIDI::Async` 타깃을 제공한다. 기존
소비자는 패키지 검색을 변경하고 다시 빌드해야 한다. MIDI 본체의 Core Contracts,
iiFileProvider Native, iiJson 연결도 제거했다. 저작자 검증과 저장은 호출자가
제공하는 인터페이스이며, `example/FileProvider`가 기존 구현을 조합한다.

macOS arm64에서 다른 SDK 검색을 비활성화한 본체 빌드와 5개 검사가 통과했다.
별도 파일 제공자 소비자의 기존 저작자·SMF fixture·충돌·프리뷰 검사가 통과했고,
별도 비동기 소비자의 기존 30초 검사도 통과했다. 기존 Async 컴포넌트를 필수와
선택으로 요청하는 패키지 구성 검사를 각각 확인했으며, 본체와 새 Async 패키지의
설치 소비자는 컴파일만 확인했다. 실제 애플리케이션과 Windows·Linux 실행은 이번에
확인하지 않았다. 격리된 새 설치본에는 이전 버전의 헤더·타깃이 남지 않는다.
기존 설치 경로를 재사용하면 이전 파일이 남아 있을 수 있으므로 해당 파일의
정리는 앱별 사용 여부를 확인한 뒤 수행한다.
