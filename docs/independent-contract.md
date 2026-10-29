# 독립 MIDI 문서와 앱 조합 계약

`iiCSMIDI`는 C++23 표준 타입으로 SMF 데이터와 편집 상태를 다룬다.
다른 SDK의 헤더·타깃·설치 순서 또는 파일 생성 구현을 요구하지 않는다.
파일 생성과 변경은 애플리케이션이 주입한 `MidiStorage`에 위임한다.
인터페이스는 표준 컨테이너와 가상 함수를 사용하는 C++ ABI이므로 같은 앱에서
조합하는 바이너리는 호환되는 컴파일러·표준 라이브러리·런타임을 사용해야 한다.

## 파일과 편집

`MidiFile::create(path, storage, document)`와 `open(path, storage, factory)`는
공유 소유권으로 저장 구현의 수명을 유지한다. 생성은 기존 대상을 보존해야 하고
`update`는 이전에 확정한 바이트가 현재 내용과 맞는지 확인한 뒤 게시해야 한다.
편집이 거부되거나 콜백·저장 작업이 예외를 던지면 문서의 확정 상태는 변하지
않는다. 중첩 편집과 이동된 파일의 편집은 거부한다. 이 보장은 저장 구현이
계약을 지킬 때 성립하며 임의의 외부 프로세스를 잠그는 분산 CAS는 아니다.

## 저작자 메타데이터

저작자 이벤트가 없는 일반 SMF는 어댑터 없이 읽을 수 있다. 저작자 이벤트가
있으면 `AuthorshipFactory::fromDump`를 제공해야 하며, 검증 실패 또는 반환된
직렬화 바이트가 원본과 다르면 읽기를 거부한다. `Authorship::clone`은 독립
상태를 반환해야 한다. `dump`의 뷰는 해당 상태가 변경되거나 파괴될 때까지
유효해야 한다. 작성자는 `setFileAuthor`에 직렬화 프로필을 제공하고, 팩토리가
이를 검증한다. SDK는 임의의 팩토리가 안전한 JSON 또는 자격 증명 제거 정책을
적용한다고 가정하지 않는다. 전체 파일은 64 MiB, 저작자 정보는 2 MiB로 제한한다.

`example/FileProvider`는 iiJson의 제한된 JSON 검증과 iiFileProvider의 기존
저작자 스키마·파일 작업을 조합한다. 원래 `iisacc:authorship:v1:` 이벤트와
기존 fixture를 보존하고 파일 충돌·덮어쓰기·프리뷰 요청을 처리한다. 이 어댑터는
SDK 빌드에 포함되지 않는다. 기존 `FileAuthor` 인자, provider 소유 저작자 반환형,
암묵적 파일 저장 호출은 새 계약으로 교체하고 소비자를 다시 빌드해야 한다.

## 비동기 조합

`example/CoreAsync`의 `iiCSMIDIAsync` 패키지는 앱에서 MIDI와 Core Runtime을
조합해 기존 `iiCSMIDI::Async` 타깃을 제공한다. `decodeAsync`는 입력을 소유하며
저작자 이벤트가 있는 파일은 팩토리를 함께 전달해야 한다. 작업 취소와 결과
전달 수명은 Core Runtime 계약에 따른다. 이전 `iiCSMIDI`의 Async 컴포넌트는
필수 요청 시 이전 안내와 함께 실패한다.
