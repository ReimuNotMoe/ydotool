cmake_minimum_required(VERSION 3.22)
project(ydotool C)

set(CMAKE_C_STANDARD 11)

# Check for libxkbcommon availability
execute_process(COMMAND pkg-config --exists xkbcommon
    RESULT_VARIABLE PKG_XKB_EXISTS
    OUTPUT_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(${PKG_XKB_EXISTS} EQUAL 0)
    execute_process(COMMAND pkg-config --cflags xkbcommon
        OUTPUT_VARIABLE XKB_CFLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(COMMAND pkg-config --libs xkbcommon
        OUTPUT_VARIABLE XKB_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "Found libxkbcommon: ${XKB_LIBS}")
    add_definitions(-DHAVE_LIBXKBCOMMON=1)
else()
    message(STATUS "libxkbcommon not found, using hardcoded layout mappings only")
    add_definitions(-DHAVE_LIBXKBCOMMON=0)
    set(XKB_CFLAGS "")
    set(XKB_LIBS "")
endif()

# Include GNU install directory module to detect where to install
# files on Linux/Unix systems (e.g., lib vs lib64)
include(GNUInstallDirs)

set(SOURCE_FILES_DAEMON Daemon/ydotoold.c)
set(SOURCE_FILES_CLIENT Client/ydotool.c Client/tool_click.c Client/tool_mousemove.c Client/tool_key.c Client/tool_stdin.c Client/tool_type.c)

add_executable(ydotoold ${SOURCE_FILES_DAEMON})
install(TARGETS ydotoold DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(ydotool ${SOURCE_FILES_CLIENT})
if(NOT ${XKB_LIBS} STREQUAL "")
    target_link_libraries(ydotool ${XKB_LIBS})
    target_compile_options(ydotool PRIVATE ${XKB_CFLAGS})
endif()
install(TARGETS ydotool DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(Daemon)
