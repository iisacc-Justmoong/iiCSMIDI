# iiCSMIDI

[Windows build and execution](docs/WindowsBuildAndRun.md)

The authorship storage test uses an explicit UTF-8 filesystem path for its Korean
and emoji filename on Windows. Contract failures report their check number and
original exception in the test log.

Package release: **0.3.1** (2026-10-19). This patch packages the current
source snapshot; build, test and installation evidence is recorded in the SDK
patch-release report. Earlier versioned feature notes remain historical.


iiCSMIDI 0.3.1 is a native C++23 Standard MIDI File document library. Its public
API, implementation and installed base package are Qt-free and do not depend on
other SDK modules. Applications inject author validation and storage interfaces;
the separately configured consumers combine iiFileProvider, iiJson or Core Runtime.
It does not implement a
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
```

For authored file editing, configure the separate `example/FileProvider` consumer
and supply its provider adapters from the application:

```cpp
#include "FileProviderAdapter.h"
auto factory = std::make_shared<midi_file::NativeAuthorshipFactory>();
auto storage = std::make_shared<midi_file::NativeStorage>();
iiCSMIDI::MidiDocument authored(factory);
authored.setFileAuthor(author.toJsonText()); // validated FileAuthor supplied by the app
auto file = iiCSMIDI::MidiFile::create("composition.mid", storage, authored);
file.edit([&](iiCSMIDI::MidiDocument& draft) {
    draft.setMidiData(updatedSmfBytes);
    return true;
}); // Storage publication completed before returning.
```

The independent API uses `std::vector<std::byte>`, `std::span<const std::byte>`
and `std::filesystem::path`. `MidiFile` requires a caller-supplied `MidiStorage`;
`setFileAuthor` accepts a serialized author profile through an `AuthorshipFactory`.
`authorship()` returns the module's abstract state, not a provider-specific type.
Existing source and binary consumers must migrate and rebuild.

MidiDocument owns strict SMF 0/1/2 bytes and an optional caller-provided authorship
state. Empty construction produces a valid minimal format-0 file. setMidiData
validates before changing music/metadata; the provided Native adapter validates
credential-free account profiles.
Real edits immediately advance the ledger; no-op edits do not. fromBytes restores
attribution without selecting an active editing identity. Borrowed getters require
an lvalue owner and cannot be taken from temporaries.

Authorship uses the existing `iisacc:authorship:v1:` base64url envelope in a
delta-zero FF 01 text meta event at the beginning of track zero. SMF track lengths
are updated; timing, unrelated event bytes, running status, SysEx, header
extensions and SMPTE timing are retained. Unknown versions, misplaced/duplicate
envelopes, noncanonical base64url, malformed VLQs, truncated tracks and
invalid/missing end-of-track fail closed. Authored input requires a caller-supplied
validator: the Native adapter rejects invalid UTF-8/JSON, duplicate keys and
invalid metadata under its existing schema 1..3 contract. The factory must preserve
the imported serialized bytes exactly. Whole files are capped at 64 MiB and
authorship at 2 MiB. New output uses Native's serialization, not iiJson's writer.

MidiFile owns a single-threaded edit binding. Documents copied from it are
snapshots without storage bindings. Empty/nested edits fail. A false/throwing edit
leaves memory and storage untouched. The Native storage adapter refuses an existing
destination and checks previously committed bytes before atomic replacement; live
state is committed only after successful publication. External applications are
not locked, so this is optimistic conflict detection, not an interprocess CAS.
Native exclusive publication uses same-directory hard links and explicitly fails
on filesystems without this capability. Presentation metadata/previews are queued
by the provider for visible files; unsupported media receives an information card.

```cmake
find_package(iiCSMIDIAsync 0.3 CONFIG REQUIRED)
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
Configure `example/CoreAsync` separately against the installed iiCSMIDI and
iiSDKCore packages, then install that consumer's `iiCSMIDIAsync` package. The
former `iiCSMIDI` Async component is unavailable and its callers must switch to
the new package and rebuild. Pass an authorship factory to `decodeAsync` when
decoding files containing authorship metadata.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON -DPython3_EXECUTABLE=/usr/bin/python3 \
  -DCMAKE_INSTALL_PREFIX="$PWD/build/install"
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
cmake --install build
```

`install.sh` supports INSTALL_PREFIX/CMAKE_PREFIX_PATH and uses only build/ for
build artifacts. It tests the installed base consumer. Set
`IICSMIDI_TEST_FILE_PROVIDER=ON` to test the provider/JSON composition and
`IICSMIDI_INSTALL_ASYNC=ON` to build, test and install the separate Async
consumer. These opt-in consumers require their named SDKs. Python is a
build-time boundary check only. Shared native builds use platform export macros;
iOS/WASM select a static library. These build rules are not target-execution
evidence; see [validation](docs/validation.md) for tested platforms.

The legacy Qt 6.8.3 authored-file fixtures were captured before replacing the
implementation and must round-trip byte-for-byte. Tests also preserve the original
SMF validation/storage assertions, enumerate truncated inputs, check draft and
external conflict preservation, and exercise actual Native presentation. See
[fixture provenance](tests/fixtures/README.md). Attribution is not ownership proof.

Original code/documents are MIT. See LICENSE. No Qt implementation or
code is copied. Public headers and implementations live together under src/.

## License

SPDX-License-Identifier: MIT

The original code and documentation in this repository are licensed under the
[MIT License](LICENSE). Third-party code, libraries, fonts, tools, and model
weights retain their respective licenses.

<!-- workspace-docs:readme:start -->
## Documentation entry points

- Human overview: this English `README.md`.
- Agent instructions: [AGENT.md](AGENT.md).
- Human documentation: [documentation index](docs/README.md).
- Structured agent documentation: [agent index](docs/agent-index.json).
<!-- workspace-docs:readme:end -->
