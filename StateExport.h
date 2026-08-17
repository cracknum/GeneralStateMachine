#pragma once

// ============================================================
//  GeneralStateMachine 共享库导出宏
//
//  Windows（MSVC）下 DLL 必须显式导出符号，否则不生成导入库（.lib）：
//    - 构建 GeneralStateMachine 时（CMake 已定义 GENERAL_STATE_MACHINE_BUILD）→ dllexport
//    - 外部链接（include 头文件）→ dllimport
//  非 Windows 平台为空实现（符号默认导出，无需标记）。
// ============================================================
#if defined(_WIN32) || defined(_WIN64)
#  if defined(GENERAL_STATE_MACHINE_BUILD)
#    define GENERAL_STATE_MACHINE_API __declspec(dllexport)
#  else
#    define GENERAL_STATE_MACHINE_API __declspec(dllimport)
#  endif
#else
#  define GENERAL_STATE_MACHINE_API
#endif
