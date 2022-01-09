add_library(sol2 INTERFACE)
target_compile_definitions(sol2 INTERFACE SOL_ALL_SAFETIES_ON=1)
target_include_directories(sol2 INTERFACE "${CMAKE_CURRENT_LIST_DIR}")
