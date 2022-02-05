cmake_minimum_required(VERSION 3.4)
project(ydotool)

set(CMAKE_CXX_STANDARD 17)

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


set(SOURCE_FILES_DAEMON Daemon/ydotoold.c)
set(SOURCE_FILES_CLIENT Client/ydotool.c Client/tool_click.c Client/tool_mousemove.c Client/tool_type.c Client/tool_key.c)

add_executable(ydotoold ${SOURCE_FILES_DAEMON})
install(TARGETS ydotoold DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(ydotool ${SOURCE_FILES_CLIENT})
install(TARGETS ydotool DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(Daemon)
add_subdirectory(manpage)
