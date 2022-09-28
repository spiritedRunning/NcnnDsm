cmake_minimum_required(VERSION 3.4.1)

add_library(
        dsm
        SHARED
        src/main/cpp/algo_native.cpp
        src/main/cpp/algo/mtcnn/mtcnn.cpp
        src/main/cpp/algo/kcf/fhog.cpp
        src/main/cpp/algo/kcf/kcftracker.cpp
        src/main/cpp/algo/classifier.cpp
        src/main/cpp/algo/objectDetect.cpp
        src/main/cpp/algo/algo.cpp
        src/main/cpp/algo/detectNcnn.cpp
        )

set(EXTERNAL_LIBS ${CMAKE_SOURCE_DIR}/../include/libs)
message(STATUS "EXTERNAL_LIBS path: ${EXTERNAL_LIBS}")

#add_library(caffe SHARED IMPORTED)
#set_target_properties(caffe PROPERTIES IMPORTED_LOCATION
#        ${CMAKE_SOURCE_DIR}/../include/libs/libcaffe.so)

#add_library(opencvjava3 SHARED IMPORTED)
#set_target_properties(opencvjava3 PROPERTIES IMPORTED_LOCATION
#        ${CMAKE_SOURCE_DIR}/../include/libs/libopencv_java3.so)


# 添加编译选项 -O3 最大优化，其余一次 -Os -O2 -O1
ADD_DEFINITIONS(-DCPU_ONLY)
ADD_DEFINITIONS(--std=c++11 -O3 -DNDEBUG -fPIC -march=armv8-a -mtune=cortex-a53 -fomit-frame-pointer -fstrict-aliasing -funswitch-loops -finline-limit=64 -finline-functions -fwhole-program -fexceptions -frtti -Wno-psabi -funwind-tables -fsigned-char -no-canonical-prefixes -fdata-sections -ffunction-sections -Wa,--noexecstack -Wall -Wno-sign-compare -Wno-uninitialized)
ADD_DEFINITIONS(-fvisibility=hidden -fvisibility-inlines-hidden -fomit-frame-pointer -fstrict-aliasing -ffunction-sections -fdata-sections -ffast-math)
ADD_DEFINITIONS(-DANDROID_PLATFORM=android-21 -DANDROID_NDK -DDISABLE_IMPORTGL -DANDROID_TOOLCHAIN=clang -DANDROID_STL=gnustl_static)

set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -Wl,--no-warn-mismatch  -Wl,--no-undefined -Wl,-allow-shlib-undefined -Wl,--gc-sections -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now")

link_directories("${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/")

include_directories("${CMAKE_SOURCE_DIR}/../include/")

target_link_libraries(
        dsm
        ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libncnn.a
        ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libalpu.so
        ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libcaffe.so
        ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libopencv_java3.so
        z
        EGL
        GLESv2
        android
        jnigraphics
        log)