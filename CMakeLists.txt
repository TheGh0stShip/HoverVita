cmake_minimum_required(VERSION 3.16)

# Vita builds are selected by the VitaSDK toolchain file, e.g.
#   cmake -B build-vita -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake
if(DEFINED ENV{VITASDK} AND CMAKE_TOOLCHAIN_FILE MATCHES "vita")
  include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
endif()

project(HoverVita C)

set(VITA_APP_NAME "Hover!")
set(VITA_TITLEID  "HOVR00001")
set(VITA_VERSION  "00.02")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_compile_options(-Wall -Wextra -Wno-unused-parameter)

# Portable game code: no platform headers allowed in here.
set(ENGINE_SOURCES
  src/engine/archive.c
  src/engine/maze.c
  src/engine/texture.c
)

add_executable(${PROJECT_NAME}
  src/main.c
  src/render/renderer.c
  src/platform/platform_files.c
  ${ENGINE_SOURCES}
)
target_include_directories(${PROJECT_NAME} PRIVATE src)

if(VITA)
  # Native Vita backend: vitaGL for rendering, sce* for input/power.
  target_sources(${PROJECT_NAME} PRIVATE src/platform/platform_vita.c)
  target_compile_options(${PROJECT_NAME} PRIVATE -O2 -mfpu=neon -mcpu=cortex-a9)
  target_link_libraries(${PROJECT_NAME}
    vitaGL vitashark SceShaccCgExt SceShaccCg_stub mathneon taihen_stub
    SceKernelDmacMgr_stub SceGxm_stub SceDisplay_stub SceCtrl_stub SceAppMgr_stub
    SceCommonDialog_stub SceIme_stub ScePower_stub SceSysmodule_stub SceLibKernel_stub
    m
  )
  # vitaGL's shader compiler is C++; link with the C++ driver for libstdc++.
  enable_language(CXX)
  set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

  vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)
  vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} eboot.bin
    VERSION ${VITA_VERSION}
    NAME ${VITA_APP_NAME}
    FILE sce_sys/icon0.png sce_sys/icon0.png
    FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
    FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
    FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
  )
else()
  find_package(SDL2 REQUIRED)
  find_package(OpenGL REQUIRED)
  target_sources(${PROJECT_NAME} PRIVATE src/platform/platform_pc.c)
  target_link_libraries(${PROJECT_NAME} SDL2::SDL2 OpenGL::GL m)
  if(TARGET SDL2::SDL2main)
    target_link_libraries(${PROJECT_NAME} SDL2::SDL2main)
  endif()

  include(CTest)
  if(BUILD_TESTING)
    # Tests need no window: they only use the engine and file loading, plus
    # a stdio logger.
    add_library(test_support STATIC tests/test_log.c src/platform/platform_files.c ${ENGINE_SOURCES})
    target_include_directories(test_support PUBLIC src)
    foreach(t textures mazes)
      add_executable(test_${t} tests/test_${t}.c)
      target_link_libraries(test_${t} test_support m)
      add_test(NAME ${t} COMMAND test_${t})
      set_tests_properties(${t} PROPERTIES
        SKIP_RETURN_CODE 77
        ENVIRONMENT "HOVER_DATA=${CMAKE_SOURCE_DIR}/hover")
    endforeach()
  endif()
endif()
