#include "StateMachine.h"

#include <spdlog/spdlog.h>

namespace gsm
{

StateMachine::StateMachine(StateMachineConfig spec) : m_spec(std::move(spec)) {}

void StateMachine::connectAction(const QString &name, ActionFunction fn)
{
    if (name.isEmpty() || !fn)
        return;
    m_actionDelegates.insert(name, std::move(fn));
}

void StateMachine::connectCondition(const QString &name, ConditionFunction fn)
{
    if (name.isEmpty() || !fn)
        return;
    m_conditionDelegates.insert(name, std::move(fn));
}

const State *StateMachine::findState(const QString &name) const
{
    for (const auto &s : m_spec.states)
    {
        if (s.name == name)
            return &s;
    }
    return nullptr;
}

bool StateMachine::runActions(const QStringList &names, StateMachineContext &ctx)
{
    bool ok = true;
    for (const auto &n : names)
    {
        auto it = m_actionDelegates.find(n);
        if (it == m_actionDelegates.end() || !it.value())
        {
            spdlog::error("[state] action '{}' not bound", n.toStdString());
            ok = false;
            continue;
        }
        it.value()(ctx);
    }
    return ok;
}

bool StateMachine::start()
{
    if (m_started)
        return true;

    // 首次启动前自动触发派生类绑定（幂等；基类为默认空实现）
    if (!m_bound)
    {
        bind();
        m_bound = true;
    }

    const State *init = findState(m_spec.initial);
    if (!init)
    {
        spdlog::error("[state] initial state '{}' not found", m_spec.initial.toStdString());
        return false;
    }

    StateMachineContext ctx;
    ctx.machine = this;
    m_current = m_spec.initial;
    m_started = true;

    if (!runActions(init->entryActions, ctx))
    {
        m_started = false;
        return false;
    }
    return true;
}

bool StateMachine::handleEvent(const QString &event, const QVariantMap &data)
{
    if (!m_started && !start())
        return false;

    const State *cur = findState(m_current);
    if (!cur)
        return false;

    // 找到当前状态上第一个匹配事件名的转移
    const Transition *tr = nullptr;
    for (const auto &t : cur->transitions)
    {
        if (t.event == event)
        {
            tr = &t;
            break;
        }
    }
    if (!tr)
        return false; // 当前状态不响应此事件 → 忽略

    StateMachineContext ctx;
    ctx.machine = this;
    ctx.event = event;
    ctx.data = data;

    // 1. 条件短路：任一不满足即不转移
    for (const auto &cname : tr->conditions)
    {
        auto it = m_conditionDelegates.find(cname);
        if (it == m_conditionDelegates.end() || !it.value())
        {
            spdlog::error("[state] condition '{}' not bound", cname.toStdString());
            return false;
        }
        if (!it.value()(ctx))
            return false;
    }

    // 2. 旧状态 exit 动作
    runActions(cur->exitActions, ctx);

    // 3. 转移动作
    runActions(tr->actions, ctx);

    // 4. 切换状态
    m_current = tr->target;

    // 5. 新状态 entry 动作
    const State *next = findState(m_current);
    if (next)
        runActions(next->entryActions, ctx);
    return true;
}

} // namespace gsm
