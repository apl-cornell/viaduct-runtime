add_library(
  viaduct
  Socket.cpp
  ViaductRuntime.cpp
  ${Viaduct_SOURCE_DIR}/include/viaduct/Socket.h
  ${Viaduct_SOURCE_DIR}/include/viaduct/SyncQueue.h
  ${Viaduct_SOURCE_DIR}/include/viaduct/ViaductRuntime.h
)

target_include_directories(viaduct PUBLIC ${Viaduct_SOURCE_DIR}/include)

install(TARGETS viaduct DESTINATION bin)