tolua_generate(
  INPUT tolua_client.pkg
  HEADER tolua_client_gen.h
  SOURCE tolua_client_gen.c
  PACKAGE_NAME client
)

add_library(
  luascript
  STATIC
  api_client_base.c
  script_client.c
  # Generated
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_client_gen.c
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_client_gen.h
)

target_include_directories(luascript PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(luascript PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(luascript PUBLIC client)
target_link_libraries(luascript PUBLIC tolua)
