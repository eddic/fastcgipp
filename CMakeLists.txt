cmake_minimum_required(VERSION 2.8)
project(fastcgi++ LANGUAGES CXX)

set(VERSION      "3.0beta")
set(VERSION_MAJOR 3)
string(TIMESTAMP BUILD_TIME UTC)

option(BUILD_STATIC_LIBS "Set to on to build and install static library" OFF)

# Set up our log level for fastcgi++/log.hpp
if(NOT LOG_LEVEL)
    if(CMAKE_BUILD_TYPE STREQUAL "RELEASE")
        set(LOG_LEVEL 1)
    elseif(CMAKE_BUILD_TYPE STREQUAL "DEBUG")
        set(LOG_LEVEL 3)
    else()
        set(LOG_LEVEL 2)
    endif()
endif()

# We'll need this stuff for sockets stuff
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(SYSTEM "LINUX")
elseif(UNIX)
    set(SYSTEM "UNIX")
elseif(WIN32)
    message(FATAL_ERROR "Windows systems not supported")
    set(SYSTEM "WINDOWS")
else()
    message(FATAL_ERROR "Unknown operating system")
endif()

# Set compile flags for gcc and clang
if(UNIX)
    # Mac Os Don`t use pthread to build this project
    if(APPLE)
        set(CMAKE_CXX_FLAGS
                "$ENV{CXXFLAGS} -std=c++14 -Wall -Werror -Wold-style-cast")
        set(CMAKE_CXX_FLAGS_RELEASE
                "$ENV{CXXFLAGS} -O3 -fomit-frame-pointer -std=c++14 -Wall -Werror -Wold-style-cast")
        set(CMAKE_CXX_FLAGS_DEBUG
                "$ENV{CXXFLAGS} -O0 -fno-omit-frame-pointer -g -ggdb -std=c++14 -Wall -Werror -Wold-style-cast")
    else()
        set(CMAKE_CXX_FLAGS
            "$ENV{CXXFLAGS} -std=c++14 -Wall -Werror -Wold-style-cast -pthread")
        set(CMAKE_CXX_FLAGS_RELEASE
            "$ENV{CXXFLAGS} -O3 -fomit-frame-pointer -std=c++14 -Wall -Werror -Wold-style-cast -pthread")
        set(CMAKE_CXX_FLAGS_DEBUG
            "$ENV{CXXFLAGS} -O0 -fno-omit-frame-pointer -g -ggdb -std=c++14 -Wall -Werror -Wold-style-cast -pthread")
    endif()
endif()

include_directories(
    "${CMAKE_CURRENT_BINARY_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Our configuration 
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/fastcgi++/config.hpp" @ONLY)

# The shared library itself
add_library(fastcgipp SHARED
    src/log.cpp
    src/block.cpp
    src/http.cpp
    src/protocol.cpp
    src/sockets.cpp
    src/transceiver.cpp
    src/fcgistreambuf.cpp
    src/webstreambuf.cpp
    src/request.cpp
    src/manager.cpp)
set_target_properties(fastcgipp PROPERTIES VERSION ${VERSION}
                                           SOVERSION ${VERSION_MAJOR})
install(TARGETS fastcgipp LIBRARY DESTINATION lib${LIB_SUFFIX})


# A static version of the library
if (BUILD_STATIC_LIBS)
    add_library(fastcgipp-static STATIC
        src/log.cpp
        src/block.cpp
        src/http.cpp
        src/protocol.cpp
        src/sockets.cpp
        src/transceiver.cpp
        src/fcgistreambuf.cpp
        src/webstreambuf.cpp
        src/request.cpp
        src/manager.cpp)
    install(TARGETS fastcgipp-static ARCHIVE DESTINATION lib${LIB_SUFFIX})
endif (BUILD_STATIC_LIBS)

# Install the header file
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/include/fastcgi++/config.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fastcgi++/block.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fastcgi++/fcgistreambuf.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fastcgi++/http.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fastcgi++/log.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fastcgi++/manager.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fastcgi++/message.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fastcgi++/protocol.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fastcgi++/request.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fastcgi++/sockets.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fastcgi++/transceiver.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/fastcgi++/webstreambuf.hpp"
    DESTINATION "include/fastcgi++")

# All the test stuff
enable_testing()

add_executable(protocol_test EXCLUDE_FROM_ALL tests/protocol.cpp)
add_dependencies(protocol_test fastcgipp)
target_link_libraries(protocol_test PRIVATE fastcgipp)
add_test("Fastcgipp::Protocol" protocol_test)

add_executable(http_test EXCLUDE_FROM_ALL tests/http.cpp)
add_dependencies(http_test fastcgipp)
target_link_libraries(http_test PRIVATE fastcgipp)
add_test("Fastcgipp::Http" http_test)

add_executable(sockets_test EXCLUDE_FROM_ALL tests/sockets.cpp)
add_dependencies(sockets_test fastcgipp)
target_link_libraries(sockets_test PRIVATE fastcgipp)
add_test("Fastcgipp::Sockets" sockets_test)

add_executable(transceiver_test EXCLUDE_FROM_ALL tests/transceiver.cpp)
add_dependencies(transceiver_test fastcgipp)
target_link_libraries(transceiver_test PRIVATE fastcgipp)
add_test("Fastcgipp::Transceiver" transceiver_test)

add_executable(fcgistreambuf_test EXCLUDE_FROM_ALL tests/fcgistreambuf.cpp)
add_dependencies(fcgistreambuf_test fastcgipp)
target_link_libraries(fcgistreambuf_test PRIVATE fastcgipp)
add_test("Fastcgipp::FcgiStreambuf" fcgistreambuf_test)

add_custom_target(
    tests DEPENDS
    protocol_test
    http_test
    sockets_test
    transceiver_test
    fcgistreambuf_test)

# Examples

add_executable(helloworld.fcgi EXCLUDE_FROM_ALL examples/helloworld.cpp)
add_dependencies(helloworld.fcgi fastcgipp)
target_link_libraries(helloworld.fcgi PRIVATE fastcgipp)

add_executable(echo.fcgi EXCLUDE_FROM_ALL examples/echo.cpp)
add_dependencies(echo.fcgi fastcgipp)
target_link_libraries(echo.fcgi PRIVATE fastcgipp)

add_executable(gnu.fcgi EXCLUDE_FROM_ALL examples/gnu.cpp)
add_dependencies(gnu.fcgi fastcgipp)
target_link_libraries(gnu.fcgi PRIVATE fastcgipp)

add_executable(sessions.fcgi EXCLUDE_FROM_ALL examples/sessions.cpp)
add_dependencies(sessions.fcgi fastcgipp)
target_link_libraries(sessions.fcgi PRIVATE fastcgipp)

add_executable(timer.fcgi EXCLUDE_FROM_ALL examples/timer.cpp)
add_dependencies(timer.fcgi fastcgipp)
target_link_libraries(timer.fcgi PRIVATE fastcgipp)

add_custom_target(
    examples DEPENDS
    echo.fcgi
    gnu.fcgi
    sessions.fcgi
    timer.fcgi
    helloworld.fcgi)

# And finally the documentation
find_package(Doxygen)
if(DOXYGEN_FOUND)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in"
        "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile" @ONLY)
    add_custom_target(
        doc
        "${DOXYGEN_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Generating API documentation with Doxygen" VERBATIM)
    install(
        DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doc"
        DESTINATION "share/fastcgi++"
        OPTIONAL)
endif(DOXYGEN_FOUND)
