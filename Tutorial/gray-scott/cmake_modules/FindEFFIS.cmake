function(kpcmake src dest suffix)	
	# This actually runs the pre-processor that replaces the pragmas
	#	* ${src} is just the XGC repo -- the source we want to check for EFFIS pragmas
	# 	* The --tree-output is where the output files will go, and in particular we can send them to the CMake build are
	#	* --confdir is where the the small .kittie-setup.nml will write (which says what EFFIS things are used)
	#	* --suffix is a suffix appended to the processes sourced files. It doesn't really matter here
	execute_process(COMMAND kittie-cpp.py repo ${src} --tree-output=${dest} --suffix=${suffix})
endfunction()

function(listreplace srcfiles effisfiles suffix result)
	# This function goes through a list of source files and checks if effis wrote an output file for any them
	# If so it returns the list to use the EFFIS version of the file

	# This sets up a list of all the possible matches
	set(srcfix "")
	foreach(src ${srcfiles})
		get_filename_component(dir  ${src} DIRECTORY)
		get_filename_component(base ${src} NAME_WE)
		get_filename_component(ext  ${src} EXT)

		string(COMPARE EQUAL "${dir}" "" empty)
		if (empty)
			list(APPEND srcfix "${base}${suffix}${ext}")
		else()
			list(APPEND srcfix "${dir}/${base}${suffix}${ext}")
		endif()

	endforeach()

	# This is where we actually check for matches
	foreach(match ${effisfiles})
		list(FIND srcfix ${match} index)
		if (${index} GREATER -1)
			list(INSERT srcfiles ${index} ${match})
			MATH(EXPR __INDEX "${index} + 1")
			list(REMOVE_AT srcfiles ${__INDEX})
		endif()
	endforeach()

	set(${result} ${srcfiles} PARENT_SCOPE)
endfunction()

# The rest of this is pretty standard CMake search for a package

find_file(KITTIE_COMPOSE kittie-compose.py  HINTS "$ENV{KITTIE_DIR}/bin")

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
find_package_handle_standard_args(EFFIS REQUIRED_VARS KITTIE_INCLUDE_DIRS kittie_cpp kittie_fortran)


if (KITTIE_FOUND)
	if(NOT TARGET EFFIS::cpp)
		add_library(EFFIS::cpp INTERFACE IMPORTED)
		set_property(TARGET EFFIS::cpp PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${KITTIE_INCLUDE_DIRS}")
		set_property(TARGET EFFIS::cpp PROPERTY INTERFACE_LINK_LIBRARIES      "${kittie_cpp}")
    endif()
	if(NOT TARGET EFFIS::fortran)
		add_library(EFFIS::fortran INTERFACE IMPORTED)
		set_property(TARGET EFFIS::fortran PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${KITTIE_INCLUDE_DIRS}")
		set_property(TARGET EFFIS::fortran PROPERTY INTERFACE_LINK_LIBRARIES      "${kittie_fortran}")
    endif()
endif()
