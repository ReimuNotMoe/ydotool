cmake_minimum_required(VERSION 3.4)
project(ydotool C)

set(CMAKE_C_STANDARD 99)

# Include GNU install directory module to detect where to install
# files on Linux/Unix systems (e.g., lib vs lib64)
include(GNUInstallDirs)
find_package(PkgConfig)

# Try to get the systemd directory for user units from the package
# and fall back on upstream default without the '/usr' prefix
pkg_check_modules(SYSTEMD systemd)
if (SYSTEMD_FOUND)
    pkg_get_variable(SD_UNITDIR systemd systemduserunitdir)
else()
    set(SD_UNITDIR "lib/systemd/user")
endif()

execute_process(COMMAND git describe --tags --long --always RESULT_VARIABLE RC_GIT_VER OUTPUT_VARIABLE GIT_VERSION)

if (${RC_GIT_VER} EQUAL 0)
    string(STRIP ${GIT_VERSION} GIT_VERSION)
    message("-- Version: " ${GIT_VERSION})
    add_definitions(-DVERSION=\"${GIT_VERSION}\")
endif()



set(SOURCE_FILES_DAEMON Daemon/ydotoold.c)
set(SOURCE_FILES_CLIENT Client/ydotool.c Client/tool_click.c Client/tool_mousemove.c Client/tool_type.c Client/tool_key.c)

add_executable(ydotoold ${SOURCE_FILES_DAEMON})
install(TARGETS ydotoold DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(ydotool ${SOURCE_FILES_CLIENT})
install(TARGETS ydotool DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(Daemon)
add_subdirectory(manpage)
