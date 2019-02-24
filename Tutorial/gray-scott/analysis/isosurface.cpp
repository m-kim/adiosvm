/*
 * Analysis code for the Gray-Scott simulation.
 * Reads variable U and and extracts the iso-surface using VTK.
 * Writes the extracted iso-surface using ADIOS or VTK.
 *
 * Keichi Takahashi <takahashi.keichi@ais.cmc.osaka-u.ac.jp>
 *
 */

#include <chrono>
#include <iostream>
#include <sstream>

#include <adios2.h>

#include <vtkImageData.h>
#include <vtkImageImport.h>
#include <vtkMarchingCubes.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataWriter.h>

vtkSmartPointer<vtkPolyData>
compute_isosurface(const adios2::Variable<double> &varField,
                   const std::vector<double> &field, double isovalue)
{
    // Convert vector of field values to vtkImageData
    auto importer = vtkSmartPointer<vtkImageImport>::New();
    importer->SetDataSpacing(1, 1, 1);
    importer->SetDataOrigin(varField.Start()[2], varField.Start()[1],
                            varField.Start()[0]);
    importer->SetWholeExtent(0, varField.Count()[2] - 1, 0,
                             varField.Count()[1] - 1, 0,
                             varField.Count()[0] - 1);
    importer->SetDataExtentToWholeExtent();
    importer->SetDataScalarTypeToDouble();
    importer->SetNumberOfScalarComponents(1);
    importer->SetImportVoidPointer(const_cast<double *>(field.data()));

    // Run the marching cubes algorithm
    auto mcubes = vtkSmartPointer<vtkMarchingCubes>::New();
    mcubes->SetInputConnection(importer->GetOutputPort());
    mcubes->ComputeNormalsOn();
    mcubes->SetValue(0, isovalue);
    mcubes->Update();

    // Return the isosurface as vtkPolyData
    return mcubes->GetOutput();
}

void write_vtk(const std::string &fname,
               const vtkSmartPointer<vtkPolyData> polyData)
{
    auto writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
    writer->SetFileName(fname.c_str());
    writer->SetInputData(polyData);
    writer->Write();
}

void write_adios(adios2::Engine &writer,
                 const vtkSmartPointer<vtkPolyData> polyData,
                 adios2::Variable<double> &varPoint,
                 adios2::Variable<int> &varCell,
                 adios2::Variable<int> &varOutStep, int step, MPI_Comm comm)
{
    int numCells = polyData->GetNumberOfPolys();
    int numPoints = polyData->GetNumberOfPoints();

    std::cout << numCells << " cells " << numPoints << " points" << std::endl;

    std::vector<double> points(numPoints * 3);
    std::vector<int> cells(numCells * 3); // Assumes that cells are triangles

    double coords[3];

    vtkSmartPointer<vtkCellArray> cellArray = polyData->GetPolys();

    cellArray->InitTraversal();

    for (int i = 0; i < polyData->GetNumberOfPolys(); i++) {
        auto idList = vtkSmartPointer<vtkIdList>::New();

        cellArray->GetNextCell(idList);

        for (int j = 0; j < idList->GetNumberOfIds(); j++) {
            auto id = idList->GetId(j);

            cells[i * 3 + j] = id;

            polyData->GetPoint(id, coords);

            points[id * 3 + 0] = coords[0];
            points[id * 3 + 1] = coords[1];
            points[id * 3 + 2] = coords[2];
        }
    }

    int totalPoints, offsetPoints;
    MPI_Allreduce(&numPoints, &totalPoints, 1, MPI_INT, MPI_SUM, comm);
    MPI_Scan(&numPoints, &offsetPoints, 1, MPI_INT, MPI_SUM, comm);

    writer.BeginStep();

    varPoint.SetShape({static_cast<size_t>(totalPoints), 3});
    varPoint.SetSelection({{static_cast<size_t>(offsetPoints - numPoints), 0},
                           {static_cast<size_t>(numPoints), 3}});

    if (numPoints) {
        writer.Put(varPoint, points.data());
    }

    int totalCells, offsetCells;
    MPI_Allreduce(&numCells, &totalCells, 1, MPI_INT, MPI_SUM, comm);
    MPI_Scan(&numCells, &offsetCells, 1, MPI_INT, MPI_SUM, comm);

    for (int i = 0; i < cells.size(); i++) {
        cells[i] += (offsetPoints - numPoints);
    }

    varCell.SetShape({static_cast<size_t>(totalCells), 3});
    varCell.SetSelection({{static_cast<size_t>(offsetCells - numCells), 0},
                          {static_cast<size_t>(numCells), 3}});

    if (numCells) {
        writer.Put(varCell, cells.data());
    }

    writer.Put(varOutStep, step);

    writer.EndStep();
}

std::chrono::milliseconds
diff(const std::chrono::steady_clock::time_point &start,
     const std::chrono::steady_clock::time_point &end)
{
    auto diff = end - start;

    return std::chrono::duration_cast<std::chrono::milliseconds>(diff);
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int rank, procs, wrank;

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    const unsigned int color = 5;
    MPI_Comm comm;
    MPI_Comm_split(MPI_COMM_WORLD, color, wrank, &comm);

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &procs);

    int dims[3] = {0};
    MPI_Dims_create(procs, 3, dims);
    size_t npx = dims[0];
    size_t npy = dims[1];
    size_t npz = dims[2];

    int coords[3] = {0};
    int periods[3] = {0};
    MPI_Comm cart_comm;
    MPI_Cart_create(comm, 3, dims, periods, 0, &cart_comm);
    MPI_Cart_coords(cart_comm, rank, 3, coords);
    size_t px = coords[0];
    size_t py = coords[1];
    size_t pz = coords[2];

    if (argc < 4) {
        std::cerr << "Too few arguments" << std::endl;
        std::cout << "Usage: isosurface input output isovalue" << std::endl;
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    const std::string input_fname(argv[1]);
    const std::string output_fname(argv[2]);
    const double isovalue = std::stod(argv[3]);

    adios2::ADIOS adios("adios2.xml", comm, adios2::DebugON);

    adios2::IO inIO = adios.DeclareIO("SimulationOutput");
    adios2::Engine reader = inIO.Open(input_fname, adios2::Mode::Read);

    adios2::IO outIO = adios.DeclareIO("IsosurfaceOutput");
    adios2::Engine writer = outIO.Open(output_fname, adios2::Mode::Write);

    auto varPoint =
        outIO.DefineVariable<double>("point", {1, 3}, {0, 0}, {1, 3});
    auto varCell = outIO.DefineVariable<int>("cell", {1, 3}, {0, 0}, {1, 3});
    auto varOutStep = outIO.DefineVariable<int>("step");

    std::vector<double> u;
    int step;

    auto start_total = std::chrono::steady_clock::now();

    while (true) {
        auto start_step = std::chrono::steady_clock::now();

        adios2::StepStatus status =
            reader.BeginStep(adios2::StepMode::NextAvailable);

        if (status != adios2::StepStatus::OK) {
            break;
        }

        adios2::Variable<double> varU = inIO.InquireVariable<double>("U");
        const adios2::Variable<int> varStep = inIO.InquireVariable<int>("step");

        adios2::Dims shape = varU.Shape();

        size_t size_x = shape[0] / npx;
        size_t size_y = shape[1] / npy;
        size_t size_z = shape[2] / npz;

        varU.SetSelection({{px * size_x, py * size_y, pz * size_z},
                           {size_x + (px != npx - 1 ? 1 : 0),
                            size_y + (py != npy - 1 ? 1 : 0),
                            size_z + (pz != npz - 1 ? 1 : 0)}});

        reader.Get<double>(varU, u);
        reader.Get<int>(varStep, step);
        reader.EndStep();

        auto end_read = std::chrono::steady_clock::now();

        auto polyData = compute_isosurface(varU, u, isovalue);

        auto end_compute = std::chrono::steady_clock::now();

        std::stringstream ss;

        ss << "iso-" << rank << "-" << step << ".vtp";

        write_adios(writer, polyData, varPoint, varCell, varOutStep, step,
                    comm);
        write_vtk(ss.str(), polyData);

        auto end_step = std::chrono::steady_clock::now();

        std::cout << "Step " << step << " read IO "
                  << diff(start_step, end_read).count() << " [ms]"
                  << " compute " << diff(end_read, end_compute).count()
                  << " [ms]"
                  << " write IO " << diff(end_compute, end_step).count()
                  << " [ms]" << std::endl;
    }

    auto end_total = std::chrono::steady_clock::now();

    std::cout << "Total runtime: " << diff(start_total, end_total).count()
              << " [ms]" << std::endl;

    writer.Close();
    reader.Close();

    MPI_Finalize();
}
