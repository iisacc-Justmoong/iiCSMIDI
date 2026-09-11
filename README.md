# iiCSMIDI

C++20과 Qt 6.8.3 Core를 사용하는 버전 0.2.1의 동적 라이브러리이다. `MidiDocument`가 Standard MIDI File과 iiFileProvider 작성자 기록을 보유하고, `MidiFile`이 편집 내용과 메타데이터를 즉시 파일에 반영한다.

## 공개 API

```cpp
#include <iiCSMIDI.h>

const QString message = iiCSMIDI::helloWorld();
```

`[[nodiscard]] QString iiCSMIDI::helloWorld()`는 호출할 때마다 `Hello world!`를 반환한다. 기존 `helloWorld()`도 유지한다. 공개 헤더와 구현은 소스 루트에 함께 배치한다. Qt 6.8.3 Core와 iiFileProvider 0.5.0이 필요하다. Qt의 사용 및 배포 조건은 설치된 Qt 라이선스에 따른다.

## 빌드, 테스트, 설치

CMake 3.24 이상, C++20 컴파일러 및 Qt 6.8.3이 필요하다. macOS에서는 `/Volumes/Storage/Qt/6.8.3/macos`가 존재하면 자동으로 탐색 경로에 추가한다.

```sh
./install.sh
```

단독 프로젝트로 구성할 때만 기본 설치 경로를 설정하므로 `add_subdirectory()`로 포함하는 상위 프로젝트의 설치 경로는 유지한다.

스크립트는 `build/`에서 Release 빌드 및 CTest를 실행하고, 기본 경로 `~/.local/SDK/iiCSMIDI`에 설치한 뒤 `build/consumer/build/`에서 설치된 CMake 패키지만 사용하는 별도 실행 파일을 빌드하고 테스트한다. 라이브러리 테스트와 설치 소비자 테스트는 반환 문자열, C++20 컴파일 설정, Qt 6.8.3 헤더 버전과 런타임 버전을 검사한다.

설치 소비자 구성에는 현재 설치 경로의 패키지 디렉터리를 명시하므로 `INSTALL_PREFIX`를 변경해 재실행해도 이전 패키지 캐시를 사용하지 않는다.

설정은 명령행 인자 대신 환경 변수로 전달한다. `INSTALL_PREFIX`는 절대 경로여야 하며, `CMAKE_PREFIX_PATH`는 세미콜론 또는 콜론으로 구분한 추가 검색 경로를 받는다. 병렬 빌드 개수는 `CMAKE_BUILD_PARALLEL_LEVEL`로 지정하며 기본값은 2이다.

```sh
QT_PREFIX_PATH="/Volumes/Storage/Qt/6.8.3/macos" \
INSTALL_PREFIX="$HOME/.local/SDK/iiCSMIDI" \
./install.sh
```

수동 실행 시에도 빌드 디렉터리는 `build/`를 사용한다.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DCMAKE_PREFIX_PATH="/Volumes/Storage/Qt/6.8.3/macos"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release
cmake -S tests/consumer -B build/consumer/build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$HOME/.local/SDK/iiCSMIDI;/Volumes/Storage/Qt/6.8.3/macos"
cmake --build build/consumer/build --config Release
ctest --test-dir build/consumer/build -C Release --output-on-failure
```

## 설치 결과와 소비

기본 설치 경로에 `include/iiCSMIDI.h`, `lib/`의 공유 라이브러리, `lib/cmake/iiCSMIDI/`의 CMake 패키지, `share/iiCSMIDI/README.md`가 생성된다. Windows 공유 라이브러리 실행 파일은 `bin/`에 설치된다. 소비자에게 C++20 및 `Qt6::Core` 링크 요구 사항을 전달한다. Qt를 묶어서 복사하지 않으며 설치된 Qt 런타임이 필요하다. 공유 라이브러리의 설치 RPATH는 링크에 사용한 외부 라이브러리 경로를 포함한다.

```cmake
find_package(iiCSMIDI 0.2.1 CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE iiCSMIDI::iiCSMIDI)
```

`CMAKE_PREFIX_PATH`에 SDK 설치 경로와 Qt 경로를 포함한다. 빌드·테스트·설치까지만 제공하며 커밋, 원격 업로드 또는 배포 단계는 없다.

## License

SPDX-License-Identifier: AGPL-3.0-only

iiCSMIDI의 자체 작성 코드와 문서는 GNU Affero General Public License v3.0 전용으로
배포한다. 전체 조건은 [LICENSE](LICENSE)를 따른다.

Qt를 포함한 외부 라이브러리와 별도 고지가 있는 서드파티 코드는 각자의 라이선스를
유지한다. 이 프로젝트의 라이선스 선언은 해당 서드파티 라이선스를 대체하지 않는다.

## Authored Standard MIDI Files

`MidiDocument` owns strict SMF 0/1/2 bytes and an iiFileProvider 0.2 `Authorship`
value. `setFileAuthor` selects the current account; `setMidiData` validates and
replaces musical content, preserving the current ledger. Each real change
synchronously regenerates its cached JSON dump. Identical input is a no-op, and
malformed input throws before either content or metadata changes. `fromBytes`
restores file attribution but never the active editor or authentication token.

`toBytes` embeds the UTF-8 JSON as canonical base64url ASCII in a delta-zero
text meta event (`FF 01`) at the beginning of track 0. The text prefix is
`iisacc:authorship:v1:`. Standard track lengths are updated; existing event bytes,
running status, tempo, note data and timing remain unchanged. This is a normal
[SMF text event](https://midi.org/standard-midi-files), not a playback message.
Unknown authored-event versions, duplicate/misplaced records, malformed VLQs,
truncated events, missing end-of-track, unsupported status bytes and files over
64 MiB fail closed. The metadata contract itself is capped at 1 MiB. Header
extension bytes and all supported track events are retained; unknown chunk types
and non-SMF containers are rejected. Other editors may remove text metadata.

```cpp
auto file = iiCSMIDI::MidiFile::open("composition.mid");
file.edit([&](iiCSMIDI::MidiDocument &draft) {
    draft.setFileAuthor(author); // Validated iiFileProvider::FileAuthor
    draft.setMidiData(updatedSmfBytes);
    return true;
}); // Content and authorship have reached the file here.
```

`MidiFile::create` refuses an existing destination. `edit` works on a temporary
copy, writes with Qt's atomic `QSaveFile`, and commits live state only on success.
False or exceptions reject the draft. It detects an already changed external file
before replacement and rejects nested edits. Copies of the document carry no file
binding. The owner is single-threaded; external applications are not locked.

Dependency review: iiFileProvider is a required, actively maintained sibling SDK
under AGPL-3.0-only, matching iiCSMIDI. It reuses Qt Core already linked here, with
no network or authentication runtime. Qt provides JSON and atomic file I/O. The
small SMF metadata transport is this library's domain; no sequencer, synthesizer
or additional MIDI engine is introduced. Tokens remain only in runtime account
objects and never enter the authored file. Attribution is not ownership proof.

`install.sh` runs source and standalone installed-package author tests, including
lossless event round trips, no-op/rejection behavior, malformed metadata and an
external-write conflict.

## 파일 저장 소유권

0.2.1의 MidiFile은 iiFileProvider 0.5의 create/read/update에 저장을 위임한다. SMF 검증과 작성자 기록은 이 SDK에 남고, 파일 삭제는 iiFileProvider::File::remove가 수행한다. 외부 저장 충돌은 메모리 상태를 바꾸지 않고 실패한다.
