cmake_minimum_required(VERSION 3.0)
project(ydotool)

set(CMAKE_CXX_STANDARD 14)
set(PROJECT_VERSION "0.1.5")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "5")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Generic Linux command-line automation tool (no X!)")
set(CPACK_PACKAGE_CONTACT "Reimu NotMoe <https://github.com/ReimuNotMoe>")
set(CMAKE_PROJECT_HOMEPAGE_URL "https://github.com/ReimuNotMoe/ydotool")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE ${CMAKE_PROJECT_HOMEPAGE_URL})
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.14), libstdc++6 (>= 5.2), libuinputplus (>= 0.1.3), libevdevplus (>= 0.1.0)")
set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}_${PROJECT_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE})
set(CPACK_DEBIAN_DEBUGINFO_PACKAGE yes)
set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")
set(CPACK_GENERATOR "DEB;RPM;TXZ")
set(CPACK_SOURCE_GENERATOR "DEB;RPM;TXZ")
include(CPack)

include(GNUInstallDirs)

set(SOURCE_FILES_LIBRARY
        CommonIncludes.hpp
        Library/libydotool.cpp Library/libydotool.hpp
        Library/Tool.hpp Library/Tool.cpp
#        Library/Transport.cpp Library/Transport.hpp
        Library/Instance.cpp Library/Instance.hpp
        Library/Utils.cpp Library/Utils.hpp Tools/Tools.hpp
        Tools/MouseMove/MouseMove.hpp Tools/MouseMove/MouseMove.cpp
        Tools/Key/Key.hpp Tools/Key/Key.cpp
        Tools/Click/Click.hpp Tools/Click/Click.cpp
        Tools/Type/Type.hpp Tools/Type/Type.cpp
        Tools/Recorder/Recorder.cpp Tools/Recorder/Recorder.hpp
        )
#        Library/Transports/UnixSocket.cpp Library/Transports/UnixSocket.hpp )

set(SOURCE_FILES_DAEMON
        CommonIncludes.hpp
        Daemon/ydotoold.cpp Daemon/ydotoold.hpp)


set(SOURCE_FILES_CLIENT
        CommonIncludes.hpp
        Client/ydotool.cpp Client/ydotool.hpp)

add_library(ydotool_library SHARED ${SOURCE_FILES_LIBRARY})
set_target_properties(ydotool_library PROPERTIES OUTPUT_NAME ydotool)
add_library(ydotool_library_static STATIC ${SOURCE_FILES_LIBRARY})
set_target_properties(ydotool_library_static PROPERTIES OUTPUT_NAME ydotool)
target_link_libraries(ydotool_library dl boost_program_options uInputPlus evdevPlus)
target_link_libraries(ydotool_library_static dl boost_program_options uInputPlus evdevPlus)

link_libraries(-L.)
add_executable(ydotoold ${SOURCE_FILES_DAEMON})
target_link_libraries(ydotoold ydotool_library dl pthread boost_program_options uInputPlus evdevPlus)

add_executable(ydotool_client ${SOURCE_FILES_CLIENT})
target_link_libraries(ydotool_client ydotool_library boost_program_options pthread uInputPlus evdevPlus)
set_target_properties(ydotool_client PROPERTIES OUTPUT_NAME ydotool)

add_executable(ydotool_client_static ${SOURCE_FILES_CLIENT})
target_link_libraries(ydotool_client_static ydotool_library_static boost_program_options pthread uInputPlus evdevPlus -static)
set_target_properties(ydotool_client_static PROPERTIES OUTPUT_NAME ydotool_static)

#add_library(mousemove SHARED Tools/MouseMove/MouseMove.hpp Tools/MouseMove/MouseMove.cpp)
#target_link_libraries(mousemove ydotool_library boost_program_options uInputPlus evdevPlus)
#
#add_library(key SHARED Tools/Key/Key.hpp Tools/Key/Key.cpp)
#target_link_libraries(key ydotool_library boost_program_options uInputPlus evdevPlus)
#
#add_library(click SHARED Tools/Click/Click.hpp Tools/Click/Click.cpp)
#target_link_libraries(click ydotool_library boost_program_options uInputPlus evdevPlus)
#
#add_library(type SHARED Tools/Type/Type.hpp Tools/Type/Type.cpp)
#target_link_libraries(type ydotool_library boost_program_options uInputPlus evdevPlus)
#
#add_library(recorder SHARED Tools/Recorder/Recorder.cpp Tools/Recorder/Recorder.hpp)
#target_link_libraries(recorder ydotool_library boost_program_options uInputPlus evdevPlus)


install(TARGETS ydotool_library ydotool_library_static DESTINATION lib)
#install(TARGETS mousemove key click type recorder DESTINATION lib/ydotool)
install(TARGETS ydotoold ydotool_client DESTINATION bin)