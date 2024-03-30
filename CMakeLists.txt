# @TESTALL:ANY@

cmake_minimum_required(VERSION 3.22)

#
# DEPENDENCIES
#
include(FetchContent)
FetchContent_Populate(
  reapersdk
  URL https://github.com/justinfrankel/reaper-sdk/archive/02deb819c4ba0fc1bac001c8dfff7b9dbaf408ad.zip
  URL_HASH SHA256=da690f80c498005046a7301b10923f7345a49a0e5691f621bf289099ce20230f
  EXCLUDE_FROM_ALL
  )
FetchContent_Populate(
  inipp
  URL https://github.com/mcmtroffaes/inipp/archive/c61e699682d3f1091209c2179f1d03f5fc593327.zip
  URL_HASH SHA256=1727e025c03e80db4f66925fa5e20e36943c2a2f94c6b141cb1f6f792a6cdca8
  EXCLUDE_FROM_ALL
  )

#
# THE PROJECT
#
project(ReaperThemeHackDll)
set(CMAKE_CXX_STANDARD 20)

if (MSVC)
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
  add_compile_options(/MP)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  add_compile_definitions(NOMINMAX UNICODE)
endif()

cmake_path(SET INIPP_INCLUDE_DIRS NORMALIZE "${inipp_SOURCE_DIR}/inipp/")
cmake_path(SET REAPERSDK_INCLUDE_DIRS NORMALIZE "${reapersdk_SOURCE_DIR}/sdk/")

add_library(${PROJECT_NAME} SHARED
    framework.h

    utils.cpp
    utils.h

    UAHMenuBar.cpp
    UAHMenuBar.h

    reaper_api_impl.cpp
    
    ReaperMenuBarThemeDll.cpp

    README.md
)
target_include_directories(${PROJECT_NAME} PRIVATE ${REAPERSDK_INCLUDE_DIRS} ${INIPP_INCLUDE_DIRS})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
target_precompile_headers(${PROJECT_NAME} PRIVATE framework.h)

#
# set up debugging under reaper for convenience
#
if(UNIX)
    cmake_path(SET USERHOME NORMALIZE $ENV{HOME})
else()
    cmake_path(SET USERHOME NORMALIZE $ENV{USERPROFILE})
endif()
cmake_path(SET REAPER_EXE_PATH NORMALIZE "${USERHOME}")
cmake_path(APPEND REAPER_EXE_PATH "source/reaper/reaper.exe" )
cmake_path(SET REAPER_USERDLL_PATH NORMALIZE "${USERHOME}")
cmake_path(APPEND REAPER_USERDLL_PATH "source/reaper/UserPlugins/reaper_example-x64.dll" )

get_filename_component(_fullpath "${_dir}" REALPATH)
if (EXISTS "${REAPER_USERDLL_PATH}")
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD 
    COMMAND "${CMAKE_COMMAND}" -E copy 
      "$<TARGET_FILE:${PROJECT_NAME}>"
      "${REAPER_USERDLL_PATH}" 
    COMMENT "Copying to reaper userplugins directory ${REAPER_USERDLL_PATH}")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_COMMAND "${REAPER_EXE_PATH}"
    VS_DEBUGGER_COMMAND_ARGUMENTS "")

