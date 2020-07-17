add_executable(ping_pong ping_pong.cpp)

target_include_directories(ping_pong PRIVATE ${Viaduct_SOURCE_DIR}/include)
target_link_libraries(ping_pong viaduct ${Boost_LIBRARIES})
