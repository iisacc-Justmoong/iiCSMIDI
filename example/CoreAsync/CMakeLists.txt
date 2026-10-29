cmake_minimum_required(VERSION 3.31)
# iisdk:consumer-application
project(iiCSMIDIAsync VERSION 0.3.1 LANGUAGES CXX)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)
find_package(iiCSMIDI 0.3 CONFIG REQUIRED)
find_package(iiSDKCore 0.14 CONFIG REQUIRED COMPONENTS Runtime)
add_library(iiCSMIDIAsync INTERFACE)
add_library(iiCSMIDI::Async ALIAS iiCSMIDIAsync)
set_target_properties(iiCSMIDIAsync PROPERTIES EXPORT_NAME Async)
target_compile_features(iiCSMIDIAsync INTERFACE cxx_std_23)
target_include_directories(iiCSMIDIAsync INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/App>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(iiCSMIDIAsync INTERFACE iiCSMIDI::iiCSMIDI iiSDKCore::Runtime)
if(BUILD_TESTING)
    add_executable(iiCSMIDI_async tests/async.cpp)
    target_link_libraries(iiCSMIDI_async PRIVATE iiCSMIDI::Async)
    add_test(NAME iiCSMIDI.Async COMMAND iiCSMIDI_async)
    set_tests_properties(iiCSMIDI.Async PROPERTIES TIMEOUT 30)
endif()
set(package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/iiCSMIDIAsync")
install(TARGETS iiCSMIDIAsync EXPORT iiCSMIDIAsyncTargets)
install(EXPORT iiCSMIDIAsyncTargets NAMESPACE iiCSMIDI:: DESTINATION "${package_dir}")
install(FILES App/MidiAsync.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
configure_package_config_file(cmake/iiCSMIDIAsyncConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/iiCSMIDIAsyncConfig.cmake" INSTALL_DESTINATION "${package_dir}")
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/iiCSMIDIAsyncConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/iiCSMIDIAsyncConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/iiCSMIDIAsyncConfigVersion.cmake" DESTINATION "${package_dir}")
