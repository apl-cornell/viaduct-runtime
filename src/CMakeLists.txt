add_library(viaduct STATIC
  Socket.cpp
  ViaductRuntime.cpp
  ${Viaduct_SOURCE_DIR}/include/viaduct/ViaductRuntime.h
  ${Viaduct_SOURCE_DIR}/include/viaduct/Socket.h
  ${Viaduct_SOURCE_DIR}/include/viaduct/SyncQueue.h
)

set_target_properties(viaduct PROPERTIES
  PUBLIC_HEADER 
  ${Viaduct_SOURCE_DIR}/include/viaduct/ViaductRuntime.h
  ${Viaduct_SOURCE_DIR}/include/viaduct/Socket.h
  ${Viaduct_SOURCE_DIR}/include/viaduct/SyncQueue.h
  )

target_include_directories(viaduct PUBLIC ${Viaduct_SOURCE_DIR}/include)

install(TARGETS viaduct
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include/viaduct)