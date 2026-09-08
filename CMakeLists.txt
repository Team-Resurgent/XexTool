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
target_include_directories(xecrypt PUBLIC
    third_party/XeCrypt
    third_party/XeCrypt/aes third_party/XeCrypt/md5 third_party/XeCrypt/rc4
    third_party/XeCrypt/sha third_party/XeCrypt/bign third_party/XeCrypt/des)

# --- libLZX ---------------------------------------------------------------
add_library(lzx STATIC
    third_party/libLZX/lzx/lzx_encoder.c
    third_party/libLZX/lzx/lzx_decoder.c)
target_include_directories(lzx PUBLIC third_party/libLZX)

# --- XexTool --------------------------------------------------------------
file(GLOB XEXTOOL_SOURCES src/*.cpp src/*.c src/lzx/*.cpp)
add_executable(XexTool ${XEXTOOL_SOURCES} third_party/tinyxml2/tinyxml2.cpp)
target_include_directories(XexTool PRIVATE src src/lzx third_party/tinyxml2)
target_link_libraries(XexTool PRIVATE xecrypt lzx)

install(TARGETS XexTool RUNTIME DESTINATION bin)
