add_subdirectory(cvercmp)
if(BUILD_LUA)
  add_subdirectory(lua-5.3)
endif()
if(BUILD_LUA OR BUILD_TOLUA)
  # If we build lua, need to make sure tolua is compatible, so build it ourselves
  add_subdirectory(tolua-5.2)
endif()