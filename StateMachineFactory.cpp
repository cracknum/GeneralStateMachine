#include "StateMachineFactory.h"

namespace gsm
{

StateMachineFactory &StateMachineFactory::instance()
{
    static StateMachineFactory inst;
    return inst;
}

std::unique_ptr<StateMachine> StateMachineFactory::createFromXml(const QByteArray &xml, QString *error) const
{
    StateMachineConfig spec;
    QString parseErr;
    if (!spec.fromXml(xml, &parseErr))
    {
        if (error)
            *error = "XML parse failed: " + parseErr;
        return nullptr;
    }
    return std::make_unique<StateMachine>(std::move(spec));
}

std::unique_ptr<StateMachine> StateMachineFactory::createFromFile(const QString &path, QString *error) const
{
    StateMachineConfig spec;
    QString parseErr;
    if (!spec.fromFile(path, &parseErr))
    {
        if (error)
            *error = "load failed: " + parseErr;
        return nullptr;
    }
    return std::make_unique<StateMachine>(std::move(spec));
}

} // namespace gsm
