cmake_minimum_required(VERSION 3.10)
project(g-engine VERSION 1.0.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS特定配置
if(APPLE)
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        # 对于Apple Clang，需要特殊处理OpenMP
        execute_process(
            COMMAND brew --prefix libomp
            OUTPUT_VARIABLE OPENMP_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(OPENMP_PREFIX)
            set(OpenMP_C_FLAGS "-Xpreprocessor -fopenmp -I${OPENMP_PREFIX}/include")
            set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I${OPENMP_PREFIX}/include")
            set(OpenMP_C_LIB_NAMES "omp")
            set(OpenMP_CXX_LIB_NAMES "omp")
            set(OpenMP_omp_LIBRARY "${OPENMP_PREFIX}/lib/libomp.dylib")
        endif()
    endif()
endif()

# 查找依赖包
find_package(OpenMP REQUIRED)

# 查找nlohmann_json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    # 如果没有找到系统安装的nlohmann_json，使用FetchContent下载
    include(FetchContent)
    FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
    )
    FetchContent_MakeAvailable(json)
endif()

# 设置库的源文件
set(SOURCES
    src/NewtonianSimulator.cpp
    src/BarnesHutSimulator.cpp
    src/OctreeNode.cpp
)

# 设置头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 创建静态库
add_library(gengine_static STATIC ${SOURCES})
if(nlohmann_json_FOUND)
    target_link_libraries(gengine_static PRIVATE OpenMP::OpenMP_CXX nlohmann_json::nlohmann_json)
else()
    target_link_libraries(gengine_static PRIVATE OpenMP::OpenMP_CXX nlohmann_json::nlohmann_json)
endif()
set_target_properties(gengine_static PROPERTIES OUTPUT_NAME "gengine")

# 创建动态库
add_library(gengine_shared SHARED ${SOURCES})
if(nlohmann_json_FOUND)
    target_link_libraries(gengine_shared PRIVATE OpenMP::OpenMP_CXX nlohmann_json::nlohmann_json)
else()
    target_link_libraries(gengine_shared PRIVATE OpenMP::OpenMP_CXX nlohmann_json::nlohmann_json)
endif()
set_target_properties(gengine_shared PROPERTIES OUTPUT_NAME "gengine")

# 创建可执行文件
add_executable(server main.cpp)
if(nlohmann_json_FOUND)
    target_link_libraries(server PRIVATE gengine_static OpenMP::OpenMP_CXX nlohmann_json::nlohmann_json)
else()
    target_link_libraries(server PRIVATE gengine_static OpenMP::OpenMP_CXX nlohmann_json::nlohmann_json)
endif()

# 安装规则
install(TARGETS gengine_static gengine_shared
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include/gengine)

# 导出CMake配置
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/GEngineConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)