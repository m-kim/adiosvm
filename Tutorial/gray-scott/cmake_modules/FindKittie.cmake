find_file(KITTIE_COMPOSE kittie-compose.py  HINTS "$ENV{KITTIE_DIR}/bin")
message(STATUS "KITTIE_COMPOSE: ${KITTIE_COMPOSE}")


if(KITTIE_COMPOSE)
	set(KITTIE_FOUND TRUE)
	get_filename_component(KITTIE_BINDIR ${KITTIE_COMPOSE} DIRECTORY)
	get_filename_component(KITTIE_PREFIX ${KITTIE_BINDIR}  DIRECTORY)
	set(KITTIE_INCLUDE_DIRS ${KITTIE_PREFIX}/include) 
	set(KITTIE_LIBRARY_DIRS ${KITTIE_PREFIX}/lib) 
	find_library(kittie_cpp     NAMES kittie   HINTS ${KITTIE_LIBRARY_DIRS})
	find_library(kittie_fortran NAMES kittie_f HINTS ${KITTIE_LIBRARY_DIRS})
else(KITTIE_COMPOSE)
	set(KITTIE_FOUND FALSE)
endif(KITTIE_COMPOSE)


if(NOT KITTIE_FOUND)
	unset(KITTIE_INCLUDE_DIRS)
	unset(kittie_cpp)
	unset(kittie_fortran)
endif(NOT KITTIE_FOUND)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(KITTIE REQUIRED_VARS KITTIE_INCLUDE_DIRS kittie_cpp kittie_fortran)

