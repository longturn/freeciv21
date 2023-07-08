# CMakeLists for Freeciv21 Docs

set(FREECIV_ENABLE_MANPAGES FALSE)
find_package(Sphinx QUIET)

if(SPHINX_FOUND)
  message(STATUS "Sphinx Found, configuring.")
  set(FREECIV_ENABLE_MANPAGES TRUE)
  set(SPHINX_SOURCE ${CMAKE_SOURCE_DIR}/docs)
  set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR})
  set(SPHINX_MAN ${CMAKE_CURRENT_BINARY_DIR}/man)

  add_custom_target(docs
                COMMAND
                ${SPHINX_EXECUTABLE} -b html ${SPHINX_SOURCE} ${SPHINX_BUILD}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Generating documentation with Sphinx")

  # We only create man pages on Unix
  if(UNIX AND NOT APPLE)
    add_custom_target(man-pages ALL
                COMMAND
                ${SPHINX_EXECUTABLE} -b man ${SPHINX_SOURCE} ${SPHINX_MAN}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Generating manual pages with Sphinx")
  endif()
else()
  message(STATUS "Sphinx NOT Found.")
  set(FREECIV_ENABLE_MANPAGES FALSE)
endif()
