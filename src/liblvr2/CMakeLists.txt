set(LVR2_SOURCES
    algorithm/ChunkBuilder.cpp
    algorithm/ChunkManager.cpp
    algorithm/ChunkHashGrid.cpp
    algorithm/HLODTree.cpp
    algorithm/UtilAlgorithms.cpp
    algorithm/pmp/DifferentialGeometry.cpp
    algorithm/pmp/DistancePointTriangle.cpp
    algorithm/pmp/SurfaceFactory.cpp
    algorithm/pmp/SurfaceGeodesic.cpp
    algorithm/pmp/SurfaceParameterization.cpp
    algorithm/pmp/SurfaceSmoothing.cpp
    algorithm/pmp/TriangleKdTree.cpp
    algorithm/pmp/SurfaceFairing.cpp
    algorithm/pmp/SurfaceHoleFilling.cpp
    algorithm/pmp/SurfaceRemeshing.cpp
    algorithm/pmp/SurfaceSubdivision.cpp
    algorithm/pmp/SurfaceCurvature.cpp
    algorithm/pmp/SurfaceFeatures.cpp
    algorithm/pmp/SurfaceNormals.cpp
    algorithm/pmp/SurfaceSimplification.cpp
    algorithm/pmp/SurfaceTriangulation.cpp
    config/lvropenmp.cpp
    config/BaseOption.cpp
    display/Renderable.cpp
    # display/GroundPlane.cpp
    display/MultiPointCloud.cpp
    display/StaticMesh.cpp
    display/Color.cpp
    display/Grid.cpp
    display/PointCloud.cpp
    display/InteractivePointCloud.cpp
    display/CoordinateAxes.cpp
    display/GlTexture.cpp
    display/PointCorrespondences.cpp
    # display/Arrow.cpp
    display/TexturedMesh.cpp
    display/MeshCluster.cpp
    geometry/pmp/SurfaceMesh.cpp
    geometry/pmp/SurfaceMeshIO.cpp
    geometry/SoilAssistField.cpp
    geometry/SoilAssistSubField.cpp
    io/baseio/yaml/Matrix.cpp
    io/modelio/AsciiIO.cpp
    io/modelio/ModelIOBase.cpp
    io/modelio/DatIO.cpp
    io/modelio/LasIO.cpp
    io/modelio/GeoTIFFIO.cpp
    # io/KinectIO.cpp
    io/AttributeMeshIOBase.cpp
    io/modelio/PPMIO.cpp
    io/modelio/PLYIO.cpp
    io/modelio/UosIO.cpp
    io/modelio/PCDIO.cpp
    io/kernels/DirectoryKernel.cpp
    io/kernels/HDF5Kernel.cpp
    io/LineReader.cpp
    # io/HDF5IO.cpp
    io/GridIO.cpp
    io/ModelFactory.cpp
    io/MeshIOFacade.cpp
    io/storage/StorageRegistry.cpp
    io/storage/FileKernelStorageBackend.cpp
    io/scan/ProjectStore.cpp
    # io/ScanDataManager.cpp
    io/ScanDirectoryParser.cpp
    io/schema/ScanProjectSchemaRaw.cpp
    io/schema/ScanProjectSchemaSlam6D.cpp
    io/schema/ScanProjectSchemaEuRoC.cpp
    # io/schema/ScanProjectSchemaOusterPLY.cpp
    io/schema/ScanProjectSchema.cpp
    io/schema/MeshSchemaDirectory.cpp
    io/schema/MeshSchemaHDF5.cpp
    # io/schema/ScanProjectSchemaSLAM.cpp
    # io/schema/ScanProjectSchemaHyperlib.cpp
    io/schema/ScanProjectSchemaHDF5.cpp
    # io/scanio/LabelScanProjectSchemaHDF5V2.cpp
    io/scanio/DirectoryDataIO.cpp
    io/scanio/MetaFormatFactory.cpp
    # io/scanio/HDF5MetaDescriptionV2.cpp
    reconstruction/Projection.cpp
    reconstruction/PanoramaNormals.cpp
    reconstruction/ModelToImage.cpp
    reconstruction/LBKdTree.cpp
    registration/ICPPointAlign.cpp
    registration/SLAMScanWrapper.cpp
    registration/Metascan.cpp
    registration/SLAMAlign.cpp
    registration/GraphSLAM.cpp
    registration/NearestCenterOctreeReduction.cpp
    registration/RandomSampleOctreeReduction.cpp
    registration/RegistrationPipeline.cpp
    registration/FPFH.cpp
    types/CustomChannelTypes.cpp
    types/MeshBuffer.cpp
    types/PolygonBuffer.cpp
    types/PointBuffer.cpp
    types/DistortionModels.cpp
    texture/Texture.cpp
    texture/TextureFactory.cpp
    util/ColorGradient.cpp
    util/CoordinateTransform.cpp
    util/Hdf5Util.cpp
    util/IOUtils.cpp
    util/Synthetic.cpp
    util/ScanProjectSchemaUtils.cpp
    util/ScanProjectUtils.cpp
    util/Util.cpp
    util/Progress.cpp
    util/Timestamp.cpp
    util/Logging.cpp
    )

#####################################################################################
# Add PCD io if PCL is installed
#####################################################################################

if(PCL_FOUND)
    list(APPEND LVR2_SOURCES
        reconstruction/PCLFiltering.cpp)
endif(PCL_FOUND)

if(embree_FOUND)
    list(APPEND LVR2_SOURCES 
        algorithm/raycasting/EmbreeRaycaster.cpp
)
endif(embree_FOUND)

if(WITH_FREENECT AND LIBFREENECT_FOUND)
    list(APPEND LVR2_SOURCES
        io/KinectGrabber.cpp 
        io/KinectIO.cpp)
endif()

if(RDB_FOUND)
    list(APPEND LVR2_SOURCES 
        io/modelio/RdbxIO.cpp
        io/schema/ScanProjectSchemaRdbx.cpp)
endif(RDB_FOUND)

#####################################################################################
# Draco Geometry Compression
#####################################################################################

if(draco_FOUND)
    list(APPEND LVR2_SOURCES
        io/modelio/DracoEncoder.cpp
        io/modelio/DracoDecoder.cpp 
        io/modelio/DrcIO.cpp)
endif(draco_FOUND)

#####################################################################################
# 3D Tiles
#####################################################################################

if(LVR2_WITH_3DTILES)
    list(APPEND LVR2_SOURCES
        io/Tiles3dIO.cpp
        io/modelio/B3dmIO.cpp)
endif()

#####################################################################################
# RiVLib
#####################################################################################

if(RiVLib_FOUND)
    list(APPEND LVR2_SOURCES
        io/baseio/RxpIO.cpp)
endif()

#####################################################################################
# OpenCL
#####################################################################################

if(OPENCL_FOUND)
    list(APPEND LVR2_SOURCES
        reconstruction/opencl/ClSurface.cpp
        reconstruction/opencl/ClStatisticalOutlierFilter.cpp)
endif()

#####################################################################################
# Setup dependencies to external libraries
#####################################################################################

set(LVR2_INTERNAL_DEPENDENCIES_STATIC ${LVR2_RPLY_TARGET} ${LVR2_LASLIB_TARGET})
set(LVR2_INTERNAL_DEPENDENCIES_SHARED ${LVR2_RPLY_TARGET} ${LVR2_LASLIB_TARGET})

# Export name for the default aggregate target in this build mode.
set(_LVR2_STATIC_EXPORT_NAME lvr2_static)
if(BUILD_SHARED_LIBS)
  set(_LVR2_STATIC_EXPORT_NAME lvr2_static)
else()
  set(_LVR2_STATIC_EXPORT_NAME lvr2)
endif()

#####################################################################################
# Set c++0x flags for gcc compilers (needed for boctree io)
#####################################################################################

if(UNIX)
  SET_SOURCE_FILES_PROPERTIES(io/BoctreeIO.cpp PROPERTIES COMPILE_FLAGS "-std=c++14")
  SET_SOURCE_FILES_PROPERTIES(util/Logging.cpp PROPERTIES COMPILE_FLAGS "-fvisibility=hidden")
endif(UNIX)

#####################################################################################
# Build static and shared libraries
#####################################################################################

set(_LVR2_LIBRARY_TARGETS)

if(LVR2_BUILD_STATIC_LIBS)
    message(STATUS "Building static library")
    add_library(lvr2core OBJECT ${LVR2_SOURCES})
    target_link_libraries(lvr2core
        PRIVATE
        ${LVR2_SPDLOG_TARGET}
        ${LVR2_RPLY_TARGET}
        ${LVR2_LASLIB_TARGET}
    )
    target_link_libraries(lvr2core
        PUBLIC
        Eigen3::Eigen
        tl::expected
        ${LVR2_HIGHFIVE_TARGET}
    )

    add_library(lvr2_static STATIC $<TARGET_OBJECTS:lvr2core>)
    target_include_directories(lvr2_static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        ${HDF5_INCLUDE_DIRS}
    )
    target_link_libraries(lvr2_static
        PUBLIC
        tl::expected
        ${LVR2_HIGHFIVE_TARGET}
        ${LVR2_INTERNAL_DEPENDENCIES_STATIC}
        ${LVR2_LIB_DEPENDENCIES}
    )
    target_link_libraries(lvr2_static
        PRIVATE
        ${LVR2_SPDLOG_TARGET}
    )
    target_compile_definitions(lvr2_static
        PUBLIC
        ${LVR2_DEFINITIONS}
    )
    set_target_properties(lvr2_static
        PROPERTIES
        EXPORT_NAME ${_LVR2_STATIC_EXPORT_NAME}
        CXX_STANDARD 17
    )

    list(APPEND _LVR2_LIBRARY_TARGETS lvr2_static)
endif()

if(BUILD_SHARED_LIBS)
    message(STATUS "Building shared library")
    add_library(lvr2 SHARED ${LVR2_SOURCES})
    target_include_directories(lvr2 PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        ${Boost_INCLUDE_DIRS}
        ${HDF5_INCLUDE_DIRS}
    )
    target_link_libraries(lvr2
        PUBLIC
        Eigen3::Eigen
        tl::expected
        ${LVR2_HIGHFIVE_TARGET}
        ${LVR2_INTERNAL_DEPENDENCIES_SHARED}
        ${LVR2_LIB_DEPENDENCIES}
    )

    target_link_libraries(lvr2
        PRIVATE
        ${LVR2_SPDLOG_TARGET}
    )
    if(LVR2_MESH_IO_ASSIMP_AVAILABLE)
        target_sources(lvr2 PRIVATE io/AssimpMeshAdapter.cpp)
        if(LVR2_ASSIMP_TARGET)
            target_link_libraries(lvr2 PRIVATE ${LVR2_ASSIMP_TARGET})
        else()
            target_link_libraries(lvr2 PRIVATE ${LVR2_ASSIMP_LIBRARIES})
            target_include_directories(lvr2 PRIVATE ${LVR2_ASSIMP_INCLUDE_DIRS})
        endif()
        target_compile_definitions(lvr2 PRIVATE LVR2_MESH_IO_HAS_ASSIMP=1)
    endif()
    target_compile_definitions(lvr2
        PUBLIC
        ${LVR2_DEFINITIONS}
        -DLVR2_BUILDING_SHARED
    )
    set_target_properties(lvr2
        PROPERTIES
        EXPORT_NAME lvr2
        SOVERSION ${lvr2_VERSION_MAJOR}
        VERSION ${lvr2_VERSION}
        CXX_STANDARD 17
        VISIBILITY_INLINES_HIDDEN ON
    )

    list(APPEND _LVR2_LIBRARY_TARGETS lvr2)
endif()

if(LVR2_BUILD_STATIC_LIBS AND NOT BUILD_SHARED_LIBS)
    # Static-only builds keep the aggregate in-tree target available.
    add_library(lvr2 ALIAS lvr2_static)
endif()

if(NOT LVR2_BUILD_STATIC_LIBS AND BUILD_SHARED_LIBS)
    # In-tree compatibility for consumers hard-linking to lvr2_static.
    add_library(lvr2_static ALIAS lvr2)

endif()

set(_LVR2_COMPONENTS
    geometry
    io
    reconstruction
    texture
    registration
    display
)

set(_LVR2_COMPONENT_TARGETS)
if(BUILD_SHARED_LIBS)
    set(_LVR2_COMPONENT_AGGREGATE_TARGET lvr2)
elseif(LVR2_BUILD_STATIC_LIBS)
    set(_LVR2_COMPONENT_AGGREGATE_TARGET lvr2_static)
else()
    set(_LVR2_COMPONENT_AGGREGATE_TARGET)
endif()

if(_LVR2_COMPONENT_AGGREGATE_TARGET)
    foreach(_LVR2_COMPONENT IN LISTS _LVR2_COMPONENTS)
        set(_LVR2_COMPONENT_TARGET "lvr2_${_LVR2_COMPONENT}")
        add_library(${_LVR2_COMPONENT_TARGET} INTERFACE)
        target_link_libraries(${_LVR2_COMPONENT_TARGET} INTERFACE ${_LVR2_COMPONENT_AGGREGATE_TARGET})
        set_target_properties(${_LVR2_COMPONENT_TARGET}
            PROPERTIES
            EXPORT_NAME ${_LVR2_COMPONENT}
        )
        list(APPEND _LVR2_COMPONENT_TARGETS ${_LVR2_COMPONENT_TARGET})
    endforeach()
endif()

if(_LVR2_LIBRARY_TARGETS OR _LVR2_COMPONENT_TARGETS)
    install(TARGETS ${_LVR2_LIBRARY_TARGETS} ${_LVR2_COMPONENT_TARGETS}
      EXPORT lvr2-targets
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

#####################################################################################
# CUDA
#####################################################################################

if(CUDA_FOUND)
    set(LBVH_KERNEL_INCLUDES "${PROJECT_SOURCE_DIR}/include/lvr2/reconstruction/cuda/lbvh")
    add_definitions(-DLBVH_KERNEL_INCLUDES="${LBVH_KERNEL_INCLUDES}")
    
    set(LBVH_KERNEL_DIR "${PROJECT_SOURCE_DIR}/src/liblvr2/reconstruction/cuda/lbvh")
    add_definitions(-DLBVH_KERNEL_DIR="${LBVH_KERNEL_DIR}")
    
    add_definitions(-DCUDA_INCLUDE_DIRS="${CUDA_INCLUDE_DIRS}")
    
    set(CUDA_SEPARABLE_COMPILATION ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

    # List of CUDA kernel code sources
    set(LVR2_CUDA_SRC
        reconstruction/cuda/CudaSurface.cu
        reconstruction/cuda/LBVHIndex.cu
        reconstruction/cuda/lbvh/lbvh_kernels.cu
        reconstruction/cuda/lbvh/morton_code.cu
        reconstruction/cuda/lbvh/normals_kernel.cu
    )

    # find NVRTC - runtime compilation library for CUDA C++
	if (NOT DEFINED CUDA_NVRTC_LIBRARY)
        string(REGEX REPLACE cudart nvrtc CUDA_NVRTC_LIBRARY ${CUDA_CUDART_LIBRARY})
    endif()
    MESSAGE(STATUS "CUDA NVRTC: " ${CUDA_NVRTC_LIBRARY})

    if(NOT CUDA_NVRTC_LIBRARY)
        message(FATAL_ERROR "Required library 'nvrtc' not found.")
    endif(NOT CUDA_NVRTC_LIBRARY)

    set(_LVR2_CUDA_TARGETS)

    set(_LVR2_CUDA_STATIC_EXPORT_NAME lvr2cuda_static)
    if(NOT BUILD_SHARED_LIBS)
        set(_LVR2_CUDA_STATIC_EXPORT_NAME lvr2cuda)
    endif()

    if(LVR2_BUILD_STATIC_LIBS)
        message(STATUS "Building static LVR CUDA library")
        add_library(lvr2cuda_static STATIC ${LVR2_CUDA_SRC})

        # Add dependency to avoid that both targets
        # are build concurrently in parallel builds
        add_dependencies(lvr2cuda_static
            lvr2_static)

        target_link_libraries(lvr2cuda_static
            lvr2_static
            ${CUDA_LIBRARIES}
            ${CUDA_DRIVER_LIBRARY}
            ${CUDA_NVRTC_LIBRARY}
        )

        set_target_properties(lvr2cuda_static
        PROPERTIES
            EXPORT_NAME ${_LVR2_CUDA_STATIC_EXPORT_NAME}
            SOVERSION ${lvr2_VERSION_MAJOR}
            VERSION ${lvr2_VERSION}
            CUDA_SEPARABLE_COMPILATION ON
        )

        list(APPEND _LVR2_CUDA_TARGETS lvr2cuda_static)

        if(NOT BUILD_SHARED_LIBS)
            add_library(lvr2cuda ALIAS lvr2cuda_static)
        endif()
    endif()

    if(BUILD_SHARED_LIBS)
        message(STATUS "Building shared LVR CUDA library")

        add_library(lvr2cuda SHARED ${LVR2_CUDA_CPP_SRC} ${LVR2_CUDA_SRC})

        target_include_directories(lvr2cuda
            PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/lvr2>
                ${CUDA_INCLUDE_DIRS}
            )

        target_link_libraries(lvr2cuda PUBLIC
            lvr2
            ${CUDA_LIBRARIES}
            ${CUDA_DRIVER_LIBRARY}
            ${CUDA_NVRTC_LIBRARY}
        )

        add_dependencies(lvr2cuda
            lvr2)

        set_target_properties(lvr2cuda
            PROPERTIES
                SOVERSION ${lvr2_VERSION_MAJOR}
                VERSION ${lvr2_VERSION}
                CUDA_SEPARABLE_COMPILATION ON
        )

        list(APPEND _LVR2_CUDA_TARGETS lvr2cuda)

        if(NOT LVR2_BUILD_STATIC_LIBS)
            add_library(lvr2cuda_static ALIAS lvr2cuda)
        endif()
    endif()

    if(_LVR2_CUDA_TARGETS)
        install(
            TARGETS ${_LVR2_CUDA_TARGETS}
            EXPORT lvr2-targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    endif()

endif()

