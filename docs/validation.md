# Native migration validation

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
