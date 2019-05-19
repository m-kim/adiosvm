function(kpcmake src dest suffix)	
	execute_process(COMMAND kittie-cpp.py repo ${src} --tree-output=${dest} --confdir=${dest} --suffix=${suffix})
endfunction()

function(listreplace srcfiles effisfiles suffix result)
	set(srcfix "")
	foreach(src ${srcfiles})
		get_filename_component(dir  ${src} DIRECTORY)
		get_filename_component(base ${src} NAME_WE)
		get_filename_component(ext  ${src} EXT)
		list(APPEND srcfix "${dir}/${base}${suffix}${ext}")
	endforeach()

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
find_package_handle_standard_args(Kittie REQUIRED_VARS KITTIE_INCLUDE_DIRS kittie_cpp kittie_fortran)


if (KITTIE_FOUND)
	if(NOT TARGET KITTIE::cpp)
		add_library(KITTIE::cpp INTERFACE IMPORTED)
		set_property(TARGET KITTIE::cpp PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${KITTIE_INCLUDE_DIRS}")
		set_property(TARGET KITTIE::cpp PROPERTY INTERFACE_LINK_LIBRARIES      "${kittie_cpp}")
    endif()
	if(NOT TARGET KITTIE::fortran)
		add_library(KITTIE::fortran INTERFACE IMPORTED)
		set_property(TARGET KITTIE::fortran PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${KITTIE_INCLUDE_DIRS}")
		set_property(TARGET KITTIE::fortran PROPERTY INTERFACE_LINK_LIBRARIES      "${kittie_fortran}")
    endif()
endif()
