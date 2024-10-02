cmake_minimum_required(VERSION 3.12)
project(SVO LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# set(CMAKE_VS_PLATFORM_TOOLSET_CUDA "v12.4")

# set(CUDAToolkit_ROOT "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.4")  # Adjust this path
# set(CMAKE_CUDA_COMPILER "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.4/bin/nvcc.exe")  # Optional but might help

find_package(OpenVDB CONFIG REQUIRED)
find_package(VTK REQUIRED)
find_package(CUDAToolkit REQUIRED)  # Use the modern way of finding CUDA

enable_language(CUDA)

# Uncomment if needed for VTK
#find_package(SVO COMPONENTS 
#  CommonColor
#  CommonCore
#  CommonDataModel
#  FiltersCore
#  FiltersGeneral
#  ImagingHybrid
#  InteractionStyle
#  RenderingContextOpenGL2
#  RenderingCore
#  RenderingFreeType
#  RenderingGL2PSOpenGL2
#  RenderingOpenGL2
#)

add_executable(SVO main.cpp)

if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT SVO)
    target_compile_options(SVO PRIVATE /bigobj)
endif()

set(VTK_INCLUDE_DIR ${Z_VCPKG_ROOT_DIR}/installed/x64-windows/include/vtk-9.3)
set(VTK_INCLUDE_DIRS ${Z_VCPKG_ROOT_DIR}/installed/x64-windows/include/vtk-9.3)

target_include_directories(SVO PRIVATE
    "External"
    "External/eigen"
    ${OPENVDB_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIRS}
    ${CUDAToolkit_INCLUDE_DIRS}  # Add CUDA include directories
)

target_link_libraries(SVO PRIVATE OpenVDB::openvdb)
target_link_libraries(SVO PRIVATE ${VTK_LIBRARIES})
target_link_libraries(SVO PRIVATE CUDA::cudart CUDA::cuda_driver)  # Link CUDA runtime and driver libraries

# Enable CUDA runtime API and any necessary options for your project
set_target_properties(SVO PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON  # Enable separable compilation if needed
    CUDA_STANDARD 11  # Use CUDA 11 (you can adjust this based on your needs)
)

vtk_module_autoinit(
    TARGETS SVO
    MODULES ${VTK_LIBRARIES}
)
