option(SYSTEMD_USER_SERVICE "Install systemd user service file" ON)
option(SYSTEMD_SYSTEM_SERVICE "Install systemd system service file" OFF)
option(OPENRC "Install OpenRC service file" OFF) # To be implemented

if(SYSTEMD_USER_SERVICE AND SYSTEMD_SYSTEM_SERVICE)
    message(FATAL_ERROR "Both system and user services are enabled. Choose one.")
endif()

if(SYSTEMD_USER_SERVICE OR SYSTEMD_SYSTEM_SERVICE) 
    find_package(PkgConfig)
    # Try to get the systemd directory for system/user units from the package
    # and fall back on upstream default without the '/usr' prefix
    pkg_check_modules(SYSTEMD systemd)
    if (SYSTEMD_FOUND)
        if(SYSTEMD_USER_SERVICE)
            pkg_get_variable(SD_UNITDIR systemd systemduserunitdir)
        elseif(SYSTEMD_SYSTEM_SERVICE)
            pkg_get_variable(SD_UNITDIR systemd systemdsystemunitdir)
        endif()
    # else()
    #     set(SD_UNITDIR "lib/systemd/user")
    endif()

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/ydotoold.service.in"
        "${PROJECT_BINARY_DIR}/ydotoold.service"
        @ONLY)
    install(FILES "${PROJECT_BINARY_DIR}/ydotoold.service" DESTINATION ${SD_UNITDIR})
endif()
