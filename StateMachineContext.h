#pragma once

#include "StateExport.h"

#include <QString>
#include <QVariantMap>

namespace gsm
{
class StateMachine;

// 动作/条件共享的运行时上下文。
//  - machine：所属状态机（可查询 currentState 等）
//  - event：触发本次转移的事件名；entry/exit 动作为空串
//  - data：handleEvent 时调用方附带的事件参数
struct GENERAL_STATE_MACHINE_API StateMachineContext
{
    StateMachine *machine = nullptr;
    QString event;
    QVariantMap data;
};

} // namespace gsm
