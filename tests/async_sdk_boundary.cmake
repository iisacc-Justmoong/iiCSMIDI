if(NOT MIDI_SOURCE)
    message(FATAL_ERROR "MIDI_SOURCE is required")
endif()
file(READ "${MIDI_SOURCE}/CMakeLists.txt" build_source)
file(READ "${MIDI_SOURCE}/cmake/iiCSMIDIConfig.cmake.in" package_source)
if(EXISTS "${MIDI_SOURCE}/src/MidiAsync.h" OR
   build_source MATCHES "iiSDKCore::Runtime" OR
   build_source MATCHES "COMPONENTS[ \t]+Contracts[ \t]+Runtime" OR
   package_source MATCHES "COMPONENTS[ \t]+Runtime")
    message(FATAL_ERROR "Core Runtime composition must live in the separate async consumer")
endif()
