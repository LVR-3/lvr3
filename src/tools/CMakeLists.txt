if(NOT MSVC)
# add_subdirectory(lvr2_largescale_reconstruct)
endif(NOT MSVC)

if(LVR2_BUILD_TOOLS)
  add_subdirectory(lvr2_reconstruct)
  add_subdirectory(lvr2_mesh_reducer)
  add_subdirectory(lvr2_hdf5_mesh_tool)
endif(LVR2_BUILD_TOOLS)

if(LVR2_BUILD_TOOLS_EXPERIMENTAL)

  add_subdirectory(lvr2_dmc_reconstruction)
  add_subdirectory(lvr2_gs_reconstruction)
  add_subdirectory(lvr2_asciiconverter)
  add_subdirectory(lvr2_transform)
  add_subdirectory(lvr2_kaboom)
  add_subdirectory(lvr2_octree_test)
  add_subdirectory(lvr2_image_normals)
  add_subdirectory(lvr2_plymerger)

  # add_subdirectory(lvr2_slam2hdf5)
  # add_subdirectory(lvr2_hdf5togeotiff)
  add_subdirectory(lvr2_slam6d_merger)
  add_subdirectory(lvr2_chunking)
  # add_subdirectory(lvr2_registration)
  add_subdirectory(lvr2_chunking_server)
  add_subdirectory(lvr2_scanproject_parser)
  add_subdirectory(lvr2_ground_level_extractor)
  # add_subdirectory(lvr2_hdf5_convert_old)
  add_subdirectory(lvr2_hdf5_inspect)
  add_subdirectory(lvr2_3dtiles)
  #add_subdirectory(teaser_example)

  if(RiVLib_FOUND)
      #add_subdirectory(lvr2_riegl_project_converter)
  endif()

  if(CUDA_FOUND)
      add_subdirectory(lvr2_cuda_normals)
  endif()

  if(OPENCL_FOUND)
      add_subdirectory(lvr2_cl_normals)
      add_subdirectory(lvr2_cl_sor)
  endif()

endif(LVR2_BUILD_TOOLS_EXPERIMENTAL)

function(lvr2_tool_uses_private_library_headers target_name)
  if(TARGET ${target_name})
    get_target_property(_lvr2_tool_type ${target_name} TYPE)
    if(NOT _lvr2_tool_type STREQUAL "INTERFACE_LIBRARY")
      target_include_directories(${target_name} PRIVATE "${PROJECT_SOURCE_DIR}/src/liblvr2/private")
    endif()
  endif()
endfunction()

foreach(_lvr2_private_header_tool IN ITEMS
    lvr2_3dtiles
    lvr2_ascii_convert
    lvr2_chunking
    lvr2_cl_normals
    lvr2_cl_sor
    lvr2_cuda_normals
    lvr2_dmc_reconstruction
    lvr2_fastsense_reconstruction
    lvr2_ground_level_extractor
    lvr2_gs_reconstruction
    lvr2_hdf5_mesh_tool
    lvr2_hdf5tool
    lvr2_mesh_reduce
    lvr2_image_normals
    lvr2_kaboom
    lvr2_largescale_reconstruct
    lvr2_largescale_reconstruct_mpi
    lvr2_octree_test
    lvr2_ply_merger
    lvr2_reconstruct
    lvr2_registration
    lvr2_riegl_project_converter
    lvr2_slam6d_merger
    lvr2_transform_model)
  lvr2_tool_uses_private_library_headers(${_lvr2_private_header_tool})
endforeach()
