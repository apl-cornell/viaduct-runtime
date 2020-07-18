add_library(viaduct STATIC
  Socket.cpp
  ViaductRuntime.cpp
  ${Viaduct_SOURCE_DIR}/include/viaduct/ViaductRuntime.h
  ${Viaduct_SOURCE_DIR}/include/viaduct/Socket.h
  ${Viaduct_SOURCE_DIR}/include/viaduct/SyncQueue.h
)

target_include_directories(viaduct
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(viaduct PUBLIC pthread)

install(TARGETS viaduct
        EXPORT viaduct
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include/viaduct
)

export(
  TARGETS viaduct
  NAMESPACE viaduct::
  FILE "viaductTargets.cmake"
)

install(EXPORT viaduct
  FILE
    viaductTargets.cmake
  NAMESPACE
    viaduct::
  DESTINATION
    lib/cmake/viaduct
)

install(FILES
  ${Viaduct_SOURCE_DIR}/cmake/viaductConfig.cmake
  DESTINATION lib/cmake/viaduct
)
