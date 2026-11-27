cmake_minimum_required( VERSION 3.16 )
project( propan2ol VERSION 0.1.0 LANGUAGES C CXX )

find_package( Threads REQUIRED )

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

# Built on its own, or inside a parent project (add_subdirectory, e.g. a git submodule): then the tests and the install rules are off
# unless asked for, and the paths below are this project's own
if ( CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR )
	set( PROPAN2OL_TOP_LEVEL ON )
else()
	set( PROPAN2OL_TOP_LEVEL OFF )
endif()
option( PROPAN2OL_TESTS "the parity harness and the tests" ${PROPAN2OL_TOP_LEVEL} )
option( PROPAN2OL_INSTALL "the install rules and package config" ${PROPAN2OL_TOP_LEVEL} )

# One set of objects, in a static and a shared library. Only the C ABI is exported from the shared one.
add_library( propan2ol_objects OBJECT
	src/engine/engine.cpp src/engine/dictionary.cpp src/engine/numbers.cpp src/engine/translate.cpp src/engine/translateword.cpp
	src/engine/phoneme_data.cpp src/engine/phoneme_list.cpp src/engine/clause_reader.cpp src/engine/languages.cpp
	src/chars.cpp src/ctype_tables.cpp src/ucd_tables.cpp src/text_decoder.cpp src/text.cpp src/phonemizer.cpp src/c_api.cpp )
include( ${PROJECT_SOURCE_DIR}/cmake/embed_data.cmake )
embed_data( propan2ol_objects ${PROJECT_SOURCE_DIR}/data )
set_target_properties( propan2ol_objects PROPERTIES POSITION_INDEPENDENT_CODE ON CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON )
target_include_directories( propan2ol_objects
	PUBLIC include
	PRIVATE src src/engine )
# -fexceptions: the C ABI turns exceptions into errors, whatever a parent project's flags say
target_compile_options( propan2ol_objects PRIVATE -fexceptions -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-implicit-fallthrough )

add_library( propan2ol STATIC $<TARGET_OBJECTS:propan2ol_objects> )
target_include_directories( propan2ol PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include> )
add_library( propan2ol::propan2ol ALIAS propan2ol )

add_library( propan2ol_shared SHARED $<TARGET_OBJECTS:propan2ol_objects> )
set_target_properties( propan2ol_shared PROPERTIES OUTPUT_NAME propan2ol )
target_include_directories( propan2ol_shared PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include> )
add_library( propan2ol::propan2ol_shared ALIAS propan2ol_shared )
target_compile_definitions( propan2ol_objects PRIVATE PROPAN2OL_BUILD_SHARED )

# ---- dev-only: the parity harness against the system libespeak-ng (tests/parity) ----
if ( PROPAN2OL_TESTS )
add_executable( p2o_probe tests/parity/probe.cpp )
target_include_directories( p2o_probe PRIVATE src src/engine )
target_link_libraries( p2o_probe PRIVATE propan2ol )

find_library( ESPEAK_NG_LIBRARY espeak-ng )
if ( ESPEAK_NG_LIBRARY )
	add_executable( ref_probe tests/parity/probe.cpp )
	target_compile_definitions( ref_probe PRIVATE REFERENCE )
	target_link_libraries( ref_probe PRIVATE ${ESPEAK_NG_LIBRARY} )
endif()

add_executable( test_instances tests/parity/test_instances.cpp )
target_include_directories( test_instances PRIVATE src )
target_link_libraries( test_instances PRIVATE propan2ol Threads::Threads )

add_executable( test_api tests/test_api.cpp )
target_link_libraries( test_api PRIVATE propan2ol )
endif()

# ---- install: the libraries, the headers, and a package config (find_package( propan2ol ) -> propan2ol::propan2ol) ----
if ( PROPAN2OL_INSTALL )
include( GNUInstallDirs )
include( CMakePackageConfigHelpers )
install( TARGETS propan2ol propan2ol_shared EXPORT propan2ol-targets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
install( DIRECTORY include/propan2ol DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} )
install( EXPORT propan2ol-targets NAMESPACE propan2ol:: FILE propan2ol-targets.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/propan2ol )
file( WRITE ${CMAKE_CURRENT_BINARY_DIR}/propan2ol-config.cmake "include( \"\${CMAKE_CURRENT_LIST_DIR}/propan2ol-targets.cmake\" )\n" )
write_basic_package_version_file( ${CMAKE_CURRENT_BINARY_DIR}/propan2ol-config-version.cmake VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/propan2ol-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/propan2ol-config-version.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/propan2ol )
endif()
