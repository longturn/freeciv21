# Support for sphinx-build
include(FindPackageHandleStandardArgs)

if(PythonInterp_FOUND)
  get_filename_component(_PYTHON_DIR "${PYTHON_EXECUTABLE}" DIRECTORY)
    set(_PYTHON_PATHS
        "${_PYTHON_DIR}"
        "${_PYTHON_DIR}/bin"
        "${_PYTHON_DIR}/Scripts")
  find_program(
    SPHINX_EXECUTABLE
    NAMES sphinx-build sphinx-build.exe
    HINTS ${_PYTHON_PATHS}
    DOC "Path to sphinx-build executable")
  mark_as_advanced(SPHINX_EXECUTABLE)
  find_package_handle_standard_args(Sphinx DEFAULT_MSG SPHINX_EXECUTABLE)
endif()