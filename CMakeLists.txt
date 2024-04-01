# @TESTALL:ANY@

cmake_minimum_required(VERSION 3.22)

if(UNIX)
    cmake_path(SET USERHOME NORMALIZE $ENV{HOME})
else()
    cmake_path(SET USERHOME NORMALIZE $ENV{USERPROFILE})
endif()

#
# USER CONFIG
#

# Use preloading via withdll.exe from Microsoft Detours project?
# (ie 'withdll.exe /d:ReaperThemeHackDll.dll reaper.exe')
# (recommended for best results)
# (also debugging needs a manual connect to the reaper process - CTRL-ALT-P)
set(WITH_DETOURS_WITHDLL TRUE CACHE BOOL TRUE)

# your reaper install directory for debugging
# (best to use a separate and clean portable install that's user-writable)
cmake_path(SET REAPER_HOME NORMALIZE "${USERHOME}/source/reaper" )

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
cmake_path(SET REAPERSDK_INCLUDE_DIRS NORMALIZE "${reapersdk_SOURCE_DIR}/sdk/")

FetchContent_Populate(
  inipp
  URL https://github.com/mcmtroffaes/inipp/archive/c61e699682d3f1091209c2179f1d03f5fc593327.zip
  URL_HASH SHA256=1727e025c03e80db4f66925fa5e20e36943c2a2f94c6b141cb1f6f792a6cdca8
  EXCLUDE_FROM_ALL
  )
cmake_path(SET INIPP_INCLUDE_DIRS NORMALIZE "${inipp_SOURCE_DIR}/inipp/")

if(WITH_DETOURS_WITHDLL)
    include(ExternalProject)
    ExternalProject_Add(
      detours
      URL https://github.com/microsoft/Detours/archive/4b8c659f549b0ab21cf649377c7a84eb708f5e68.zip
      URL_HASH SHA256=08d965a8374ca1e56bcf05380d2ee7daa846080b85d568223a41c20b5913bbf8
      CONFIGURE_COMMAND ""
      BUILD_COMMAND nmake
      INSTALL_COMMAND ""
      BUILD_IN_SOURCE TRUE
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    ExternalProject_Get_property(detours SOURCE_DIR)
    cmake_path(SET WITHDLLEXE_PATH NORMALIZE "${SOURCE_DIR}/bin.X64/withdll.exe")
endif()

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

add_library(${PROJECT_NAME} SHARED
    framework.h

    reaper_impl.cpp

    utils.cpp
    utils.h

    UAHMenuBar.h

    ReaperMenuBarThemeDll.cpp
    ReaperMenuBarThemeDll.def

    README.md
)
add_dependencies(${PROJECT_NAME} detours)
target_include_directories(${PROJECT_NAME} PRIVATE ${REAPERSDK_INCLUDE_DIRS} ${INIPP_INCLUDE_DIRS})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
target_precompile_headers(${PROJECT_NAME} PRIVATE framework.h)

# withdll.exe wants ordinal 1
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.def\"")

#
# set up debugging under reaper for convenience
#
cmake_path(SET REAPER_EXE_PATH NORMALIZE "${REAPER_HOME}/reaper.exe")
cmake_path(SET REAPER_USERDLL_PATH NORMALIZE "${REAPER_HOME}/UserPlugins/reaper_themehack-x64.dll")

if (EXISTS "${REAPER_EXE_PATH}")
    if(WITH_DETOURS_WITHDLL) # dll 'preload' mode
        cmake_path(GET REAPER_EXE_PATH PARENT_PATH REAPER_INSTALL_PATH)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD 
            COMMAND "${CMAKE_COMMAND}" -E copy 
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${REAPER_INSTALL_PATH}" 
            COMMENT "Copying to reaper install directory ${REAPER_INSTALL_PATH}")
        # remove UserPlugin variant so we don't end up running two instances
        # within the same Reaper process
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD 
            COMMAND "${CMAKE_COMMAND}" -E rm 
            "${REAPER_USERDLL_PATH}"
            COMMENT "Removing ${REAPER_INSTALL_PATH}")
        set_target_properties(${PROJECT_NAME} PROPERTIES
            VS_DEBUGGER_COMMAND "${WITHDLLEXE_PATH}"
            VS_DEBUGGER_COMMAND_ARGUMENTS "/d:${REAPER_INSTALL_PATH}/${PROJECT_NAME}.dll ${REAPER_EXE_PATH}")
    else() # Reaper UserPlugin mode
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD 
            COMMAND "${CMAKE_COMMAND}" -E copy 
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${REAPER_USERDLL_PATH}" 
            COMMENT "Copying to reaper userplugins directory ${REAPER_USERDLL_PATH}")
        set_target_properties(${PROJECT_NAME} PROPERTIES
            VS_DEBUGGER_COMMAND "${REAPER_EXE_PATH}"
            VS_DEBUGGER_COMMAND_ARGUMENTS "")
    endif()
endif()

