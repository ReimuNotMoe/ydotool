option(SYSTEMD_USER_SERVICE "Install systemd user service file" ON)
option(SYSTEMD_SYSTEM_SERVICE "Install systemd system service file" OFF)
option(OPENRC "Install OpenRC service file" OFF) # To be implemented

if(SYSTEMD_USER_SERVICE AND SYSTEMD_SYSTEM_SERVICE)
    message(FATAL_ERROR "Both system and user services are enabled. Choose one.")
endif()

if(SYSTEMD_USER_SERVICE OR SYSTEMD_SYSTEM_SERVICE) 
    add_subdirectory(systemd)
endif()

if(OPENRC)
    add_subdirectory(openrc)
endif()
