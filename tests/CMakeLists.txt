# ============================================================
#  GeneralStateMachine 单元测试（gtest）
#  覆盖：
#    state_machine_test —— XML 解析/构建/事件转移/条件短路/动作顺序/成员函数绑定
# ============================================================
find_package(GTest CONFIG REQUIRED)

add_executable(GeneralStateMachineTests
    state_machine_test.cpp
)

target_link_libraries(GeneralStateMachineTests PRIVATE
    GeneralStateMachine
    GTest::gtest_main   # gtest_main 提供 main；state 测试为纯逻辑无需事件循环
    Qt${QT_VERSION_MAJOR}::Core
)

# 运行期依赖：把 GeneralStateMachine.dll + spdlog/fmt 运行时复制到测试 exe 同目录
# （不改变库/测试的默认输出布局；安装由 install(TARGETS) 负责）
add_custom_command(TARGET GeneralStateMachineTests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:GeneralStateMachine>"
        "$<TARGET_FILE:spdlog::spdlog>"
        "$<TARGET_FILE:fmt::fmt>"
        "$<TARGET_FILE_DIR:GeneralStateMachineTests>"
)

# 测试 exe 也安装到 bin/（与库同目录，运行时可定位 GeneralStateMachine.dll）
install(TARGETS GeneralStateMachineTests
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# gtest 运行时（vcpkg 默认动态库）；OPTIONAL 容错（静态 gtest 时跳过）
install(FILES
    $<TARGET_FILE:GTest::gtest>
    $<TARGET_FILE:GTest::gtest_main>
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    OPTIONAL
)

# 注册到 CTest（由 State/CMakeLists.txt include(CTest) 启用）
include(GoogleTest)
gtest_discover_tests(GeneralStateMachineTests)
