# SPDX-License-Identifier: GPLv3-or-later
# SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>
# SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

# This file runs a collection of git commands to help automate the process
# of generating the build revision. It takes an update to 3 files by hand
# to do it manually and automates it.

# In case git doesn't work, we still need the variables set.
#   So we get them from AutoRevision.txt instead that is updated by a GitHub Action at Release
# See .git/workflows/release.yaml
file(READ cmake/AutoRevision.txt FC21_REV_HEAD_HASH_H LIMIT 5)
string(REGEX REPLACE "\n$" "" FC21_REV_HEAD_HASH_H "${FC21_REV_HEAD_HASH_H}")

file(READ cmake/AutoRevision.txt FC21_REV_TAG OFFSET 6)

if ("${FC21_REV_TAG}" MATCHES dev)
  set(use_git_tag FALSE)  # There is no tag while in dev mode
else()
  set(use_git_tag TRUE)  # Tracks whether we use git. Allows to limit indentation.
endif()

# Auto Revision works best with git
find_package(Git)
if (Git_FOUND)
  # Get the value of the git hash at HEAD to 5 chars
  execute_process(
    COMMAND ${GIT_EXECUTABLE} --git-dir=${CMAKE_SOURCE_DIR}/.git rev-parse --short=5 HEAD
    OUTPUT_VARIABLE FC21_REV_HEAD_HASH_H
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE STATUS
    ERROR_QUIET)

  if (NOT STATUS EQUAL 0)
    # Can't use git
    message(STATUS "Could not get revision from git")
    set(use_git_tag FALSE)
  endif()
else()
  set(use_git_tag FALSE)
endif()

if (use_git_tag)
  # Use the temp value to get the latest revision tag
  execute_process(
    COMMAND ${GIT_EXECUTABLE} --git-dir=${CMAKE_SOURCE_DIR}/.git describe --tags --abbrev=0
    OUTPUT_VARIABLE git_tag
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE STATUS
    ERROR_QUIET)

  if (STATUS EQUAL 0)
    set(FC21_REV_TAG ${git_tag})
    message(STATUS "Latest Git Tag: ${FC21_REV_TAG}")
  else()
    # Can't use git
    message(STATUS "Could not find a git tag")
    set(use_git FALSE)
  endif()
endif()

# Convert the hexadecimal hash to a decimal number to support the project()
math(EXPR FC21_REV_HEAD_HASH_D "0x${FC21_REV_HEAD_HASH_H}" OUTPUT_FORMAT DECIMAL)
message(STATUS "Git commit hash: (hex) ${FC21_REV_HEAD_HASH_H} and (dec) ${FC21_REV_HEAD_HASH_D}")

# Manipulate the tag so we can turn it into a list for use later
string(STRIP "${FC21_REV_TAG}" FC21_REV_TAG)
message(STATUS "Version tag: ${FC21_REV_TAG}")

# Expected scheme:
#  v3.0-dev
#  v3.0-alpha.1
#  v3.0-beta.1
#  v3.0-rc.1
#  v3.0
#  v3.0.1
if (NOT "${FC21_REV_TAG}"
    MATCHES "^v[0-9]+\.[0-9]+(\.[0-9]+)?(-((((alpha)|(beta)|(rc))\.[0-9]+)|(dev)))?$")
  message(SEND_ERROR "The version tag '${FC21_REV_TAG}' does not follow the expected format.")
endif()

string(SUBSTRING "${FC21_REV_TAG}" 1 -1 FC21_REV_TAG) # Remove initial v
string(STRIP "${FC21_REV_TAG}" FC21_REV_TAG2)
string(REPLACE "." " " FC21_REV_TAG2 "${FC21_REV_TAG2}")
string(REPLACE "-" " " FC21_REV_TAG2 "${FC21_REV_TAG2}")
set(FC21_REV_TAG_LIST ${FC21_REV_TAG2})
separate_arguments(FC21_REV_TAG_LIST)
