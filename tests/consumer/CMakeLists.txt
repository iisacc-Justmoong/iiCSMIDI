cmake_minimum_required(VERSION 3.24)
project(iiCSMIDIInstalledConsumer LANGUAGES CXX)

set(CMAKE_DISABLE_FIND_PACKAGE_Qt6 TRUE)
find_package(iiCSMIDI 0.3.0 CONFIG REQUIRED)
if(TARGET Qt6::Core OR TARGET iiSDKCore::iiSDKCore)
    message(FATAL_ERROR "Base MIDI consumer imported GUI/runtime dependencies")
endif()
add_executable(iiCSMIDI_installed_consumer ../hello_world.cpp)
target_link_libraries(iiCSMIDI_installed_consumer PRIVATE iiCSMIDI::iiCSMIDI)
set_target_properties(iiCSMIDI_installed_consumer PROPERTIES CXX_EXTENSIONS OFF)

enable_testing()
add_test(NAME iiCSMIDI.installed_consumer COMMAND iiCSMIDI_installed_consumer)

add_executable(iiCSMIDI_installed_authorship ../authorship.cpp)
target_link_libraries(iiCSMIDI_installed_authorship PRIVATE iiCSMIDI::iiCSMIDI)
add_test(NAME iiCSMIDI.installed_authorship COMMAND iiCSMIDI_installed_authorship "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures")
