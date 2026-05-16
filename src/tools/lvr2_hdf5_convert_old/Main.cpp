#include "Logging.hpp"

#include "Options.hpp"

#include "lvr2/io/scan.hpp"

#include "lvr2/util/Hdf5Util.hpp"
#include <boost/filesystem.hpp>

#include "lvr2/util/Synthetic.hpp"

#include <boost/type_index.hpp>

#include <unordered_map>
#include <unordered_set>

#include <boost/iostreams/code_converter.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

#include "Hdf5ReaderOld.hpp"
#include "ScanTypesCompare.hpp"

#include <random>
#include <chrono>

using namespace lvr2;

int main(int argc, char** argv)
{
    if(argc > 1)
    {
        std::string infilename = argv[1];
        std::cout << "Load file from '" << infilename << "' with old Hdf5 format." << std::endl;
        auto sp = loadOldHDF5(infilename);
        if (!sp)
        {
            std::cout << "Unable to load old HDF5 scan project." << std::endl;
            return 1;
        }

        std::cout << "Construct new ProjectStore HDF5 output." << std::endl;

        std::string outfilename = "scan_project.h5";

        std::cout << "Write to '" << outfilename << "' with ProjectStore HDF5 format." << std::endl;
        auto saved = lvr2::io::scan::save_project(
            outfilename,
            *sp,
            lvr2::io::scan::SaveOptions::hdf5());
        if (!saved)
        {
            std::cout << "Unable to write scan project: " << saved.error().message << std::endl;
            return 1;
        }
    } else {
        std::cout << "please specify Hdf5 file that can be load with the old feature based Hdf5IO" << std::endl;
    }

    return 0;
}
