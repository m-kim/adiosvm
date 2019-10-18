cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)
# ENABLE ADIOS2_ROOT
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

project(gray-scott C CXX)

find_package(MPI REQUIRED)
find_package(ADIOS2 REQUIRED)

set(sim_files
  simulation/main.cpp
  simulation/gray-scott.cpp
  simulation/settings.cpp
  simulation/writer.cpp
)

set(analysis_files analysis/pdf_calc.cpp)

option(USE_TIMERS "Use profiling timers")
if(USE_TIMERS)
  message(STATUS "Enabling profiling timers")
  add_definitions(-DENABLE_TIMERS)
endif()

option(EFFIS "Use EFFIS" OFF)
if (EFFIS)
    list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR})
    find_package(EFFIS REQUIRED)
    set(effis_lib EFFIS::cpp)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/common DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/simulation DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    kpcmake("${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}" -kittie)
    file(GLOB_RECURSE effis_sim_files RELATIVE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/simulation/*-kittie.cpp)
    file(GLOB_RECURSE effis_analysis_files RELATIVE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/analysis/*-kittie.cpp)
    listreplace("${sim_files}" "${effis_sim_files}" "-kittie" sim_files)
    listreplace("${analysis_files}" "${effis_analysis_files}" "-kittie" analysis_files)
endif()

# We are not using the C++ API of MPI, this will stop the compiler look for it
add_definitions(-DOMPI_SKIP_MPICXX -DMPICH_SKIP_MPICXX)
add_executable(gray-scott ${sim_files})
target_link_libraries(gray-scott adios2::adios2 MPI::MPI_C ${effis_lib})

add_executable(pdf_calc ${analysis_files})
target_link_libraries(pdf_calc adios2::adios2 MPI::MPI_C ${effis_lib})

option(VTK "Build VTK apps")
if (VTK_ROOT)
  set(VTK ON)
endif(VTK_ROOT)

if (VTK)
  message(STATUS "Configuring VTK apps")

  find_package(VTK COMPONENTS
    vtkFiltersCore
    vtkIOImage
    vtkIOXML
  )

  if(VTK_FOUND)
    add_executable(isosurface analysis/isosurface.cpp)
    target_link_libraries(isosurface adios2::adios2 ${VTK_LIBRARIES}
      MPI::MPI_C)
  endif(VTK_FOUND)

  find_package(VTK COMPONENTS
    vtkFiltersCore
    vtkFiltersGeometry
  )

  if(VTK_FOUND)
    add_executable(find_blobs analysis/find_blobs.cpp)
    target_link_libraries(find_blobs adios2::adios2 ${VTK_LIBRARIES}
      MPI::MPI_C)
  endif(VTK_FOUND)

  find_package(VTK COMPONENTS
    vtkFiltersGeneral
  )

  if(VTK_FOUND)
    add_executable(compute_curvature analysis/curvature.cpp)
    target_link_libraries(compute_curvature adios2::adios2 ${VTK_LIBRARIES}
      MPI::MPI_C)
  endif(VTK_FOUND)


  find_package(VTK COMPONENTS
    vtkRenderingOpenGL2
    vtkViewsInfovis
  )

  if(VTK_FOUND)
    add_executable(render_isosurface plot/render_isosurface.cpp)
    target_link_libraries(render_isosurface adios2::adios2 ${VTK_LIBRARIES}
      MPI::MPI_C)
  endif(VTK_FOUND)
endif(VTK)
