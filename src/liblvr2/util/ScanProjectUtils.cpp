#include "lvr2/util/ScanProjectUtils.hpp"
#include "lvr2/types/ScanTypes.hpp"
#include "lvr2/util/Factories.hpp"
#include "lvr2/util/ScanSchemaUtils.hpp"
#include "lvr2/util/TransformUtils.hpp"
#include "lvr2/util/Logging.hpp"
#include "lvr2/io/ModelFactory.hpp"
#include "lvr2/io/scan.hpp"

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>

#include <algorithm>
#include <atomic>
#include <cctype>

namespace lvr2
{

namespace
{

std::string upperSchemaName(std::string schema)
{
    std::transform(schema.begin(), schema.end(), schema.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return schema;
}

bool isHdf5Path(const boost::filesystem::path& path)
{
    const std::string extension = path.extension().string();
    return extension == ".h5" || extension == ".hdf5";
}

lvr2::io::storage::Error unsupportedDirectorySchemaError(const std::string& schema)
{
    return {lvr2::io::storage::ErrorCode::Unsupported,
            "ProjectStore directory scan-project storage supports RAWPLY point data and RAW metadata-only layouts; schema '" + schema + "' is not supported by the new service path"};
}

lvr2::io::storage::Error unsupportedHdf5SchemaError(const std::string& schema)
{
    return {lvr2::io::storage::ErrorCode::Unsupported,
            "ProjectStore HDF5 scan-project storage supports the HDF5 schema; schema '" + schema + "' is not supported by the new service path"};
}

lvr2::io::storage::Result<lvr2::io::scan::Schema> directoryProjectStoreSchema(const std::string& schema)
{
    const std::string name = upperSchemaName(schema);
    if (name.empty() || name == "RAWPLY")
    {
        return lvr2::io::scan::Schema::raw_ply();
    }
    if (name == "RAW")
    {
        return lvr2::io::scan::Schema::raw();
    }
    return lvr2::io::storage::unexpected(unsupportedDirectorySchemaError(schema));
}

lvr2::io::storage::Result<lvr2::io::scan::Schema> hdf5ProjectStoreSchema(const std::string& schema)
{
    const std::string name = upperSchemaName(schema);
    if (name.empty() || name == "HDF5")
    {
        return lvr2::io::scan::Schema::hdf5();
    }
    return lvr2::io::storage::unexpected(unsupportedHdf5SchemaError(schema));
}

lvr2::io::storage::Result<lvr2::io::scan::LoadOptions> loadOptionsForScanProject(
    const std::string& schema,
    const boost::filesystem::path& sourcePath,
    bool loadData)
{
    lvr2::io::scan::LoadOptions options;
    options.loadMode = loadData
        ? lvr2::io::storage::LoadMode::Eager
        : lvr2::io::storage::LoadMode::Lazy;

    if (isHdf5Path(sourcePath))
    {
        auto hdf5Schema = hdf5ProjectStoreSchema(schema);
        if (!hdf5Schema)
        {
            return lvr2::io::storage::unexpected(hdf5Schema.error());
        }
        options.kind = lvr2::io::storage::StorageKind::hdf5();
        options.schema = hdf5Schema.value();
    }
    else
    {
        auto directorySchema = directoryProjectStoreSchema(schema);
        if (!directorySchema)
        {
            return lvr2::io::storage::unexpected(directorySchema.error());
        }
        options.kind = lvr2::io::storage::StorageKind::directory();
        options.schema = directorySchema.value();
    }
    return options;
}

lvr2::io::storage::Result<lvr2::io::scan::SaveOptions> saveOptionsForScanProject(
    const std::string& schema,
    const boost::filesystem::path& targetPath)
{
    lvr2::io::scan::SaveOptions options;
    if (isHdf5Path(targetPath))
    {
        auto hdf5Schema = hdf5ProjectStoreSchema(schema);
        if (!hdf5Schema)
        {
            return lvr2::io::storage::unexpected(hdf5Schema.error());
        }
        options.kind = lvr2::io::storage::StorageKind::hdf5();
        options.schema = hdf5Schema.value();
    }
    else
    {
        auto directorySchema = directoryProjectStoreSchema(schema);
        if (!directorySchema)
        {
            return lvr2::io::storage::unexpected(directorySchema.error());
        }
        options.kind = lvr2::io::storage::StorageKind::directory();
        options.schema = directorySchema.value();
    }
    return options;
}

void logStorageError(const std::string& action, const lvr2::io::storage::Error& error)
{
        lvr2::log::error("{}{}{}", fmt::streamed(action), fmt::streamed(": "), fmt::streamed(error.message));
}

} // namespace

std::pair<ScanPtr, Transformd> scanFromProject(ScanProjectPtr project, size_t scanPositionNo, size_t lidarNo, size_t scanNo)
{
    Transformd transform = Transformd::Identity();

    if(project && scanPositionNo < project->positions.size())
    {
        transform = transform * project->transformation;
        ScanPositionPtr pos = project->positions[scanPositionNo];
        if(pos && lidarNo < pos->lidars.size())
        {
            transform = transform * pos->transformation;
            LIDARPtr lidar =  pos->lidars[lidarNo];
            if(lidar && scanNo < lidar->scans.size())
            {
                transform = transform * lidar->transformation;
                ScanPtr scan = lidar->scans[scanNo];
                return std::make_pair(scan, transform);
            }
        }
    }

    // Something went wrong during access...
    return std::make_pair(nullptr, Transformd::Identity());
}

ScanProjectPtr scanProjectFromHDF5(std::string file, const std::string& schemaName)
{
    auto options = loadOptionsForScanProject(schemaName, boost::filesystem::path(file), false);
    if (!options)
    {
        logStorageError("[Load Scan Project from HDF5] Unsupported scan-project options", options.error());
        return nullptr;
    }

    auto loaded = lvr2::io::scan::load_project(file, options.value());
    if (!loaded)
    {
        logStorageError("[Load Scan Project from HDF5] Unable to load scan project", loaded.error());
        return nullptr;
    }
    return loaded.value();
}

ScanProjectPtr scanProjectFromFile(const std::string& file)
{
        lvr2::log::info("{}{}", fmt::streamed("[Load Scan Project from File] Creating scan project from single file: "), fmt::streamed(file));
    ScanProjectPtr project(new ScanProject);
    ModelPtr model = ModelFactory::readModel(file);

    if(model)
    {

        // Create new scan object and mark scan data as
        // loaded
        ScanPtr scan(new Scan);
        scan->points = model->m_pointCloud;

        // Create new lidar object
        LIDARPtr lidar(new LIDAR);

        // Create new scan position
        ScanPositionPtr scanPosPtr = ScanPositionPtr(new ScanPosition());

        // Buildup scan project structure
        project->positions.push_back(scanPosPtr);
        project->positions[0]->lidars.push_back(lidar);
        project->positions[0]->lidars[0]->scans.push_back(scan);

        return project;
    }
    else
    {
                lvr2::log::error("{}{}{}", fmt::streamed("[Load Scan Project from file] Unable to open file '"), fmt::streamed(file), fmt::streamed("' for reading-"));
    }
    return nullptr;
}

ScanProjectPtr scanProjectFromPLYFiles(const std::string &dir)
{
        lvr2::log::info("{}", fmt::streamed("[Load Scan Project from PLY] Creating scan project from a directory of .ply files..."));
    ScanProjectPtr scanProject(new ScanProject);
    boost::filesystem::directory_iterator it{dir};
    while (it != boost::filesystem::directory_iterator{})
    {
        string ext = it->path().extension().string();
        if (ext == ".ply")
        {
            ModelPtr model = ModelFactory::readModel(it->path().string());

            // Create new Scan
            ScanPtr scan(new Scan);
            scan->points = model->m_pointCloud;

            // Wrap scan into lidar object
            LIDARPtr lidar(new LIDAR);
            lidar->scans.push_back(scan);

            // Put lidar into new scan position
            ScanPositionPtr position(new ScanPosition);
            position->lidars.push_back(lidar);

            // Add new scan position to scan project
            scanProject->positions.push_back(position);
        }
        it++;
    }
    if(scanProject->positions.size())
    {
        return scanProject;
    }
    else
    {
                lvr2::log::warning("{}", fmt::streamed("[Load Scan Project from PLY] Warning: scan project is empty."));
        return nullptr;
    }
}

ScanProjectPtr loadScanProject(const std::string& schema, const std::string& source, bool loadData)
{
    boost::filesystem::path sourcePath(source);
    if (!boost::filesystem::is_directory(sourcePath) && !isHdf5Path(sourcePath))
    {
                lvr2::log::error("{}{}", fmt::streamed("[Load Scan Project] Source is neither a directory nor an HDF5 file: "), fmt::streamed(source));
        return nullptr;
    }

    auto options = loadOptionsForScanProject(schema, sourcePath, loadData);
    if (!options)
    {
        logStorageError("[Load Scan Project] Unsupported scan-project options", options.error());
                lvr2::log::error("{}{}", fmt::streamed("[Load Scan Project] Schema name: "), fmt::streamed(schema));
                lvr2::log::error("{}{}", fmt::streamed("[Load Scan Project] Source: "), fmt::streamed(source));
        return nullptr;
    }

    auto loaded = lvr2::io::scan::load_project(source, options.value());
    if (!loaded)
    {
        logStorageError("[Load Scan Project] Unable to load scan project", loaded.error());
                lvr2::log::error("{}{}", fmt::streamed("[Load Scan Project] Schema name: "), fmt::streamed(schema));
                lvr2::log::error("{}{}", fmt::streamed("[Load Scan Project] Source: "), fmt::streamed(source));
        return nullptr;
    }

    return loaded.value();
}

ScanProjectPtr getSubProject(ScanProjectPtr project, std::vector<size_t> positions)
{
    ScanProjectPtr tmp = std::make_shared<ScanProject>();

    // Copy meta data
    tmp->boundingBox = project->boundingBox;
    tmp->crs = project->crs;
    tmp->name = project->name;
    tmp->transformation = project->transformation;
    tmp->unit = project->unit;

    // Copy only selected scan position into new project
    for(size_t i : positions)
    {
        if(i < project->positions.size())
        {
            tmp->positions.push_back(project->positions[i]);
        }
        else
        {
                        lvr2::log::warning("{}{}{}{}", fmt::streamed("[GetSubProject] Warning: Index"), fmt::streamed(i), fmt::streamed(" out of range, size is "), fmt::streamed(project->positions.size()));
        }
    }

    // Correct bounding box
        lvr2::log::debug("{}", fmt::streamed("[GetSubProject] Correcting bounding box"));

    BoundingBox<BaseVector<float>> bb;
    for(ScanPositionPtr p : tmp->positions)
    {
        if(p->boundingBox)
        {
            bb.expand(*(p->boundingBox));
        }
    }

        lvr2::log::info("{}{}", fmt::streamed("[GetSubProject] New bounding box is: "), fmt::streamed(bb));

    return tmp;
}

void saveScanProject(
    ScanProjectPtr& project,
    const std::vector<size_t>& positions,
    const std::string& schema,
    const std::string& target)
{
    // Create tmp scan project containing only the given positions
    ScanProjectPtr tmp = getSubProject(project, positions);

    // Save scan project to destination
    saveScanProject(tmp, schema, target);
}

void saveScanProject(ScanProjectPtr& project, const std::string& schema, const std::string& target)
{
    if(project)
    {
        boost::filesystem::path targetPath(target);
        auto options = saveOptionsForScanProject(schema, targetPath);
        if (!options)
        {
            logStorageError("[Save Scan Project] Unsupported scan-project options", options.error());
                        lvr2::log::error("{}{}", fmt::streamed("[Save Scan Project] Schema name: "), fmt::streamed(schema));
                        lvr2::log::error("{}{}", fmt::streamed("[Save Scan Project] Target: "), fmt::streamed(target));
            return;
        }

        auto saved = lvr2::io::scan::save_project(target, *project, options.value());
        if (!saved)
        {
            logStorageError("[Save Scan Project] Unable to save scan project", saved.error());
                        lvr2::log::error("{}{}", fmt::streamed("[Save Scan Project] Schema name: "), fmt::streamed(schema));
                        lvr2::log::error("{}{}", fmt::streamed("[Save Scan Project] Target: "), fmt::streamed(target));
        }
    }
    else
    {
                lvr2::log::error("{}", fmt::streamed("[Save Scan Project] Cannot save scan project from null pointer"));
    }
}

void printScanProjectStructure(const ScanProjectPtr project)
{
        lvr2::log::info("{}", fmt::streamed(project));

    for(size_t i = 0; i < project->positions.size(); i++)
    {
                lvr2::log::info("{}{}{}{}", fmt::streamed("[Scan Project] Position"), fmt::streamed(i), fmt::streamed(" / "), fmt::streamed(project->positions.size()));
        printScanPositionStructure(project->positions[i]);
    }
}

void printScanPositionStructure(const ScanPositionPtr p)
{
        lvr2::log::info("{}", fmt::streamed(p));
    for(size_t i = 0; i < p->lidars.size(); i++)
    {
                lvr2::log::info("{}{}{}{}", fmt::streamed("[Scan Position] LiDAR "), fmt::streamed(i), fmt::streamed(" / "), fmt::streamed(p->lidars.size()));
        printLIDARStructure(p->lidars[i]);
    }
    for(size_t i = 0; i < p->cameras.size(); i++)
    {
                lvr2::log::info("{}{}{}{}", fmt::streamed("[Scan Position] Camera "), fmt::streamed(i), fmt::streamed(" / "), fmt::streamed(p->cameras.size()));
        printCameraStructure(p->cameras[i]);
    }
    for(size_t i = 0; i < p->hyperspectral_cameras.size(); i++)
    {
        printHyperspectralCameraStructure(p->hyperspectral_cameras[i]);
    }

}

void printScanStructure(const ScanPtr p)
{
        lvr2::log::info("{}", fmt::streamed(p));
    // TODO: Implement output for point buffer
}

void printLIDARStructure(const LIDARPtr p)
{
        lvr2::log::info("{}", fmt::streamed(p));
    for(size_t i = 0; i < p->scans.size(); i++)
    {
                lvr2::log::info("{}{}{}{}", fmt::streamed("[LiDAR] Scan "), fmt::streamed(i), fmt::streamed(" / "), fmt::streamed(p->scans.size()));
        printScanStructure(p->scans[i]);
    }
}

void printCameraStructure(const CameraPtr p)
{
        lvr2::log::info("{}", fmt::streamed(p));

    for(size_t i = 0; i < p->groups.size(); i++)
    {
                lvr2::log::info("{}{}{}{}", fmt::streamed("[Camera] Camera group "), fmt::streamed(i), fmt::streamed(" / "), fmt::streamed(p->groups.size()));
        CameraImageGroupPtr g = p->groups[i];
                lvr2::log::info("{}{}", fmt::streamed("[Camera] Transformation: "), fmt::streamed(g->transformation));
                lvr2::log::info("{}{}", fmt::streamed("[Camera] Type: "), fmt::streamed(g->type));
                lvr2::log::info("{}{}", fmt::streamed("[Camera] Number of images: "), fmt::streamed(g->images.size()));
    }
}

void printCameraImageGroupStructure(const CameraImageGroupPtr p)
{
        lvr2::log::info("{}", fmt::streamed(p));
    for(size_t i = 0; i < p->images.size(); i++)
    {
                lvr2::log::info("{}{}{}{}", fmt::streamed("[Image Group] Image "), fmt::streamed(i), fmt::streamed(" / "), fmt::streamed(p->images.size()));
                lvr2::log::info("{}", fmt::streamed(p->images[i]));
    }
}

void printHyperspectralCameraStructure(const HyperspectralCameraPtr p)
{
        lvr2::log::info("{}", fmt::streamed(p));
    for(size_t i = 0; i < p->panoramas.size(); i++)
    {
                lvr2::log::info("{}{}{}{}", fmt::streamed("[Hyperspectral Camera] Panorama "), fmt::streamed(i), fmt::streamed(" / "), fmt::streamed(p->panoramas.size()));
        printHyperspectralPanoramaStructure(p->panoramas[i]);
    }
}

void printHyperspectralPanoramaStructure(const HyperspectralPanoramaPtr p)
{
        lvr2::log::info("{}", fmt::streamed(p));
    for(size_t i = 0; i < p->channels.size(); i++)
    {
              lvr2::log::info("{}{}{}{}", fmt::streamed("[Panorama Structure] Channel "), fmt::streamed(i), fmt::streamed(" / "), fmt::streamed(p->channels.size()));
              lvr2::log::info("{}", fmt::streamed(p->channels[i]));
    }
}

void printCameraImageStructure(const CameraImagePtr p)
{
        lvr2::log::info("{}", fmt::streamed(p));
}

void estimateProjectNormals(ScanProjectPtr p, size_t kn, size_t ki)
{
    for(size_t positionNr = 0; positionNr < p->positions.size(); positionNr++)
    {
        ScanPositionPtr position = p->positions[positionNr];
        if(position)
        {
            for(size_t lidarNr = 0; lidarNr < position->lidars.size(); lidarNr++)
            {
                LIDARPtr lidar = position->lidars[lidarNr];
                if(lidar)
                {
                    for(size_t scanNr = 0; scanNr < lidar->scans.size(); scanNr++)
                    {
                        ScanPtr scan = lidar->scans[scanNr];
                        if(scan)
                        {
                                                        lvr2::log::info("{}{}{}{}{}{}", fmt::streamed("[Project Normal Estimation]: Loading scan "), fmt::streamed(scanNr), fmt::streamed(" from lidar "), fmt::streamed(lidarNr), fmt::streamed(" of scan position "), fmt::streamed(positionNr));

                            scan->load();
                            PointBufferPtr ptBuffer = scan->points;
                            if(ptBuffer)
                            {
                                size_t n = ptBuffer->numPoints();
                                if(n)
                                {
                                                                        lvr2::log::info("{}{}{}", fmt::streamed("[Project Normal Estimation]: Loaded "), fmt::streamed(n), fmt::streamed(" points"));
                                                                        lvr2::log::info("{}", fmt::streamed("[Project Normal Estimation]: Building search tree..."));

                                    AdaptiveKSearchSurfacePtr<BaseVector<float>> surface(new AdaptiveKSearchSurface<BaseVector<float>>(ptBuffer, "flann", kn, ki));
                                    surface->setFlipPoint(BaseVector<float>(0, 0, 0));
                                    surface->calculateSurfaceNormals();
                                    // surface->interpolateSurfaceNormals(); -> not required, already done in calculateSurfaceNormals

                                    // Save data back to original project
                                    scan->save();

                                    // Free payload data
                                    scan->release();
                                }
                                else
                                {
                                                                        lvr2::log::warning("{}", fmt::streamed("[Project Normal Estimation]: No points in scan"));
                                }

                            }
                            else
                            {
                                                                lvr2::log::warning("{}", fmt::streamed("[Project Normal Estimation]:Unable to load point cloud data."));
                            }
                        }
                        else
                        {
                                                        lvr2::log::warning("{}{}{}{}{}{}", fmt::streamed("[Project Normal Estimation]: "), fmt::streamed("Unable to load scan "), fmt::streamed(scanNr), fmt::streamed(" of "), fmt::streamed("lidar "), fmt::streamed(lidarNr));
                        }
                    }
                }
                else
                {
                                        lvr2::log::warning("{}{}{}{}", fmt::streamed("[Project Normal Estimation]: Unable to load lidar "), fmt::streamed(lidarNr), fmt::streamed(" of scan position "), fmt::streamed(positionNr));
                }
            }
        }
        else
        {
                        lvr2::log::warning("{}{}", fmt::streamed("[Project Normal Estimation]: Unable to load scan position "), fmt::streamed(positionNr));
        }
    }
}

ScanProjectPtr loadScanPositionsExplicitly(
    const std::string& schema,
    const std::string& root,
    const std::vector<size_t>& positions)
{
    boost::filesystem::path targetPath(root);
    if (!boost::filesystem::is_directory(targetPath) && !isHdf5Path(targetPath))
    {
                lvr2::log::error("{}{}", fmt::streamed("[Load Positions Explicitly] : Root is neither a directory nor an HDF5 file: "), fmt::streamed(root));
        return nullptr;
    }

    auto options = loadOptionsForScanProject(schema, targetPath, false);
    if (!options)
    {
        logStorageError("[Load Positions Explicitly] : Unsupported scan-project options", options.error());
        return nullptr;
    }

    auto opened = lvr2::io::scan::open_project(root, options.value());
    if (!opened)
    {
        logStorageError("[Load Positions Explicitly] : Could not open scan project", opened.error());
        return nullptr;
    }

    auto loaded = opened.value().load();
    if (!loaded)
    {
        logStorageError("[Load Positions Explicitly] : Could not load scan project", loaded.error());
        return nullptr;
    }

    ScanProjectPtr selected = loaded.value();
    selected->positions.clear();

    for (size_t i : positions)
    {
        ScanPositionPtr pos;
        auto loadedPosition = opened.value().load_position(i);
        if (loadedPosition)
        {
            pos = loadedPosition.value();
        }

        if (pos)
        {
                        lvr2::log::info("{}{}", fmt::streamed("[Load Positions Explicitly] : Loading scan position "), fmt::streamed(i));
            selected->positions.push_back(pos);
        }
        else
        {
                        lvr2::log::warning("{}{}{}", fmt::streamed("[Load Positions Explicitly] : Position with index "), fmt::streamed(i), fmt::streamed(" cannot be loaded."));
        }
    }

    return selected;
}

size_t countPointsInScanProject(ScanProjectPtr project, bool firstScanOnly)
{
    size_t n = 0;
     // Go through all scans
    for(size_t positionNo = 0; positionNo < project->positions.size(); positionNo++)
    {
        for(size_t lidarNo = 0; lidarNo < project->positions[positionNo]->lidars.size(); lidarNo++)
        {
            LIDARPtr lidar = project->positions[positionNo]->lidars[lidarNo];
            if(lidar->scans.size() > 0)
            {
                for(size_t scanNo = 0; scanNo < lidar->scans.size(); scanNo++)
                {
                    // Stop of more data is present
                    if (scanNo > 1 && firstScanOnly)
                    {
                        break;
                    }
                    n += lidar->scans[scanNo]->numPoints;
                }
            }
        }
    }
    return n;
}

void exportScanProjectToPLY(ScanProjectPtr project, const std::string plyFile, bool firstScanOnly, OctreeReductionAlgorithmPtr red)
{
    // Step 0: Check if output file is valid
    std::ofstream outfile;
    outfile.open(plyFile.c_str(), std::ios::binary);

    if(!outfile.good())
    {
                lvr2::log::warning("{}{}{}", fmt::streamed("[WriteScanProjectToPLY]: Unable to open file '"), fmt::streamed(plyFile), fmt::streamed("' for writing."));
    }


    size_t numPointsInProject = countPointsInScanProject(project, true);
    size_t scansWithNormals = 0;
    size_t scansWithColors = 0;
    size_t totalScans = 0;
    size_t w_color;

    // Create three memory mapped files for temporary
    // point cloud data
    boost::iostreams::mapped_file_params params;

    // Worst case estimated of required file size: we assume that
    // the buffers are not reduced and create files large enough
    // to hold all data
    params.new_file_size = numPointsInProject * 3 * sizeof(float);
    params.mode = std::ios_base::in | std::ios_base::out | std::ios_base::trunc;

    boost::iostreams::mapped_file pointFile;
    boost::iostreams::mapped_file colorFile;
    boost::iostreams::mapped_file normalFile;

    params.path = "points.tmp";
    pointFile.open(params);
    float* mmf_points = (float*)pointFile.data();

    params.path = "colors.tmp";
    colorFile.open(params);
    unsigned char* mmf_colors = (unsigned char*)colorFile.data();

    params.path = "normals.tmp";
    normalFile.open(params);
    float* mmf_normals = (float*)normalFile.data();

    // Reset (for robustness we count the points that were actually loaded
    numPointsInProject = 0;

    // Go through all scans
    for(size_t positionNo = 0; positionNo < project->positions.size(); positionNo++)
    {
        for(size_t lidarNo = 0; lidarNo < project->positions[positionNo]->lidars.size(); lidarNo++)
        {
            LIDARPtr lidar = project->positions[positionNo]->lidars[lidarNo];
            if(lidar->scans.size() > 0)
            {
                for(size_t scanNo = 0; scanNo < lidar->scans.size(); scanNo++)
                {
                    // Stop of more data is present
                    if (scanNo > 1 && firstScanOnly)
                    {
                        break;
                    }

                    // Get current scan
                    ScanPtr scan = lidar->scans[scanNo];

                    // Load payload data
                    if(red)
                    {
                                                lvr2::log::info("{}", fmt::streamed("[WriteScanProjectToPLY] Loading reduced points"));
                        scan->load(red);
                    }
                    else
                    {
                                                lvr2::log::info("{}", fmt::streamed("[WriteScanProjectToPLY] Loading all points"));
                        scan->load();
                    }

                    PointBufferPtr points = scan->points;

                    // Transform scan data
                    Transformd positionPose = project->positions[positionNo]->transformation;
                    Transformd lidarPose = project->positions[positionNo]->lidars[lidarNo]->transformation;
                    Transformd transformation = transformRegistration(positionPose, lidarPose);

                    transformPointBuffer(points, transformation);
                                        lvr2::log::info("{}", fmt::streamed("[WriteScanProjectToPLY] Writing tmp chunks..."));
                    if(points)
                    {
                        totalScans++;

                        //Write points
                        #pragma omp task
                        {
                        for(size_t i = 0; i < points->numPoints(); i++)
                        {
                            floatArr pts = points->getPointArray();
                            mmf_points[3 * numPointsInProject + 3 * i    ] = pts[i * 3];
                            mmf_points[3 * numPointsInProject + 3 * i + 1] = pts[i * 3 + 1];
                            mmf_points[3 * numPointsInProject + 3 * i + 2] = pts[i * 3 + 2];
                        }
                        }

                        if(points->hasColors())
                        {
                            // TODO: Implement with correct color width
                            ucharArr pts = points->getColorArray(w_color);
                             #pragma omp task
                            {
                            for(size_t i = 0; i < points->numPoints(); i++)
                            {
                                mmf_colors[3 * numPointsInProject + 3 * i    ] = pts[i * 3];
                                mmf_colors[3 * numPointsInProject + 3 * i + 1] = pts[i * 3 + 1];
                                mmf_colors[3 * numPointsInProject + 3 * i + 2] = pts[i * 3 + 2];
                            }
                            }
                            scansWithColors++;
                        }

                        if(points->hasNormals())
                        {
                            floatArr pts = points->getNormalArray();
                            #pragma omp task
                            {
                            for(size_t i = 0; i < points->numPoints(); i++)
                            {
                                mmf_normals[3 * numPointsInProject + 3 * i    ] = pts[i * 3];
                                mmf_normals[3 * numPointsInProject + 3 * i + 1] = pts[i * 3 + 1];
                                mmf_normals[3 * numPointsInProject + 3 * i + 2] = pts[i * 3 + 2];
                            }
                            }
                            scansWithNormals++;
                        }

                        numPointsInProject += points->numPoints();
                    }
                    #pragma omp barrier
                    // Release data. This causes IO overhead, but
                    // we do not want to store all data in RAM
                    scan->release();
                }
            }
        }
    }
        lvr2::log::info("{}{}{}", fmt::streamed("[WriteScanProjectToPLY]: Scan project has "), fmt::streamed(numPointsInProject), fmt::streamed(" points."));
        lvr2::log::info("{}{}{}", fmt::streamed("[WriteScanProjectToPLY]: Found "), fmt::streamed(scansWithNormals), fmt::streamed(" scans with normals."));
        lvr2::log::info("{}{}{}", fmt::streamed("[WriteScanProjectToPLY]: Found "), fmt::streamed(scansWithColors), fmt::streamed(" scans with colors."));

    // Check color / normal consistency
    bool exportColors = (scansWithColors == totalScans);
    bool exportNormals = (scansWithNormals == totalScans);

    if(exportNormals)
    {
                lvr2::log::info("{}", fmt::streamed("[WriteScanProjectToPLY]: Exporting normals."));
    }

    if(exportColors)
    {
                lvr2::log::info("{}", fmt::streamed("[WriteScanProjectToPLY]: Exporting colors."));
    }

    // Step 2: Write PLY header
    outfile << "ply" << std::endl;
    outfile << "format binary_little_endian 1.0" << std::endl;
    outfile << "element vertex " << numPointsInProject << std::endl;
    outfile << "property float x" << std::endl;
    outfile << "property float y" << std::endl;
    outfile << "property float z" << std::endl;

    if(exportColors)
    {
        outfile << "property uchar red" << std::endl;
        outfile << "property uchar green" << std::endl;
        outfile << "property uchar blue" << std::endl;
    }

    if(exportNormals)
    {
        outfile << "property float nx" << std::endl;
        outfile << "property float ny" << std::endl;
        outfile << "property float nz" << std::endl;
    }
    outfile << "end_header" << std::endl;

    // Determine size of single point
    size_t buffer_size = 3 * sizeof(float);

    if (exportColors)
    {
        buffer_size += w_color * sizeof(unsigned char);
    }

    if (exportNormals)
    {
        buffer_size += 3 * sizeof(float);
    }

    // Alloc buffer for a single point
    char *buffer = new char[buffer_size];

    // Read data from memory mapped files and write
    // to target file
    for (size_t i = 0; i < numPointsInProject; i++)
    {
        char *ptr = &buffer[0];

        // Write coordinates to buffer
        *((float *)ptr) = mmf_points[3 * i];
        ptr += sizeof(float);
        *((float *)ptr) = mmf_points[3 * i + 1];
        ptr += sizeof(float);
        *((float *)ptr) = mmf_points[3 * i + 2];

        // Write colors to buffer
        if (exportColors)
        {
            ptr += sizeof(float);
            *((unsigned char *)ptr) = mmf_colors[3 * i];
            ptr += sizeof(unsigned char);
            *((unsigned char *)ptr) = mmf_colors[3 * i + 1];
            ptr += sizeof(unsigned char);
            *((unsigned char *)ptr) = mmf_colors[3 * i + 2];
        }

        if (exportNormals)
        {
            ptr += sizeof(unsigned char);
            *((float *)ptr) = mmf_normals[3 * i];
            ptr += sizeof(float);
            *((float *)ptr) = mmf_normals[3 * i + 1];
            ptr += sizeof(float);
            *((float *)ptr) = mmf_normals[3 * i + 2];
        }
        outfile.write((const char *)buffer, buffer_size);
    }

    // Cleanup
    delete[] buffer;
    outfile.close();
    pointFile.close();
    normalFile.close();
    colorFile.close();
}

} // namespace lvr2

