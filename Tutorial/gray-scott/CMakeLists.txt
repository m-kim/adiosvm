cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
project(gray-scott C CXX)

set(libs adios2::adios2 MPI::MPI_C)
find_package(MPI REQUIRED)
find_package(ADIOS2 REQUIRED)


# Use Kittie or not?
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)
set(USE_KITTIE OFF CACHE STRING "Use MPI")
set_property(CACHE USE_KITTIE PROPERTY STRINGS "ON;TRUE;OFF;FALSE")

if(USE_KITTIE)
	find_package(KITTIE REQUIRED)
	add_definitions(-DKITTIE -DUSE_MPI -std=c++11)
	include_directories(${KITTIE_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/simulation)
	set(gs   ${CMAKE_CURRENT_BINARY_DIR}/simulation/main-kittie.cpp)
	set(pdf  ${CMAKE_CURRENT_BINARY_DIR}/analysis/pdf_calc-kittie.cpp)
	set(libs ${libs} ${kittie_cpp})
	add_custom_command(
		OUTPUT  ${gs} ${pdf}
		DEPENDS simulation/main.cpp analysis/pdf_calc.cpp
		COMMAND kittie-cpp.py repo ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} --suffix="-kittie" --name=grayscott --mimic --new
	)
else(USE_KITTIE)
	set(gs  simulation/main.cpp)
	set(pdf analysis/pdf_calc.cpp)
endif(USE_KITTIE)


# We are not using the C++ API of MPI, this will stop the compiler look for it
add_definitions(-DOMPI_SKIP_MPICXX -DMPICH_SKIP_MPICXX)   

add_executable(gray-scott ${gs} simulation/gray-scott.cpp simulation/settings.cpp)
target_link_libraries(gray-scott ${libs})
add_executable(pdf_calc ${pdf})
add_dependencies(pdf_calc gray-scott)
target_link_libraries(pdf_calc ${libs})

if(USE_KITTIE)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/.kitite-setup.yaml DESTINATION ${CMAKE_INSTALL_PREFIX}/)
endif(USE_KITTIE)
