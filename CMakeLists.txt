cmake_minimum_required(VERSION 3.21)

# 允许单独构建本库，也允许顶层一次构建全部
if(NOT CMAKE_TOOLCHAIN_FILE)
    if ($ENV{VCPKG_ROOT} EQUAL "")
        message(FATAL_ERROR "environment VCPKG_ROOT is not setted")
    endif()
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

# ============================================================
#  版本配置：cmake/version.json（单一来源）
#  解读逻辑封装在 cmake/VersionConfig.cmake，各模块可复用；
#  修改版本号只需编辑 JSON 文件，无需改动本 CMakeLists。
# ============================================================
include(cmake/VersionConfig.cmake)
read_version(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.json"
    GENERAL_STATE_MACHINE
)
project(GeneralStateMachine VERSION ${GENERAL_STATE_MACHINE_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
#  GeneralStateMachine：独立通用状态机库（顶层模块，不归属 Common）
#  XML 配置（StateMachineSpec）→ StateMachine 引擎；动作/条件按名字
#  绑定到宿主成员函数（connectAction/connectCondition 或
#  CONNECT_STATE_MACHINE_ACTION/CONDITION，MITK 新版
#  EventStateMachine 风格，无全局注册表）。
#  仅依赖 Qt（Qt5/Qt6 均可，Core 组件）+ spdlog，不依赖本仓库其他模块，可独立迭代。
# ============================================================
find_package(spdlog CONFIG REQUIRED)
# fmt 是 spdlog 的二进制依赖（vcpkg 默认编译模式），运行期分发用
find_package(fmt CONFIG REQUIRED)
# 同时支持 Qt5/Qt6（CMake 3.19+ 的 QT NAMES 语法，优先 Qt6）
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

# ============================================================
#  动态库
# ============================================================
add_library(${PROJECT_NAME} SHARED
    StateMachine.cpp
    StateMachineConfig.cpp
    StateMachineFactory.cpp
)

# Windows：生成并编译 VERSIONINFO 资源，使 DLL 携带四位版本号
# （文件属性 -> 详细信息；版本号来自 cmake/version.json 单一来源）
if(WIN32)
    string(REPLACE "." "," GENERAL_STATE_MACHINE_VERSION_COMMA "${GENERAL_STATE_MACHINE_VERSION}")
    configure_file(
        cmake/version.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
        @ONLY
    )
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
endif()

# 非 Windows：通过 SONAME / 版本符号链接携带版本信息
#  - Linux:  libGeneralStateMachine.so -> libGeneralStateMachine.so.1 -> libGeneralStateMachine.so.1.0.0.0
#  - macOS:  libGeneralStateMachine.1.0.0.0.dylib（current_version=1.0.0, compatibility=1）
if(NOT WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VERSION   ${GENERAL_STATE_MACHINE_VERSION}
        SOVERSION ${GENERAL_STATE_MACHINE_VERSION_MAJOR}
    )
endif()

# 头文件不随库安装，故仅构建期接口（BUILD_INTERFACE）：
#  - add_subdirectory 下游（如顶层构建）可正常 include；
#  - install(EXPORT) 时该属性为空，不会引用源目录而报错。
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# 构建共享库时定义导出宏（客户端链接时走 dllimport 分支）
target_compile_definitions(${PROJECT_NAME} PRIVATE GENERAL_STATE_MACHINE_BUILD)

# spdlog 仅内部使用（StateMachine.cpp 打日志），公共头文件不暴露，
# 因此 PRIVATE 链接，外部消费方无需安装/链接 spdlog。
# Qt Core 为公共 API 类型（QString/QVariantMap 等），PUBLIC 传递。
target_link_libraries(${PROJECT_NAME} PRIVATE
    spdlog::spdlog
)
target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
)

# ============================================================
#  安装 / 导出（供外部 find_package(GeneralStateMachine CONFIG) 使用）
#   cmake --install <build> [--prefix <dir>]
#  用法示例见 Docs/architecture.md
# ============================================================
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS ${PROJECT_NAME}
    EXPORT GeneralStateMachineTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# GeneralStateMachine.dll 运行期依赖 spdlog/fmt（vcpkg 动态库），
# 随库分发到 bin/，保证消费方无需自行安装 spdlog/fmt 即可运行。
install(FILES
    $<TARGET_FILE:spdlog::spdlog>
    $<TARGET_FILE:fmt::fmt>
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    OPTIONAL
)

# 注：头文件不随库安装（由消费方/源码树自行管理）

install(EXPORT GeneralStateMachineTargets
    NAMESPACE GeneralStateMachine::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GeneralStateMachine
)

# Config 包：find_package(GeneralStateMachine CONFIG REQUIRED)
configure_package_config_file(
    cmake/GeneralStateMachineConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/GeneralStateMachineConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GeneralStateMachine
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/GeneralStateMachineConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/GeneralStateMachineConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/GeneralStateMachineConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GeneralStateMachine
)

# ============================================================
#  单元测试（gtest；include(CTest) 提供 BUILD_TESTING，默认 OFF）
# ============================================================
if(BUILD_TESTING)
    include(CTest)
    add_subdirectory(tests)
endif()
