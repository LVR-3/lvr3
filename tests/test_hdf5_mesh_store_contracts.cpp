#include "lvr2/io/MeshStores.hpp"
#include "lvr2/types/CustomChannelTypes.hpp"
#include "lvr2/types/MeshBuffer.hpp"

#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

int main()
{
    namespace fs = std::filesystem;

    const auto path = fs::temp_directory_path() / "lvr2_hdf5_mesh_store_contracts.h5";
    fs::remove(path);

    try
    {
        auto mesh = std::make_shared<lvr2::MeshBuffer>();
        auto waveform = std::shared_ptr<lvr2::WaveformData[]>(new lvr2::WaveformData[1]);
        waveform[0].samples = {1, 2, 3};
        waveform[0].echo_type = 7;
        waveform[0].low_power = false;
        mesh->addChannel<lvr2::WaveformData>(waveform, "waveform", 1, 1);

        lvr2::io::mesh::Hdf5MeshStore store;
        store.open(path.string());

        try
        {
            store.save_mesh("mesh", mesh);
            std::cerr << "expected unsupported WaveformData channel save to fail\n";
            fs::remove(path);
            return 1;
        }
        catch (const std::runtime_error& e)
        {
            const std::string message = e.what();
            if (message.find("does not support channel type") == std::string::npos)
            {
                std::cerr << "unexpected unsupported-channel diagnostic: " << message << '\n';
                fs::remove(path);
                return 2;
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "unexpected HDF5 mesh store contract failure: " << e.what() << '\n';
        fs::remove(path);
        return 3;
    }

    fs::remove(path);
    return 0;
}
