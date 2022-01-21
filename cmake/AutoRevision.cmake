# This file runs a collection of git commands to help automate the process
# of generating the build revision. It takes an update to 3 files by hand
# to do it manually and automates it.

# Auto Revision needs git to work
find_package(Git)

if(Git_FOUND)

  # run git status to see if we are in a real local repo
  execute_process(COMMAND ${GIT_EXECUTABLE} status
                  OUTPUT_VARIABLE check-status
                  OUTPUT_STRIP_TRAILING_WHITESPACE)

  if(NOT ${check-status} MATCHES "fatal: not a git repository (or any of the parent directories): .git")

    # get the value of the git hash at HEAD to 5 chars
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short=5 HEAD
                    OUTPUT_VARIABLE FC21_REV_HEAD_HASH_H
                    OUTPUT_STRIP_TRAILING_WHITESPACE)

    # convert the hexadecimal hash to a decimal number to support the project()
    math(EXPR FC21_REV_HEAD_HASH_D "0x${FC21_REV_HEAD_HASH_H}" OUTPUT_FORMAT DECIMAL)
    message("-- Git HEAD Commit Hash: (hex) ${FC21_REV_HEAD_HASH_H} and (dec) ${FC21_REV_HEAD_HASH_D}")

    # get a temp value of the full commit hash of the latest tag that is active
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-list --tags --max-count=1
                    OUTPUT_VARIABLE _output
                    OUTPUT_STRIP_TRAILING_WHITESPACE)

    # use the temp value to get the latest revision tag
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags ${_output}
                    OUTPUT_VARIABLE FC21_REV_TAG
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    message("-- Latest Git Tag: ${FC21_REV_TAG}")

    # manipulate the tag so we can turn it into a list for use later
    string(REPLACE "v" "" FC21_REV_TAG2 "${FC21_REV_TAG}")
    string(REPLACE "." " " FC21_REV_TAG2 "${FC21_REV_TAG2}")
    string(REPLACE "-" " " FC21_REV_TAG2 "${FC21_REV_TAG2}")
    set(FC21_REV_TAG_LIST ${FC21_REV_TAG2})
    separate_arguments(FC21_REV_TAG_LIST)
    message("-- Latest Git Tag List: ${FC21_REV_TAG_LIST}")

  endif()

else()

  # In case git is not found (for example the person uses the tarball) we still need the variables
  # set. So we get them from AutoRevision.txt instead that is updated by a GitHub Action at Release
  # See .git/workflows/release.yaml

  file(READ cmake/AutoRevision.txt FC21_REV_HEAD_HASH_H LIMIT 5)
  string(REGEX REPLACE "\n$" "" FC21_REV_HEAD_HASH_H "${FC21_REV_HEAD_HASH_H}")

  # convert the hexadecimal hash to a decimal number to support the project()
  math(EXPR FC21_REV_HEAD_HASH_D "0x${FC21_REV_HEAD_HASH_H}" OUTPUT_FORMAT DECIMAL)
  message("-- AutoRevision HEAD Commit Hash: (hex) ${FC21_REV_HEAD_HASH_H} and (dec) ${FC21_REV_HEAD_HASH_D}")

  file(READ cmake/AutoRevision.txt FC21_REV_TAG OFFSET 6)
  string(REGEX REPLACE "\n$" "" FC21_REV_TAG "${FC21_REV_TAG}")

  # manipulate the tag so we can turn it into a list for use later
  string(REPLACE "v" "" FC21_REV_TAG2 "${FC21_REV_TAG}")
  string(REPLACE "." " " FC21_REV_TAG2 "${FC21_REV_TAG2}")
  string(REPLACE "-" " " FC21_REV_TAG2 "${FC21_REV_TAG2}")
  set(FC21_REV_TAG_LIST ${FC21_REV_TAG2})
  separate_arguments(FC21_REV_TAG_LIST)
  message("-- AutoRevision Git Tag List: ${FC21_REV_TAG_LIST}")

endif()
