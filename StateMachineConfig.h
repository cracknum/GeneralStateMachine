#pragma once

#include "StateExport.h"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

namespace gsm
{
// ============================================================
//  状态机 XML 配置模型（StateMachineConfig）
//
//  XML 结构（参考 MITK 交互状态机的配置驱动思想，但通用化，
//  无任何交互专属概念，事件即普通字符串）：
//
//  <stateMachine name="volume_load" initial="IDLE">
//      <state name="IDLE">
//          <entryAction name="log_entry"/>            <!- 可选 -->
//          <exitAction name="log_exit"/>              <!- 可选 -->
//          <transition event="load_requested" target="LOADING">
//              <condition name="path_valid"/>         <!- 可选，按序全真才转移 -->
//              <action name="begin_load"/>            <!- 可选，按序执行 -->
//          </transition>
//      </state>
//      ...
//  </stateMachine>
//
//  命名规则与 MITK 对齐：state/transition/action/condition 均为
//  配置实体；工厂构建时按名字查注册表实例化（延迟绑定）。
// ============================================================

// 单条转移：event 触发，target 为目标状态；
// conditions 全部通过才转移，actions 在转移时按序执行。
struct GENERAL_STATE_MACHINE_API Transition
{
    QString event;
    QString target;
    QStringList conditions; // condition 名字（按序评估，短路）
    QStringList actions;    // action 名字（按序执行）
};

// 单个状态：可选 entry/exit 动作 + 转移表。
struct GENERAL_STATE_MACHINE_API State
{
    QString name;
    QStringList entryActions;  // 进入该状态时执行
    QStringList exitActions;   // 离开该状态时执行
    QVector<Transition> transitions;
};

// 完整状态机配置（解析后的内存模型，与运行时引擎分离）。
struct GENERAL_STATE_MACHINE_API StateMachineConfig
{
    QString name;
    QString initial;
    QVector<State> states;

    // 从 XML 字节解析；失败返回 false 并在 error（若非空）中给出原因。
    bool fromXml(const QByteArray &xml, QString *error = nullptr);
    // 从 XML 文件解析；失败返回 false。
    bool fromFile(const QString &path, QString *error = nullptr);
};

} // namespace gsm
