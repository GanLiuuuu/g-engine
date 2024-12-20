cmake_minimum_required(VERSION 3.10)
project(celestial_simulation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 检测操作系统
if(APPLE)
    # macOS特定配置
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

# 查找OpenMP
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

# 添加可执行文件
add_executable(celestial_simulation main.cpp)

# 链接OpenMP
if(OpenMP_CXX_FOUND)
    target_link_libraries(celestial_simulation PRIVATE OpenMP::OpenMP_CXX)
endif()

# 链接nlohmann_json
if(nlohmann_json_FOUND)
    target_link_libraries(celestial_simulation PRIVATE nlohmann_json::nlohmann_json)
else()
    target_link_libraries(celestial_simulation PRIVATE nlohmann_json::nlohmann_json)
endif()

# 设置编译选项
if(MSVC)
    # Windows with MSVC
    target_compile_options(celestial_simulation PRIVATE /W4)
else()
    # Linux, macOS, and Windows with MinGW
    target_compile_options(celestial_simulation PRIVATE -Wall -Wextra)
endif()

# 如果是Windows系统，设置可执行文件的输出目录
if(WIN32)
    set_target_properties(celestial_simulation PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}
    )
endif()