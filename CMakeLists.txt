cmake_minimum_required(VERSION 3.16)

# Vita builds are selected by the VitaSDK toolchain file, e.g.
#   cmake -B build-vita -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake
if(DEFINED ENV{VITASDK} AND CMAKE_TOOLCHAIN_FILE MATCHES "vita")
  include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
endif()

project(HoverVita C)

set(VITA_APP_NAME "Hover!")
set(VITA_TITLEID  "HOVR00001")
set(VITA_VERSION  "00.01")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_compile_options(-Wall -Wextra -Wno-unused-parameter)

set(ENGINE_SOURCES
  src/engine/archive.c
  src/engine/texture.c
)

set(PLATFORM_SOURCES
  src/platform/platform_sdl.c
)

add_executable(${PROJECT_NAME} src/main.c ${ENGINE_SOURCES} ${PLATFORM_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src)

if(VITA)
  # Use SDL2's own static dependency list: it differs between VitaSDK SDL2
  # builds (newer ones render through vitaGL and need its shader libraries).
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
  target_link_libraries(${PROJECT_NAME} ${SDL2_STATIC_LIBRARIES})
  if(SDL2_STATIC_LIBRARIES MATCHES "vitaGL")
    # sdl2.pc omits these vitaGL dependencies
    target_link_libraries(${PROJECT_NAME} SceShaccCgExt taihen_stub)
  endif()
  target_link_directories(${PROJECT_NAME} PRIVATE ${SDL2_STATIC_LIBRARY_DIRS})
  target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_STATIC_INCLUDE_DIRS})
  # vitaGL is C++; link with the C++ driver so libstdc++ comes last.
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
  target_link_libraries(${PROJECT_NAME} SDL2::SDL2 m)
  if(TARGET SDL2::SDL2main)
    target_link_libraries(${PROJECT_NAME} SDL2::SDL2main)
  endif()
endif()

if(NOT VITA)
  include(CTest)
  if(BUILD_TESTING)
    add_executable(test_textures tests/test_textures.c ${ENGINE_SOURCES} ${PLATFORM_SOURCES})
    target_include_directories(test_textures PRIVATE src)
    target_link_libraries(test_textures SDL2::SDL2 m)
    add_test(NAME textures COMMAND test_textures)
    set_tests_properties(textures PROPERTIES
      SKIP_RETURN_CODE 77
      ENVIRONMENT "HOVER_DATA=${CMAKE_SOURCE_DIR}/hover")
  endif()
endif()
