# iiCSMIDI

A dynamic library, version 0.2.1, using C++20 and Qt 6.8.3 Core. `MidiDocument` holds the Standard MIDI File and iiFileProvider authorship records. `MidiFile` immediately writes editing changes and metadata to the file.

<a id="공개-api"></a>

## Public API

```cpp
#include <iiCSMIDI.h>

const QString message = iiCSMIDI::helloWorld();
```

`[[nodiscard]] QString iiCSMIDI::helloWorld()` returns `Hello world!` every time it is called. Existing `helloWorld()` is also maintained. Public headers and implementation are placed together in the source root. Qt 6.8.3 Core and iiFileProvider 0.5.0 is required. Qt usage and distribution conditions follow the installed Qt license.

<a id="빌드-테스트-설치"></a>

## Build, test, install

CMake 3.24 or higher, C++20 compiler, and Qt 6.8.3 are required. macOS automatically adds `/Volumes/Storage/Qt/6.8.3/macos` to the search path if it exists.

```sh
./install.sh
```

When configured as a standalone project, the default installation path is set, so the installation path of the parent project included via `add_subdirectory()` is maintained.

The script runs Release build and CTest from `build/`, installs to the default path `~/.local/SDK/iiCSMIDI`, and builds and tests a separate executable that uses only the installed CMake package from `build/consumer/build/`. Library tests and installer consumer tests check return strings, C++20 compile settings, Qt 6.8.3 header version, and runtime version.

The installer consumer configuration explicitly specifies the package directory of the current installation path, so changing `INSTALL_PREFIX` and re-running does not reuse the previous package cache.

Settings are passed as environment variables instead of command-line arguments. `INSTALL_PREFIX` must be an absolute path, and `CMAKE_PREFIX_PATH` receives additional search paths separated by semicolons or colons. The number of parallel builds is specified as `CMAKE_BUILD_PARALLEL_LEVEL`, with a default of 2.

```sh
QT_PREFIX_PATH="/Volumes/Storage/Qt/6.8.3/macos" \
INSTALL_PREFIX="$HOME/.local/SDK/iiCSMIDI" \
./install.sh
```

Even in manual execution, the build directory uses `build/`.

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

<a id="설치-결과와-소비"></a>

## Installation results and consumption

At the default installation path, `include/iiCSMIDI.h`, `lib/` shared libraries, `lib/cmake/iiCSMIDI/` CMake packages, and `share/iiCSMIDI/README.md` are created. The Windows shared library executable is installed at `bin/`. Consumers are informed of C++20 and `Qt6::Core` link requirements. Qt is not bundled and copied; an installed Qt runtime is required. The shared library installation RPATH includes the paths of external libraries used during linking.

```cmake
find_package(iiCSMIDI 0.2.1 CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE iiCSMIDI::iiCSMIDI)
```

Includes `CMAKE_PREFIX_PATH` installation path and SDK path at Qt. Provides only build, test, and installation, with no commit, remote upload, or deployment stages.

## License

SPDX-License-Identifier: AGPL-3.0-only

Self-written code and documents of iiCSMIDI are distributed exclusively under the GNU Affero General Public License v3.0. The full terms follow [LICENSE](LICENSE).

External libraries including Qt and third-party code with separate notices maintain their own licenses. This project's license declaration does not replace the corresponding third-party license.

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

<a id="파일-저장-소유권"></a>

## File storage ownership

MidiFile in 0.2.1 delegates saving to create/read/update in iiFileProvider 0.5. SMF validation and authorship records remain in this SDK; file deletion is performed by iiFileProvider::File::remove. External storage conflicts fail without changing the in-memory state.

## Source layout

Implementation files and their headers live together under `src/`. Existing feature and platform subdirectories retain their responsibilities. Build configuration, tests, documentation, resources, and maintenance scripts remain at the project root. Configure and build using the repository-local `build/` directory.
