# MIDI and Core Runtime composition

Configure this consumer independently against iiCSMIDI and iiSDKCore. The
`iiCSMIDIAsync` package exports `iiCSMIDI::Async` and `MidiAsync.h`. Its bounded
async decoder retains the caller-owned bytes and uses Core Runtime executors;
MIDI does not need to import the runtime to build its base package.
