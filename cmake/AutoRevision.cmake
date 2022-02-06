# This file runs a collection of git commands to help automate the process
# of generating the build revision. It takes an update to 3 files by hand
# to do it manually and automates it.

set(ok true) # Tracks whether we use git. Allows to limit indentation.

# Auto Revision needs git to work
find_package(Git)
if (NOT Git_FOUND)
  set(ok false)
endif()

if (ok)
  # Get the value of the git hash at HEAD to 5 chars
  execute_process(
    COMMAND ${GIT_EXECUTABLE} --git-dir=${CMAKE_SOURCE_DIR}/.git rev-parse --short=5 HEAD
    OUTPUT_VARIABLE FC21_REV_HEAD_HASH_H
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE STATUS
    ERROR_QUIET)

  if (STATUS EQUAL 0)
    # Convert the hexadecimal hash to a decimal number to support the project()
    math(EXPR FC21_REV_HEAD_HASH_D "0x${FC21_REV_HEAD_HASH_H}" OUTPUT_FORMAT DECIMAL)
    message(STATUS "Git HEAD Commit Hash: (hex) ${FC21_REV_HEAD_HASH_H} and (dec) ${FC21_REV_HEAD_HASH_D}")
  else()
    # Can't use git
    message(STATUS "Could not get revision from git")
    set(ok false)
  endif()
endif()

if (ok)
  # Get a temp value of the full commit hash of the latest tag that is active
  execute_process(
    COMMAND ${GIT_EXECUTABLE} --git-dir=${CMAKE_SOURCE_DIR}/.git rev-list --tags --max-count=1
    OUTPUT_VARIABLE hash
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE STATUS
    ERROR_QUIET)

  if (STATUS EQUAL 0)
    message(STATUS "Last tag commit hash: ${hash}")
  else()
    # Can't use git
    message(STATUS "Could not find a git tag")
    set(ok false)
  endif()
endif()

if (ok)
  # Use the temp value to get the latest revision tag
  execute_process(
    COMMAND ${GIT_EXECUTABLE} --git-dir=${CMAKE_SOURCE_DIR}/.git describe --tags ${hash}
    OUTPUT_VARIABLE FC21_REV_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE STATUS
    ERROR_QUIET)

  if (STATUS EQUAL 0)
    message(STATUS "Latest Git Tag: ${FC21_REV_TAG}")
    # Manipulate the tag so we can turn it into a list for use later
    string(REPLACE "v" "" FC21_REV_TAG "${FC21_REV_TAG}")
    string(REPLACE "." " " FC21_REV_TAG "${FC21_REV_TAG}")
    string(REPLACE "-" " " FC21_REV_TAG "${FC21_REV_TAG}")
    set(FC21_REV_TAG_LIST ${FC21_REV_TAG})
    separate_arguments(FC21_REV_TAG_LIST)
  else()
    # Can't use git
    message(STATUS "Could not find a git tag")
    set(ok false)
  endif()
endif()

if (NOT ok)
  # In this case git didn't work, but we still need the variables set.
  #   So we get them from AutoRevision.txt instead that is updated by a GitHub Action at Release
  # See .git/workflows/release.yaml

  message(STATUS "Could not fetch version information from git. Using AutoRevision.txt")
  file(READ cmake/AutoRevision.txt FC21_REV_HEAD_HASH_H LIMIT 5)
  string(REGEX REPLACE "\n$" "" FC21_REV_HEAD_HASH_H "${FC21_REV_HEAD_HASH_H}")

  # Convert the hexadecimal hash to a decimal number to support the project()
  math(EXPR FC21_REV_HEAD_HASH_D "0x${FC21_REV_HEAD_HASH_H}" OUTPUT_FORMAT DECIMAL)
  message(STATUS "AutoRevision HEAD Commit Hash: (hex) ${FC21_REV_HEAD_HASH_H} and (dec) ${FC21_REV_HEAD_HASH_D}")

  file(READ cmake/AutoRevision.txt FC21_REV_TAG OFFSET 6)
  string(REGEX REPLACE "\n$" "" FC21_REV_TAG "${FC21_REV_TAG}")

  # Manipulate the tag so we can turn it into a list for use later
  string(REPLACE "v" "" FC21_REV_TAG2 "${FC21_REV_TAG}")
  string(REPLACE "." " " FC21_REV_TAG2 "${FC21_REV_TAG2}")
  string(REPLACE "-" " " FC21_REV_TAG2 "${FC21_REV_TAG2}")
  set(FC21_REV_TAG_LIST ${FC21_REV_TAG2})
  separate_arguments(FC21_REV_TAG_LIST)
endif()
