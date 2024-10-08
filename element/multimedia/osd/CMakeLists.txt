cmake_minimum_required(VERSION 3.10)
project(algorithmAPI)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}  -fprofile-arcs -g")

if (NOT DEFINED TARGET_ARCH)
    set(TARGET_ARCH pcie)
endif()

if (${TARGET_ARCH} STREQUAL "pcie")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -pthread -fpermissive")

    set(FFMPEG_DIR  /opt/sophon/sophon-ffmpeg-latest/lib/cmake)
    find_package(FFMPEG REQUIRED)
    include_directories(${FFMPEG_INCLUDE_DIRS})
    link_directories(${FFMPEG_LIB_DIRS})

    set(OpenCV_DIR  /opt/sophon/sophon-opencv-latest/lib/cmake/opencv4)
    find_package(OpenCV REQUIRED)
    include_directories(${OpenCV_INCLUDE_DIRS})
    link_directories(${OpenCV_LIB_DIRS})

    set(LIBSOPHON_DIR  /opt/sophon/libsophon-current/data/libsophon-config.cmake)
    find_package(LIBSOPHON REQUIRED)
    include_directories(${LIBSOPHON_INCLUDE_DIRS})
    link_directories(${LIBSOPHON_LIB_DIRS})

    set(BM_LIBS bmlib bmrt bmcv yuv)
    find_library(BMJPU bmjpuapi)
    if(BMJPU)
        set(JPU_LIBS bmjpuapi bmjpulite)
    endif()

    include_directories(../../../framework)
    include_directories(../../../framework/include)

    include_directories(../../../3rdparty/spdlog/include)
    include_directories(../../../3rdparty/nlohmann-json/include)
    include_directories(../../../3rdparty/httplib)

    include_directories(include)
    add_library(osd SHARED
        src/osd.cc
    )
    add_library(cvunitext SHARED src/cvUniText.cc)
    target_link_libraries(osd cvunitext)
    add_dependencies(cvunitext freetype)
    target_link_libraries(cvunitext freetype ${OPENCV_LIBS})

    add_dependencies(osd ivslogger framework cvunitext)
    add_dependencies(osd ivslogger framework)
    target_link_libraries(osd ${BM_LIBS} ${FFMPEG_LIBS} ${OpenCV_LIBS} ${JPU_LIBS} -lpthread)

elseif (${TARGET_ARCH} STREQUAL "soc")
    add_compile_options(-fPIC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}  -fprofile-arcs -ftest-coverage -g -rdynamic")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}  -fprofile-arcs -ftest-coverage -rdynamic -fpermissive")
    set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
    set(CMAKE_ASM_COMPILER aarch64-linux-gnu-gcc)
    set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

    set(BM_LIBS bmlib bmrt bmcv yuv)
    find_library(BMJPU bmjpuapi)
    if(BMJPU)
        set(JPU_LIBS bmjpuapi bmjpulite)
    endif()
    set(OPENCV_LIBS opencv_imgproc opencv_core opencv_highgui opencv_imgcodecs opencv_videoio opencv_video)
    include_directories("${SOPHON_SDK_SOC}/include/")
    include_directories("${SOPHON_SDK_SOC}/include/opencv4")
    link_directories("${SOPHON_SDK_SOC}/lib/")
    
    include_directories(../../../framework)
    include_directories(../../../framework/include)

    include_directories(../../../3rdparty/spdlog/include)
    include_directories(../../../3rdparty/nlohmann-json/include)
    include_directories(../../../3rdparty/httplib)

    include_directories(include)
    add_library(osd SHARED
        src/osd.cc
    )
    add_library(cvunitext SHARED src/cvUniText.cc)
    target_link_libraries(${demo_name}  cvunitext)
    add_dependencies(cvunitext freetype)
    target_link_libraries(cvunitext freetype ${OPENCV_LIBS})

    add_dependencies(osd ivslogger framework cvunitext)
    add_dependencies(osd ivslogger framework)
    target_link_libraries(osd ${BM_LIBS} ${OPENCV_LIBS} ${JPU_LIBS} -fprofile-arcs -lgcov -lpthread -lavcodec -lavformat -lavutil)
endif()
