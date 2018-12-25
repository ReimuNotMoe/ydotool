cmake_minimum_required(VERSION 3.8)
project(ydotool)

set(CMAKE_CXX_STANDARD 11)
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

set(SOURCE_FILES
        ydotool.cpp CommonIncludes.hpp Commands.hpp Commands.cpp
        Commands/Type.cpp Commands/Key.cpp Commands/MouseMove.cpp Commands/Click.cpp)


add_executable(ydotool ${SOURCE_FILES})
target_link_libraries(ydotool boost_program_options uInputPlus evdevPlus)

install(TARGETS ydotool DESTINATION bin)