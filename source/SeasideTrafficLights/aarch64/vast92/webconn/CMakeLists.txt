#------------------------------------------------------------------
# VA Smalltalk's Webconn module build instructions
#
# The following can be used to build the Webconn module using
# CMake (a cross-platform build generator).
# Each platform subdirectory contains the legacy makefiles and you
# can still build Webconn using those.  However, these instructions
# are, to a large degree, compiler/system independent.
#
# How To Build: 
# 1. In the directory this script is in, create a new directory called 'build'
# 2. cd build
# 3. cmake .. -G <Project Generator>
#
# For example, to make an NMake project on Windows, you would specify
# cmake .. -G "NMake Makefiles"  (this will build an nmake project)
# On Linux, you can do
# cmake .. -G "Unix Makefiles"   (this will build a make project) 
#
# There are numerous projects, please refer to cmake reference material
# to learn more about the options 
#------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8)
project(webconn)

#-- RelWithDebInfo (None Debug Release RelWithDebInfo MinSizeRel)
if(CMAKE_BUILD_TYPE STREQUAL "")
  set(DEFAULT_BUILD_TYPE "RelWithDebInfo")
  message(STATUS "Build type not specified. DEFAULT: ${DEFAULT_BUILD_TYPE}")
  set(CMAKE_BUILD_TYPE ${DEFAULT_BUILD_TYPE}
      CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif()

#-- Do not allow in-source builds.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
	message( FATAL_ERROR
		"Please create a separate folder called 'build' and do the builds in there.\n"
		"Mercurial will ignore $ROOT/build and we can keep things clean\n"
		"Example:\n"
		"  rm -rf CMakeFiles CMakeCache.txt\n"
		"  mkdir build\n"
		"  cd build\n"
		"  cmake ..")
endif()

#-- GCC specific settings
if(CMAKE_COMPILER_IS_GNUCC)
  # Statically link libgcc so it's not expected that
  # the user has gcc libraries installed on their machine
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")
endif()

#-- Setup where we want binaries and libraries to end up
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

#------------------------------------------------------------------
# Common Build Instructions
#------------------------------------------------------------------
set(webconn_INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}/source
  CACHE PATH "Webconn: Headers")
include_directories(${webconn_INCLUDES})

# Core Functions Library
set(abtwsc_LIB_NAME abtwsc)
set(abtwsc_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/source/abtwsc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/source/abtwscos.c)

# Base TCP/IP Transport Functions DLL
set(abtwstt_LIB_NAME abtwstt)
set(abtwstt_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/source/abtwstt.c
  ${CMAKE_CURRENT_SOURCE_DIR}/source/abtwstbs.c)

# CGI Executable
set(abtwsac_EXE_NAME abtwsac)
set(abtwsac_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/source/abtwsac.c)

# IBM ICS Adapter DLL
#set(abtwsai_LIB_NAME abtwsai)
#set(abtwsai_SOURCES
#  ${CMAKE_CURRENT_SOURCE_DIR}/source/abtwsai.c)

# Netscape NSAPI Adapter DLL
#set(abtwsan_LIB_NAME abtwsan)
#set(abtwsan_SOURCES
#  ${CMAKE_CURRENT_SOURCE_DIR}/source/abtwsan.c)

# Add thread-safe flags
find_package(Threads)
set(abtws_EXTERNAL_LIBS ${CMAKE_THREAD_LIBS_INIT})

#------------------------------------------------------------------
# OS-Dependent Build Instructions
#------------------------------------------------------------------
if(WIN32)

  add_definitions(-DOPSYS_WIN32 -D_CRT_SECURE_NO_DEPRECATE)

  set(abtws_EXTERNAL_LIBS ${abtws_EXTERNAL_LIBS} wsock32.lib)

  # Microsoft ISAPI Adapter DLL
  set(abtwsam_LIB_NAME abtwsam)
  set(abtwsam_SOURCES
      ${CMAKE_CURRENT_SOURCE_DIR}/source/abtwsam.c)

  #-- Configure defs
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/win/abtwsam.def
    ${CMAKE_CURRENT_BINARY_DIR}/win/abtwsam.def)
  set(abtwsam_SOURCES ${abtwsam_SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/win/abtwsam.def)

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/win/abtwsc.def
    ${CMAKE_CURRENT_BINARY_DIR}/win/abtwsc.def)
  set(abtwsc_SOURCES ${abtwsc_SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/win/abtwsc.def)

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/win/abtwstt.def
    ${CMAKE_CURRENT_BINARY_DIR}/win/abtwstt.def)
  set(abtwstt_SOURCES ${abtwstt_SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/win/abtwstt.def)

#  configure_file(
#    ${CMAKE_CURRENT_SOURCE_DIR}/win/abtwsai.def
#    ${CMAKE_CURRENT_BINARY_DIR}/win/abtwsai.def)
#  set(abtwsai_SOURCES ${abtwsai_SOURCES}
#    ${CMAKE_CURRENT_BINARY_DIR}/win/abtwsai.def)

#  configure_file(
#    ${CMAKE_CURRENT_SOURCE_DIR}/win/abtwsan.def
#    ${CMAKE_CURRENT_BINARY_DIR}/win/abtwsan.def)
#	set(abtwsan_SOURCES ${abtwsan_SOURCES}
#    ${CMAKE_CURRENT_BINARY_DIR}/win/abtwsan.def)

elseif(UNIX AND NOT APPLE)

  #-- Add unix flavor build defs
  if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

    add_definitions(-DOPSYS_LINUX)

  elseif(${CMAKE_SYSTEM_NAME} STREQUAL "AIX")

	  add_definitions(-DOPSYS_AIX)

  elseif(${CMAKE_SYSTEM_NAME} MATCHES "(Solaris|SunOS)")

	  add_definitions(-DOPSYS_SOLARIS)

  else()
    message(FATAL_ERROR "Unsupported Unix Platform")
  endif()

else()
  message(FATAL_ERROR "Unsupported Platform")
endif()

#------------------------------------------------------------------
# Core Functions DLL
#------------------------------------------------------------------
add_library(${abtwsc_LIB_NAME} SHARED ${abtwsc_SOURCES})
target_link_libraries(${abtwsc_LIB_NAME} ${CMAKE_DL_LIBS})

#------------------------------------------------------------------
# Base TCP/IP Transport Functions DLL
#------------------------------------------------------------------
add_library(${abtwstt_LIB_NAME} SHARED ${abtwstt_SOURCES})
add_dependencies(${abtwstt_LIB_NAME} ${abtwsc_LIB_NAME})
target_link_libraries(${abtwstt_LIB_NAME} ${abtwsc_LIB_NAME} ${abtws_EXTERNAL_LIBS})

#------------------------------------------------------------------
# Microsoft ISAPI Adapter DLL
#------------------------------------------------------------------
if(WIN32)
   add_library(${abtwsam_LIB_NAME} SHARED ${abtwsam_SOURCES})
   add_dependencies(${abtwstt_LIB_NAME} ${abtwsc_LIB_NAME})
   target_link_libraries(${abtwsam_LIB_NAME} ${abtwsc_LIB_NAME})
endif()

#------------------------------------------------------------------
# CGI Executable
#------------------------------------------------------------------
add_executable(${abtwsac_EXE_NAME} ${abtwsac_SOURCES})
add_dependencies(${abtwsac_EXE_NAME} ${abtwsc_LIB_NAME})
target_link_libraries(${abtwsac_EXE_NAME} ${abtwsam_LIB_NAME} ${abtwsc_LIB_NAME} ${abtws_EXTERNAL_LIBS})

#------------------------------------------------------------------
# IBM ICS Adapter DLL
#------------------------------------------------------------------
message(STATUS "Skipping deprecated module: IBM ICS Adapater")
message(STATUS "Instructions to build are commented out in the CMakeLists.txt buildfile")
#add_library(${abtwsai_LIB_NAME} SHARED ${abtwsai_SOURCES})
#add_dependencies(${abtwsai_LIB_NAME} ${abtwsc_LIB_NAME})
#target_link_libraries(${abtwsai_LIB_NAME} ${abtwsc_LIB_NAME} httpdapi.lib)

#------------------------------------------------------------------
# Netscape NSAPI Adapter DLL
#------------------------------------------------------------------
message(STATUS "Skipping deprecated module: Netscape NSAPI Adapter")
message(STATUS "Instructions to build are commented out in the CMakeLists.txt buildfile")
#add_library(${abtwsan_LIB_NAME} SHARED ${abtwsan_SOURCES})
#add_dependencies(${abtwsan_LIB_NAME} ${abtwsc_LIB_NAME})
#target_link_libraries(${abtwsan_LIB_NAME} ${abtwsc_LIB_NAME} libhttpd.lib)


#-- Don't prefix with 'lib' on gcc
#-- This is for backwards compatibility
if(UNIX OR CMAKE_COMPILER_IS_GNUCC)
  set_target_properties(${abtwsc_LIB_NAME} PROPERTIES PREFIX "")
  set_target_properties(${abtwstt_LIB_NAME} PROPERTIES PREFIX "")
  if(WIN32)
    set_target_properties(${abtwsam_LIB_NAME} PROPERTIES PREFIX "")
  endif()
  #set_target_properties(${abtwsai_LIB_NAME} PROPERTIES PREFIX "")
  #set_target_properties(${abtwsan_LIB_NAME} PROPERTIES PREFIX "")
endif()
