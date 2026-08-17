#pragma once

#include "StateExport.h"
#include "StateMachine.h"

#include <QByteArray>
#include <QString>
#include <memory>

namespace gsm
{
// ============================================================
//  StateMachineFactory — 状态机构建器
//
//  解析 XML（StateMachineSpec）并构造 StateMachine 引擎。
//  动作/条件不在构建期绑定：由调用方在得到引擎后通过
//  connectAction/connectCondition 绑定成员函数
//  （MITK 新版 EventStateMachine 风格，无全局注册表）。
// ============================================================
class GENERAL_STATE_MACHINE_API StateMachineFactory
{
public:
    static StateMachineFactory &instance();

    // 由 XML 字节解析并构造引擎。
    // 失败（解析错误）返回 nullptr 并在 error（若非空）中给出原因。
    std::unique_ptr<StateMachine> createFromXml(const QByteArray &xml, QString *error = nullptr) const;

    // 由 XML 文件解析并构造引擎。
    std::unique_ptr<StateMachine> createFromFile(const QString &path, QString *error = nullptr) const;

private:
    StateMachineFactory() = default;
};
} // namespace gsm
