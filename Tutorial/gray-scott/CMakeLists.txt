function(preproc subdir srcfiles outfiles)
	set(suffix -kittie)
	set(pattern *${suffix}.cpp)
	kpcmake("${CMAKE_CURRENT_SOURCE_DIR}/${subdir}" "${CMAKE_CURRENT_BINARY_DIR}/${subdir}" ${suffix})
	file(GLOB_RECURSE newfiles RELATIVE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${subdir}/${pattern})
	listreplace("${srcfiles}" "${newfiles}" "${suffix}" srcfiles)
	set(${outfiles} ${srcfiles} PARENT_SCOPE)
endfunction()


cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
project(gray-scott C CXX)

set(libs adios2::adios2 MPI::MPI_C)
find_package(MPI    REQUIRED)
find_package(ADIOS2 REQUIRED)

set(gray_scott_src simulation/main.cpp simulation/gray-scott.cpp simulation/settings.cpp)
set(pdf_src analysis/pdf_calc.cpp)


# Use Kittie or not?

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)
option(USE_KITTIE "Use KITTIE" OFF)

if(USE_KITTIE)
	find_package(Kittie REQUIRED)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/simulation)
	preproc("simulation" "${gray_scott_src}" gray_scott_src)
	preproc("analysis"   "${pdf_src}"        pdf_src)
endif()


# We are not using the C++ API of MPI, this will stop the compiler look for it
add_definitions(-DOMPI_SKIP_MPICXX -DMPICH_SKIP_MPICXX)   

add_executable(gray-scott ${gray_scott_src})
add_executable(pdf_calc   ${pdf_src})
target_link_libraries(gray-scott ${libs})
target_link_libraries(pdf_calc   ${libs})


if(USE_KITTIE)
	target_link_libraries(gray-scott KITTIE::cpp)
	target_link_libraries(pdf_calc   KITTIE::cpp)
endif()
