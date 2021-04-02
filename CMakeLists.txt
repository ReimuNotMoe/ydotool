cmake_minimum_required(VERSION 3.14)
project(ydotool)

set(CMAKE_CXX_STANDARD 17)

set(CPM_DOWNLOAD_VERSION 0.27.5)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake")
    file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()

include(${CPM_DOWNLOAD_LOCATION})

CPMAddPackage(
        NAME IODash
        GITHUB_REPOSITORY YukiWorkshop/IODash
        VERSION 0.1.0
)

CPMAddPackage(
        NAME libevdevPlus
        GITHUB_REPOSITORY YukiWorkshop/libevdevPlus
        VERSION 0.2.1
)

CPMAddPackage(
        NAME libuInputPlus
        GITHUB_REPOSITORY YukiWorkshop/libuInputPlus
        VERSION 0.2.1
)

CPMAddPackage(
        NAME cxxopts
        GITHUB_REPOSITORY jarro2783/cxxopts
        VERSION 3.0.0
        GIT_TAG 2d8e17c4f88efce80e274cb03eeb902e055a91d3
)

set(SOURCE_FILES_LIBRARY
        CommonIncludes.hpp
        Library/Tool.hpp Library/Tool.cpp
        Library/Utils.cpp Library/Utils.hpp
        Tools/Tools.hpp
        Tools/Sleep/Sleep.hpp Tools/Sleep/Sleep.cpp
        Tools/MouseMove/MouseMove.hpp Tools/MouseMove/MouseMove.cpp
        Tools/Key/Key.hpp Tools/Key/Key.cpp
        Tools/Click/Click.hpp Tools/Click/Click.cpp
        Tools/Type/Type.hpp Tools/Type/Type.cpp
        Tools/Recorder/Recorder.cpp Tools/Recorder/Recorder.hpp
        )
#        Library/Transports/UnixSocket.cpp Library/Transports/UnixSocket.hpp )

set(SOURCE_FILES_DAEMON CommonIncludes.hpp Daemon/ydotoold.cpp Daemon/ydotoold.hpp Library/Utils.hpp Library/Utils.cpp)


set(SOURCE_FILES_CLIENT
        CommonIncludes.hpp
        Client/ydotool.cpp Client/ydotool.hpp)

include_directories(${libevdevPlus_SOURCE_DIR})
include_directories(${libuInputPlus_SOURCE_DIR})
include_directories(${IODash_SOURCE_DIR})
include_directories(${IODash_SOURCE_DIR}/cpp_modules/portable-endian) # FIXME
include_directories(${cxxopts_SOURCE_DIR}/include)

add_executable(ydotoold ${SOURCE_FILES_DAEMON})
target_link_libraries(ydotoold PUBLIC uInputPlus evdevPlus)
install(TARGETS ydotoold DESTINATION ${CMAKE_INSTALL_BINDIR})

add_library(ydotool_library STATIC ${SOURCE_FILES_LIBRARY})
add_executable(ydotool ${SOURCE_FILES_CLIENT})
target_link_libraries(ydotool ydotool_library dl pthread uInputPlus evdevPlus)
install(TARGETS ydotool DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(Daemon)
add_subdirectory(manpage)
