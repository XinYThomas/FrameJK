cmake_minimum_required(VERSION 4.0)
project(Frame)

set(CMAKE_CXX_STANDARD 17)

# app name
set(APPNAME FrameJK)

# include
include_directories(/usr/local/include)
include_directories(${PROJECT_SOURCE_DIR}/include)

# cpp
file(GLOB DOWNSTREAM_FILES "${PROJECT_SOURCE_DIR}/src/downstream/*.cpp")
list(FILTER DOWNSTREAM_FILES EXCLUDE REGEX "\\._.*$")

# lib
link_directories(/usr/local/lib)

#add_executable(${APPNAME} main.cpp
#    ${DOWNSTREAM_FILES}
#        include/TransCtx.h
#        src/threadpool/threadpool.cpp
#        include/threadpool.h
#        include/Logger.h
#        include/ConfLoad.h
#        include/Tool.h
#)

add_executable(${APPNAME} main.cpp
        include/Tool.h
        include/ConfLoad.h
        include/InitConf.h
        include/TaskQueue.h
        include/CommentCtrl.h
        include/CreateShare.h
)

target_link_libraries(${APPNAME}
        -lspdlog
        -lcurl
        -lcjson
)
