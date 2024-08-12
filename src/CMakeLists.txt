string(TIMESTAMP FMUTIMESTAMP UTC)
string(MD5 FMUGUID modelDescription.in.xml)
configure_file(modelDescription.in.xml modelDescription.xml @ONLY)
configure_file(OSMPTraceFilePlayerConfig.in.h OSMPTraceFilePlayerConfig.h)

find_package(Protobuf 2.6.1 REQUIRED)
add_library(sl-5-5-osi-trace-file-player SHARED OSMPTraceFilePlayer.cpp)
set_target_properties(sl-5-5-osi-trace-file-player PROPERTIES PREFIX "")
target_compile_definitions(sl-5-5-osi-trace-file-player PRIVATE "FMU_SHARED_OBJECT")
if(LINK_WITH_SHARED_OSI)
	target_link_libraries(sl-5-5-osi-trace-file-player open_simulation_interface stdc++fs)
else()
	target_link_libraries(sl-5-5-osi-trace-file-player open_simulation_interface_pic stdc++fs)
endif()
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(WIN32)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(FMI_BINARIES_PLATFORM "win64")
	else()
		set(FMI_BINARIES_PLATFORM "win32")
	endif()
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(FMI_BINARIES_PLATFORM "linux64")
	else()
		set(FMI_BINARIES_PLATFORM "linux32")
	endif()
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(FMI_BINARIES_PLATFORM "darwin64")
	else()
		set(FMI_BINARIES_PLATFORM "darwin32")
	endif()
endif()

add_custom_command(TARGET sl-5-5-osi-trace-file-player
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/buildfmu"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/buildfmu/sources"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/buildfmu/binaries/${FMI_BINARIES_PLATFORM}"
		COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/modelDescription.xml" "${CMAKE_CURRENT_BINARY_DIR}/buildfmu"
		COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/OSMPTraceFilePlayer.cpp" "${CMAKE_CURRENT_BINARY_DIR}/buildfmu/sources/"
		COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/OSMPTraceFilePlayer.h" "${CMAKE_CURRENT_BINARY_DIR}/buildfmu/sources/"
		COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/OSMPTraceFilePlayerConfig.h" "${CMAKE_CURRENT_BINARY_DIR}/buildfmu/sources/OSMPTraceFilePlayerConfig.h"
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:sl-5-5-osi-trace-file-player> $<$<PLATFORM_ID:Windows>:$<$<CONFIG:Debug>:$<TARGET_PDB_FILE:sl-5-5-osi-trace-file-player>>> "${CMAKE_CURRENT_BINARY_DIR}/buildfmu/binaries/${FMI_BINARIES_PLATFORM}"
		COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_CURRENT_BINARY_DIR}/buildfmu" ${CMAKE_COMMAND} -E tar "cfv" "${FMU_INSTALL_DIR}/sl-5-5-osi-trace-file-player.fmu" --format=zip "modelDescription.xml" "${CMAKE_CURRENT_BINARY_DIR}/buildfmu/sources" "${CMAKE_CURRENT_BINARY_DIR}/buildfmu/binaries/${FMI_BINARIES_PLATFORM}")
