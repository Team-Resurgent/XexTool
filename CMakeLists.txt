cmake_minimum_required(VERSION 3.16)
project(XexTool C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/W3)
else()
    # the sources predate several warnings that are now on by default and are
    # not worth churning the code for
    add_compile_options(-Wall -Wno-unused-variable -Wno-unused-but-set-variable
                        -Wno-sign-compare -Wno-write-strings -Wno-multichar)
    # xex name fields are fixed width and deliberately not null terminated, so
    # copying into one with a bound equal to its size is correct
    add_compile_options(-Wno-sizeof-pointer-memaccess)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-Wno-stringop-truncation)
    endif()
endif()

# --- XeCrypt --------------------------------------------------------------
file(GLOB XECRYPT_SOURCES
    third_party/XeCrypt/xecrypt.c
    third_party/XeCrypt/xecryptBn.c
    third_party/XeCrypt/xecryptHmacSha.c
    third_party/XeCrypt/aes/*.c
    third_party/XeCrypt/md5/*.c
    third_party/XeCrypt/rc4/*.c
    third_party/XeCrypt/sha/*.c
    third_party/XeCrypt/bign/*.c
    third_party/XeCrypt/des/*.c)
add_library(xecrypt STATIC ${XECRYPT_SOURCES})
if(NOT MSVC)
    # the reference key parser in bign/ ignores its fgets results
    target_compile_options(xecrypt PRIVATE -Wno-unused-result)
endif()
target_include_directories(xecrypt PUBLIC
    third_party/XeCrypt
    third_party/XeCrypt/aes third_party/XeCrypt/md5 third_party/XeCrypt/rc4
    third_party/XeCrypt/sha third_party/XeCrypt/bign third_party/XeCrypt/des)

# --- libLZX ---------------------------------------------------------------
add_library(lzx STATIC
    third_party/libLZX/lzx/lzx_encoder.c
    third_party/libLZX/lzx/lzx_decoder.c)
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # the tree frequency arrays hold twice as many elements as they have
    # symbols, and only the symbol half is ever cleared
    target_compile_options(lzx PRIVATE -Wno-memset-elt-size)
endif()
target_include_directories(lzx PUBLIC third_party/libLZX)

# --- XexTool --------------------------------------------------------------
file(GLOB XEXTOOL_SOURCES src/*.cpp src/*.c src/lzx/*.cpp)
add_executable(XexTool ${XEXTOOL_SOURCES} third_party/tinyxml2/tinyxml2.cpp)
target_include_directories(XexTool PRIVATE src src/lzx third_party/tinyxml2)
target_link_libraries(XexTool PRIVATE xecrypt lzx)

# --- tests ----------------------------------------------------------------
# a build can compile cleanly and still code wrongly, so round trip real data
# through the compressor rather than trusting a green build
enable_testing()
add_executable(lzx_roundtrip tests/lzx_roundtrip.cpp src/lzx/XexLzx.cpp)
target_include_directories(lzx_roundtrip PRIVATE src/lzx third_party/libLZX)
target_link_libraries(lzx_roundtrip PRIVATE lzx)
add_test(NAME lzx_roundtrip COMMAND lzx_roundtrip)

install(TARGETS XexTool RUNTIME DESTINATION bin)
