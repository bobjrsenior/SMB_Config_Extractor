cmake_minimum_required(VERSION 3.9.0)
project(SMB_Config_Extractor)

#Export compile commands for editor autocomplete
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#Be really pedantic!
add_definitions(-Wall -Wextra -pedantic)

#Show as an executable, not a shared library in file managers
if(UNIX)
	#-nopie is unused with AppleClang
	if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
		set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -no-pie")
	endif(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
endif(UNIX)

set(SOURCE_FILES
	SMB_Config_Extractor/configExtractor.c
	SMB_Config_Extractor/main.c
	SMB_Config_Extractor/xmlbuddy.c
	)

set(HEADER_FILES
	SMB_Config_Extractor/configExtractor.h
	SMB_Config_Extractor/xmlbuddy.h
	SMB_Config_Extractor/FunctionsAndDefines.h
	)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

