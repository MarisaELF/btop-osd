cmake_minimum_required(VERSION 3.16)
project(BtopOsd VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(LayerShellQt REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_executable(btop-osd
    src/main.cpp
    src/osdwidget.cpp
    src/osdengine.cpp
    src/controlpanel.cpp
    src/config.cpp
    src/resources.qrc
    # vendored btop collection code (copied from reference/, unmodified)
    src/btop/btop_tools.cpp
    src/btop/btop_shared.cpp
    src/btop/btop_log_impl.cpp
    src/btop/btop_config_impl.cpp
    src/btop/btop_globals_impl.cpp
    src/btop/linux/btop_collect.cpp
    src/btop/linux/intel_gpu_top/igt_perf.c
    src/btop/linux/intel_gpu_top/intel_device_info.c
    src/btop/linux/intel_gpu_top/intel_gpu_top.c
    src/btop/linux/intel_gpu_top/intel_name_lookup_shim.c
)

target_include_directories(btop-osd PRIVATE src/btop src/btop/linux)

target_compile_definitions(btop-osd PRIVATE GPU_SUPPORT)

# The vendored btop / intel-gpu-top sources are not part of btop-osd's own code;
# silence their warnings to keep the build output readable.
set_source_files_properties(
    src/btop/btop_tools.cpp
    src/btop/btop_shared.cpp
    src/btop/btop_log_impl.cpp
    src/btop/btop_config_impl.cpp
    src/btop/btop_globals_impl.cpp
    src/btop/linux/btop_collect.cpp
    PROPERTIES COMPILE_OPTIONS "-w"
)
set_source_files_properties(
    src/btop/linux/intel_gpu_top/igt_perf.c
    src/btop/linux/intel_gpu_top/intel_device_info.c
    src/btop/linux/intel_gpu_top/intel_gpu_top.c
    src/btop/linux/intel_gpu_top/intel_name_lookup_shim.c
    PROPERTIES COMPILE_OPTIONS "-w"
)

target_link_libraries(btop-osd
    Qt6::Widgets
    LayerShellQt::Interface
    fmt::fmt
    dl
)
