cmake_minimum_required(VERSION 2.5)
set(APP_SRC main.cpp crawler.cpp httpClient.cpp bloomFilter.cpp hash.cpp)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lpthread -levent -fpermissive -g")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
add_executable(crawler ${APP_SRC})
