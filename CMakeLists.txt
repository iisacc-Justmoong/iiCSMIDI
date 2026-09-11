cmake_minimum_required(VERSION 3.24)
project(iiCSMIDI VERSION 0.2.1 LANGUAGES CXX)

if(PROJECT_IS_TOP_LEVEL AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
        set(_default_home "$ENV{HOME}")
    elseif(DEFINED ENV{USERPROFILE} AND NOT "$ENV{USERPROFILE}" STREQUAL "")
        set(_default_home "$ENV{USERPROFILE}")
    else()
        message(FATAL_ERROR "Set CMAKE_INSTALL_PREFIX when HOME and USERPROFILE are unavailable")
    endif()
    set(CMAKE_INSTALL_PREFIX "${_default_home}/.local/SDK/iiCSMIDI" CACHE PATH "Install prefix" FORCE)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)
list(PREPEND CMAKE_PREFIX_PATH "$ENV{HOME}/.local/SDK/iiFileProvider")
find_package(iiFileProvider 0.5.0 CONFIG REQUIRED)
find_package(Qt6 6.8.3 EXACT REQUIRED COMPONENTS Core)

add_library(iiCSMIDI SHARED iiCSMIDI.h iiCSMIDI.cpp Export.h MidiDocument.h MidiDocument.cpp)
add_library(iiCSMIDI::iiCSMIDI ALIAS iiCSMIDI)
target_compile_features(iiCSMIDI PUBLIC cxx_std_20)
target_compile_definitions(iiCSMIDI PRIVATE IICSMIDI_BUILDING_LIBRARY)
target_link_libraries(iiCSMIDI PUBLIC Qt6::Core iiFileProvider::iiFileProvider)
target_include_directories(iiCSMIDI PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(iiCSMIDI PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

if(BUILD_TESTING)
    add_executable(iiCSMIDI_hello_world tests/hello_world.cpp)
    target_link_libraries(iiCSMIDI_hello_world PRIVATE iiCSMIDI::iiCSMIDI)
    set_target_properties(iiCSMIDI_hello_world PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME iiCSMIDI.hello_world COMMAND iiCSMIDI_hello_world)
endif()

set(_package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/iiCSMIDI")
configure_package_config_file(
    cmake/iiCSMIDIConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/iiCSMIDIConfig.cmake"
    INSTALL_DESTINATION "${_package_dir}"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/iiCSMIDIConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(TARGETS iiCSMIDI EXPORT iiCSMIDITargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES iiCSMIDI.h Export.h MidiDocument.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT iiCSMIDITargets NAMESPACE iiCSMIDI:: DESTINATION "${_package_dir}")
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/iiCSMIDIConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/iiCSMIDIConfigVersion.cmake"
    DESTINATION "${_package_dir}"
)
install(FILES README.md DESTINATION "${CMAKE_INSTALL_DATADIR}/iiCSMIDI")

if(BUILD_TESTING)
    add_executable(iiCSMIDI_authorship tests/authorship.cpp)
    target_link_libraries(iiCSMIDI_authorship PRIVATE iiCSMIDI::iiCSMIDI)
    add_test(NAME iiCSMIDI.authorship COMMAND iiCSMIDI_authorship)
endif()
