# CMakeLists for Freeciv21 Docs

if(FREECIV_ENABLE_MANPAGES)
  find_package(Sphinx REQUIRED)
else()
  find_package(Sphinx QUIET)
endif()

if(SPHINX_FOUND)
  message(STATUS "Sphinx Found, configuring.")
  set(SPHINX_SOURCE ${CMAKE_SOURCE_DIR}/docs)
  set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR})
  set(SPHINX_MAN ${CMAKE_CURRENT_BINARY_DIR}/man)

  add_custom_target(docs
                COMMAND
                ${SPHINX_EXECUTABLE} -b html ${SPHINX_SOURCE} ${SPHINX_BUILD}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Generating documentation with Sphinx")

  # We only create man pages on Unix or if the user explicitly requested it.
  if((UNIX AND NOT APPLE) OR FREECIV_ENABLE_MANPAGES)
    option(FREECIV_ENABLE_MANPAGES "Enable manpages" ON)

    add_custom_target(man-pages ALL
          COMMAND
          ${SPHINX_EXECUTABLE} -b man ${SPHINX_SOURCE} ${SPHINX_MAN}
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
          COMMENT "Generating manual pages with Sphinx")
  endif()
else()
  message(STATUS "Sphinx NOT Found.")
endif()

# Default to OFF, if FREECIV_ENABLE_MANPAGES is not set yet.
option(FREECIV_ENABLE_MANPAGES "Enable manpages" OFF)
