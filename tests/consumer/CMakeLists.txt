cmake_minimum_required(VERSION 3.24)
project(iiCSMIDIInstalledConsumer LANGUAGES CXX)

find_package(iiCSMIDI 0.1.0 CONFIG REQUIRED)
add_executable(iiCSMIDI_installed_consumer ../hello_world.cpp)
target_link_libraries(iiCSMIDI_installed_consumer PRIVATE iiCSMIDI::iiCSMIDI)
set_target_properties(iiCSMIDI_installed_consumer PROPERTIES CXX_EXTENSIONS OFF)

enable_testing()
add_test(NAME iiCSMIDI.installed_consumer COMMAND iiCSMIDI_installed_consumer)
