#include "StateMachineConfig.h"

#include <QFile>
#include <QXmlStreamReader>

namespace gsm
{

namespace
{
QString attr(QXmlStreamReader &r, const char *name)
{
    return r.attributes().value(QLatin1String(name)).toString();
}

void fail(QString *error, const QString &msg)
{
    if (error)
        *error = msg;
}
} // namespace

bool StateMachineConfig::fromXml(const QByteArray &xml, QString *error)
{
    // 每次解析前清空，保证可重复调用
    name.clear();
    initial.clear();
    states.clear();

    QXmlStreamReader r(xml);
    bool rootSeen = false;
    State *curState = nullptr;
    Transition *curTransition = nullptr;

    while (!r.atEnd())
    {
        const QXmlStreamReader::TokenType tok = r.readNext();
        if (tok != QXmlStreamReader::StartElement)
            continue;

        const QString tag = r.name().toString();
        if (tag == QLatin1String("stateMachine"))
        {
            rootSeen = true;
            name = attr(r, "name");
            initial = attr(r, "initial");
            curState = nullptr;
            curTransition = nullptr;
        }
        else if (tag == QLatin1String("state"))
        {
            if (!rootSeen)
            {
                fail(error, "state before <stateMachine>");
                return false;
            }
            State s;
            s.name = attr(r, "name");
            if (s.name.isEmpty())
            {
                fail(error, "<state> missing name");
                return false;
            }
            states.push_back(std::move(s));
            curState = &states.back();
            curTransition = nullptr;
        }
        else if (tag == QLatin1String("transition"))
        {
            if (!curState)
            {
                fail(error, "<transition> outside <state>");
                return false;
            }
            Transition t;
            t.event = attr(r, "event");
            t.target = attr(r, "target");
            if (t.event.isEmpty() || t.target.isEmpty())
            {
                fail(error, QString("<transition> in '%1' missing event/target")
                                .arg(curState->name));
                return false;
            }
            curState->transitions.push_back(std::move(t));
            curTransition = &curState->transitions.back();
        }
        else if (tag == QLatin1String("condition"))
        {
            if (!curTransition)
            {
                fail(error, "<condition> outside <transition>");
                return false;
            }
            const QString n = attr(r, "name");
            if (n.isEmpty())
            {
                fail(error, "<condition> missing name");
                return false;
            }
            curTransition->conditions << n;
        }
        else if (tag == QLatin1String("action"))
        {
            if (!curTransition)
            {
                fail(error, "<action> outside <transition>");
                return false;
            }
            const QString n = attr(r, "name");
            if (n.isEmpty())
            {
                fail(error, "<action> missing name");
                return false;
            }
            curTransition->actions << n;
        }
        else if (tag == QLatin1String("entryAction"))
        {
            if (!curState)
            {
                fail(error, "<entryAction> outside <state>");
                return false;
            }
            const QString n = attr(r, "name");
            if (n.isEmpty())
            {
                fail(error, "<entryAction> missing name");
                return false;
            }
            curState->entryActions << n;
        }
        else if (tag == QLatin1String("exitAction"))
        {
            if (!curState)
            {
                fail(error, "<exitAction> outside <state>");
                return false;
            }
            const QString n = attr(r, "name");
            if (n.isEmpty())
            {
                fail(error, "<exitAction> missing name");
                return false;
            }
            curState->exitActions << n;
        }
    }

    if (r.hasError())
    {
        fail(error, QString("XML error: %1 (line %2)")
                        .arg(r.errorString())
                        .arg(r.lineNumber()));
        return false;
    }
    if (!rootSeen)
    {
        fail(error, "not a <stateMachine> XML");
        return false;
    }
    if (states.isEmpty())
    {
        fail(error, "stateMachine has no <state>");
        return false;
    }

    // initial 缺省 → 第一个状态；若显式给出但不存在则报错
    if (initial.isEmpty())
        initial = states.first().name;
    else
    {
        bool found = false;
        for (const auto &s : states)
        {
            if (s.name == initial)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            fail(error, QString("initial state '%1' not found").arg(initial));
            return false;
        }
    }

    // 校验所有 transition.target 均存在
    for (const auto &s : states)
    {
        for (const auto &t : s.transitions)
        {
            bool found = false;
            for (const auto &d : states)
            {
                if (d.name == t.target)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                fail(error, QString("transition '%1' -> target '%2' not found")
                                .arg(t.event, t.target));
                return false;
            }
        }
    }
    return true;
}

bool StateMachineConfig::fromFile(const QString &path, QString *error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        fail(error, QString("cannot open %1: %2").arg(path, f.errorString()));
        return false;
    }
    return fromXml(f.readAll(), error);
}

} // namespace gsm
