cmake_minimum_required(VERSION 3.0)
project(fastcgi++ LANGUAGES CXX)

set(VERSION      "3.0beta")
set(VERSION_MAJOR 3)
string(TIMESTAMP BUILD_TIME UTC)

# All our starting point lists
set(SRC_FILES
    "src/log.cpp"
    "src/block.cpp"
    "src/http.cpp"
    "src/protocol.cpp"
    "src/sockets.cpp"
    "src/transceiver.cpp"
    "src/fcgistreambuf.cpp"
    "src/webstreambuf.cpp"
    "src/request.cpp"
    "src/manager.cpp")
set(TESTS
    "protocol"
    "http"
    "sockets"
    "transceiver"
    "fcgistreambuf")
set(EXAMPLES
    "helloworld"
    "echo"
    "gnu"
    "sessions"
    "timer")

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
	set(CMAKE_MACOSX_RPATH ON)
    else()
        set(CMAKE_CXX_FLAGS
            "$ENV{CXXFLAGS} -std=c++14 -Wall -Werror -Wold-style-cast -pthread")
        set(CMAKE_CXX_FLAGS_RELEASE
            "$ENV{CXXFLAGS} -O3 -fomit-frame-pointer -std=c++14 -Wall -Werror -Wold-style-cast -pthread")
        set(CMAKE_CXX_FLAGS_DEBUG
            "$ENV{CXXFLAGS} -O0 -fno-omit-frame-pointer -g -ggdb -std=c++14 -Wall -Werror -Wold-style-cast -pthread")
    endif()
endif()

# Our configuration
include_directories(
    "${CMAKE_CURRENT_BINARY_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/fastcgi++/config.hpp" @ONLY)

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

# Are we worrying about SQL?
option(BUILD_SQL "Set to ON to build and install the SQL components" OFF)
if(BUILD_SQL)
    find_package(PostgreSQL REQUIRED)
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/include/fastcgi++/sql/parameters.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/fastcgi++/sql/results.hpp"
        DESTINATION "include/fastcgi++/sql")
    link_libraries(${PostgreSQL_LIBRARIES})
    include_directories(${PostgreSQL_INCLUDE_DIRS})
    list(APPEND TESTS "sql")
    list(APPEND SRC_FILES "src/parameters.cpp")
    list(APPEND SRC_FILES "src/results.cpp")
endif(BUILD_SQL)

# Build the library itself
option(BUILD_STATIC_LIBS "Set to ON to build and install only the static library" OFF)
if(BUILD_STATIC_LIBS)
    add_library(fastcgipp STATIC ${SRC_FILES})
    install(TARGETS fastcgipp ARCHIVE DESTINATION lib${LIB_SUFFIX})
else(BUILD_STATIC_LIBS)
    add_library(fastcgipp SHARED ${SRC_FILES})
    install(TARGETS fastcgipp LIBRARY DESTINATION lib${LIB_SUFFIX})
endif(BUILD_STATIC_LIBS)
set_target_properties(fastcgipp PROPERTIES VERSION ${VERSION}
                                           SOVERSION ${VERSION_MAJOR})

# All the test stuff
enable_testing()
foreach(UNITTEST ${TESTS})
    add_executable(${UNITTEST}_test EXCLUDE_FROM_ALL tests/${UNITTEST}.cpp)
    add_dependencies(${UNITTEST}_test fastcgipp)
    target_link_libraries(${UNITTEST}_test PRIVATE fastcgipp)
    add_test("${UNITTEST}" ${UNITTEST}_test)
    list(APPEND TEST_TARGET ${UNITTEST}_test)
endforeach()
add_custom_target(tests DEPENDS ${TEST_TARGET})

# Examples
foreach(EXAMPLE ${EXAMPLES})
    add_executable(${EXAMPLE}.fcgi EXCLUDE_FROM_ALL examples/${EXAMPLE}.cpp)
    add_dependencies(${EXAMPLE}.fcgi fastcgipp)
    target_link_libraries(${EXAMPLE}.fcgi PRIVATE fastcgipp)
    list(APPEND EXAMPLE_TARGETS ${EXAMPLE}.fcgi)
endforeach()
add_custom_target(examples DEPENDS ${EXAMPLE_TARGETS})

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
