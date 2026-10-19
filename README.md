# iiCSMIDI

[Windows 빌드 및 실행](docs/WindowsBuildAndRun.md)

The authorship storage test uses an explicit UTF-8 filesystem path for its Korean
and emoji filename on Windows. Contract failures report their check number and
original exception in the test log.

Package release: **0.3.1** (2026-10-19). This patch packages the current
source snapshot; build, test and installation evidence is recorded in the SDK
patch-release report. Earlier versioned feature notes remain historical.


iiCSMIDI 0.3.1 is a native C++23 Standard MIDI File document library. Its public
API, implementation and installed base package are Qt-free. It depends on
iiSDKCore 0.14 Contracts, iiFileProvider Native 0.8 and the separate iiJson 0.1
codec. The optional Async component adds Core Runtime. It does not implement a
sequencer, synthesizer or live MIDI/UMP transport; those device responsibilities
remain in the independent iiDeviceMultimedia module.

```cmake
find_package(iiCSMIDI 0.3 CONFIG REQUIRED)
target_link_libraries(app PRIVATE iiCSMIDI::iiCSMIDI)
```

```cpp
#include <iiCSMIDI.h>
iiCSMIDI::MidiDocument document;
auto bytes = document.toBytes(); // std::vector<std::byte>
auto restored = iiCSMIDI::MidiDocument::fromBytes(bytes);
auto file = iiCSMIDI::MidiFile::create("composition.mid", restored);
file.edit([&](iiCSMIDI::MidiDocument& draft) {
    draft.setFileAuthor(author); // iiFileProvider::native::FileAuthor
    draft.setMidiData(updatedSmfBytes);
    return true;
}); // Provider publication completed before returning.
iiFileProvider::native::File::remove(file.path());
```

The 0.3 source/ABI migration replaces QByteArray with `ii::core::Bytes` and
`ByteView`, QString paths with `std::filesystem::path`, and Qt authorship objects
with the existing Native types. GUI consumers perform conversions in their own
Qt adapter. `helloWorld()` now returns std::string. Core is not coupled back to
this module or the file provider. No matching source consumer was found outside
this module during the scoped SDK/Product source audit.

MidiDocument owns strict SMF 0/1/2 bytes and a Native Authorship value. Empty
construction produces a valid minimal format-0 file. setMidiData validates before
changing music/metadata; setFileAuthor selects a credential-free account snapshot.
Real edits immediately advance the ledger; no-op edits do not. fromBytes restores
attribution without selecting an active editing identity. Borrowed getters require
an lvalue owner and cannot be taken from temporaries.

Authorship uses the existing `iisacc:authorship:v1:` base64url envelope in a
delta-zero FF 01 text meta event at the beginning of track zero. SMF track lengths
are updated; timing, unrelated event bytes, running status, SysEx, header
extensions and SMPTE timing are retained. Unknown versions, misplaced/duplicate
envelopes, noncanonical base64url, invalid UTF-8/JSON, duplicate JSON keys, malformed
VLQs, truncated tracks and invalid/missing end-of-track fail closed. Whole files
are capped at 64 MiB. Metadata uses Native's existing schema 1..3 contract and
2 MiB limit; the bounded JSON parser adds depth/item/string limits. New output
continues to use Native's authored metadata serialization, not iiJson's writer.

MidiFile owns a single-threaded edit binding. Documents copied from it are
snapshots without storage bindings. Empty/nested edits fail. A false/throwing edit
leaves memory and disk untouched. Create refuses an existing destination; edit
checks the previously committed bytes before atomic replacement; live state is
committed only after successful provider publication. External applications are
not locked, so this is optimistic conflict detection, not an interprocess CAS.
Native exclusive publication uses same-directory hard links and explicitly fails
on filesystems without this capability. Presentation metadata/previews are queued
by the provider for visible files; unsupported media receives an information card.

```cmake
find_package(iiCSMIDI 0.3 CONFIG REQUIRED COMPONENTS Async)
target_link_libraries(app PRIVATE iiCSMIDI::Async)
```

```cpp
#include <MidiAsync.h>
auto operation = iiCSMIDI::decodeAsync(owner, runtime, std::move(bytes));
operation.then(replyQueue, receiverScope.sender(), [](auto result) {
    if (result->value) { /* read the owned document */ }
});
```

The async decoder owns its buffer, reuses Runtime/SerialQueue workers, checks
interruption before and after bounded SMF parsing, and reports pure decoding with
no external side effects. It does not interrupt the parser mid-event; owner close
waits for active parsing/capture cleanup. Reply placement, waiting, cancellation,
deadline and owner/executor removal follow [Core's tracked work contract](../iiSDKCore/docs/operation.md).
Fixed thread APIs can use an existing Thread + EventLoop instead of a SerialQueue.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="/path/to/iiSDKCore;/path/to/iiFileProviderNative;/path/to/iiJson" \
  -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON -DPython3_EXECUTABLE=/usr/bin/python3 \
  -DCMAKE_INSTALL_PREFIX="$PWD/build/install"
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
cmake --install build
```

`install.sh` supports INSTALL_PREFIX/CMAKE_PREFIX_PATH and uses only build/ for
build artifacts. It also tests independent installed base and Async consumers.
The base package imports Contracts without importing Core Runtime. Python is a
build-time boundary check only. Shared native builds use platform export macros;
iOS/WASM select a static library. These build rules are not target-execution
evidence; see [validation](docs/validation.md) for tested platforms.

The legacy Qt 6.8.3 authored-file fixtures were captured before replacing the
implementation and must round-trip byte-for-byte. Tests also preserve the original
SMF validation/storage assertions, enumerate truncated inputs, check draft and
external conflict preservation, and exercise actual Native presentation. See
[fixture provenance](tests/fixtures/README.md). Attribution is not ownership proof.

Original code/documents are AGPL-3.0-only. See LICENSE. No Qt implementation or
code is copied. Public headers and implementations live together under src/.
