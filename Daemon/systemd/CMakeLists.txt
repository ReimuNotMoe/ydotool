find_package(PkgConfig)

# Try to get the systemd directory for system/user units from the package
# and fall back on upstream default without the '/usr' prefix if not forced by
# command line.
pkg_check_modules(SYSTEMD systemd)
if (SYSTEMD_FOUND)
    if(SYSTEMD_USER_SERVICE)
        pkg_get_variable(SD_UNITDIR systemd systemduserunitdir)
    elseif(SYSTEMD_SYSTEM_SERVICE)
        pkg_get_variable(SD_UNITDIR systemd systemdsystemunitdir)
    endif()
endif()

if(NOT DEFINED SD_UNITDIR)
    set(SD_UNITDIR "lib/systemd/user")
endif()

message("Systemd service file is going to be installed at ${SD_UNITDIR}")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ydotoold.service.in"
    "${PROJECT_BINARY_DIR}/ydotoold.service"
    @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/ydotoold.service" DESTINATION ${SD_UNITDIR})
