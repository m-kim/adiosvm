#include <iostream>
#include <mpi.h>
#include <vector>
#include <adios2.h>
#include "gray-scott.h"

void print_io_settings(const adios2::IO &io)
{
    std::cout << "Simulation writes data using engine type:              " << io.EngineType() << std::endl;
}

void print_settings(const Settings &s)
{
    std::cout << "grid:             " << s.L << "x" << s.L << "x" << s.L
              << std::endl;
    std::cout << "steps:            " << s.steps << std::endl;
    std::cout << "plotgap:          " << s.plotgap << std::endl;
    std::cout << "F:                " << s.F << std::endl;
    std::cout << "k:                " << s.k << std::endl;
    std::cout << "dt:               " << s.dt << std::endl;
    std::cout << "Du:               " << s.Du << std::endl;
    std::cout << "Dv:               " << s.Dv << std::endl;
    std::cout << "noise:            " << s.noise << std::endl;
    std::cout << "output:           " << s.output << std::endl;
    std::cout << "adios_config:     " << s.adios_config << std::endl;
}

void print_simulator_settings(const GrayScott &s)
{
    std::cout << "decomposition:    " << s.npx << "x" << s.npy << "x" << s.npz
              << std::endl;
    std::cout << "grid per process: " << s.size_x << "x" << s.size_y << "x"
              << s.size_z << std::endl;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int rank, procs, wrank;

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    const unsigned int color = 1;
    MPI_Comm comm;
    MPI_Comm_split(MPI_COMM_WORLD, color, wrank, &comm);

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &procs);

    if (argc < 2) {
        if (rank == 0) {
            std::cerr << "Too few arguments" << std::endl;
            std::cerr << "Usage: grayscott settings.json" << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    Settings settings = Settings::from_json(argv[1]);

    if (settings.L % procs != 0) {
        if (rank == 0) {
            std::cerr << "L must be divisible by the number of processes"
                      << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    GrayScott sim(settings, comm);
    sim.init();


	//@effis-init xml="adios2.xml", comm=comm
    adios2::ADIOS adios(settings.adios_config, comm, adios2::DebugON);


	//@effis-begin "SimulationOutput"->"ConcentrationData"
    adios2::IO io = adios.DeclareIO("SimulationOutput");

    if (rank == 0) {
        print_io_settings(io);
        std::cout << "========================================" << std::endl;
        print_settings(settings);
        print_simulator_settings(sim);
        std::cout << "========================================" << std::endl;
    }

    io.DefineAttribute<double>("F", settings.F);
    io.DefineAttribute<double>("k", settings.k);
    io.DefineAttribute<double>("dt", settings.dt);
    io.DefineAttribute<double>("Du", settings.Du);
    io.DefineAttribute<double>("Dv", settings.Dv);
    io.DefineAttribute<double>("noise", settings.noise);

    adios2::Variable<double> varU = io.DefineVariable<double>(
        "U", {sim.npz * sim.size_z, sim.npy * sim.size_y, sim.npx * sim.size_x},
        {sim.pz * sim.size_z, sim.py * sim.size_y, sim.px * sim.size_x},
        {sim.size_z, sim.size_y, sim.size_x});

    adios2::Variable<double> varV = io.DefineVariable<double>(
        "V", {sim.npz * sim.size_z, sim.npy * sim.size_y, sim.npx * sim.size_x},
        {sim.pz * sim.size_z, sim.py * sim.size_y, sim.px * sim.size_x},
        {sim.size_z, sim.size_y, sim.size_x});

    adios2::Variable<int> varStep = io.DefineVariable<int>("step");

    adios2::Engine writer = io.Open(settings.output, adios2::Mode::Write);

    for (int i = 0; i < settings.steps; i++) {
		//@effis-timestep physical=i*s.dt, number=i
		
        sim.iterate();

        if (i % settings.plotgap == 0) {
            if (rank == 0) {
                std::cout << "Simulation at step " << i 
                          << " writing output step     " << i/settings.plotgap 
                          << std::endl;
            }
            std::vector<double> u = sim.u_noghost();
            std::vector<double> v = sim.v_noghost();

            writer.BeginStep();
            writer.Put<int>(varStep, &i);
            writer.Put<double>(varU, u.data());
            writer.Put<double>(varV, v.data());
            writer.EndStep();
        }
    }

    writer.Close();
	//@effis-end
	
	//@effis-finalize
    MPI_Finalize();
}
