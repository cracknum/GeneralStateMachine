#pragma once

#include "StateExport.h"
#include "StateMachineConfig.h"
#include "StateMachineContext.h"

#include <QHash>
#include <functional>

namespace gsm
{
// ============================================================
//  StateMachine — 通用状态机运行时引擎
//
//  由 StateMachineFactory 从 StateMachineSpec（XML）构建：
//  配置（spec）与运行时（引擎）分离。
//
//  动作/条件实现（参考 MITK 新版 EventStateMachine 风格）：
//    在 StateMachine 派生类里 override 虚函数 bind()，把 XML 中引用的
//    名字绑定到成员函数（connectAction/connectCondition，或用
//    CONNECT_STATE_MACHINE_ACTION / CONNECT_STATE_MACHINE_CONDITION 宏），
//    无需为每个动作新建类，动作/条件函数与宿主类直接共享状态。
//    start() 首次执行时自动调用 bind()（幂等），调用方无需手动触发。
//
//  执行顺序（参考 MITK 状态语义）：
//    旧状态 exitActions → transition actions → 切换状态
//    → 新状态 entryActions
//
//  语义：
//    - handleEvent：当前状态匹配到 event 的转移，全部 condition
//      通过才执行动作并转移；无匹配或条件不满足 → 状态不变，返回 false
//    - 未 start() 时事件先自动 start()（进入 initial，执行其 entry）
// ============================================================
class GENERAL_STATE_MACHINE_API StateMachine
{
public:
    using ActionFunction = std::function<void(StateMachineContext &)>;
    using ConditionFunction = std::function<bool(const StateMachineContext &)>;

    StateMachine() = delete;
    StateMachine(const StateMachine &) = delete;
    StateMachine &operator=(const StateMachine &) = delete;

    explicit StateMachine(StateMachineConfig spec);
    virtual ~StateMachine() = default;

    // 绑定入口（参考 MITK EventStateMachine::ConnectActionsAndFunctions）：
    // 派生类 override 本函数，用 CONNECT_STATE_MACHINE_ACTION /
    // CONNECT_STATE_MACHINE_CONDITION 宏（或 connectAction/connectCondition）
    // 把 XML 引用的动作/条件名绑定到自身成员函数。
    // start() 首次执行时自动调用本函数（幂等）；默认空实现，
    // 无需绑定的机器可直接使用。
    virtual void bind() {}

    // 将动作/条件名字绑定到本实例的处理函数（可调用对象/成员函数）。
    // 空名字或空函数忽略。未绑定的名字在运行时触发时报错。
    void connectAction(const QString &name, ActionFunction fn);
    void connectCondition(const QString &name, ConditionFunction fn);

    // 进入 initial 状态并执行其 entry 动作；已启动则幂等返回 true。
    bool start();

    // 事件驱动转移。返回 true 表示发生了状态转移。
    bool handleEvent(const QString &event, const QVariantMap &data = {});

    const QString &currentState() const { return m_current; }
    const StateMachineConfig &spec() const { return m_spec; }
    bool started() const { return m_started; }

private:
    const State *findState(const QString &name) const;
    bool runActions(const QStringList &names, StateMachineContext &ctx);

    StateMachineConfig m_spec;
    QHash<QString, ActionFunction> m_actionDelegates;       // 名字 → 绑定处理函数
    QHash<QString, ConditionFunction> m_conditionDelegates; // 名字 → 绑定条件函数
    QString m_current;
    bool m_started = false;
    bool m_bound = false; // bind() 是否已执行（start() 首次自动触发）
};

} // namespace gsm

// ============================================================
//  成员函数绑定宏：在 StateMachine 派生类的 bind() override 中使用
//  （参考 MITK 新版 EventStateMachine::ConnectActionsAndFunctions，
//   但用 StateMachineContext 通用上下文替代 InteractionEvent）。
//  start() 首次执行时自动调用 bind()，无需外部手动触发。
//
//  用法：
//    class MyMachine : public gsm::StateMachine {
//    public:
//        explicit MyMachine(gsm::StateMachineConfig spec)
//            : StateMachine(std::move(spec)) {}
//        void bind() override {
//            CONNECT_STATE_MACHINE_ACTION("begin_load", &MyMachine::onBeginLoad);
//            CONNECT_STATE_MACHINE_CONDITION("path_valid", &MyMachine::isPathValid);
//        }
//        void onBeginLoad(gsm::StateMachineContext &ctx) { ... }
//        bool isPathValid(const gsm::StateMachineContext &ctx) { ... }
//    };
//
//  成员函数签名：
//    动作   void  f(gsm::StateMachineContext &ctx)
//    条件   bool  f(const gsm::StateMachineContext &ctx)
// ============================================================
#define CONNECT_STATE_MACHINE_ACTION(Name, Member)                              \
    connectAction(QLatin1String(Name),                                          \
                  [this](gsm::StateMachineContext &ctx) { (this->*Member)(ctx); })

#define CONNECT_STATE_MACHINE_CONDITION(Name, Member)                           \
    connectCondition(QLatin1String(Name),                                       \
                     [this](const gsm::StateMachineContext &ctx) { return (this->*Member)(ctx); })
