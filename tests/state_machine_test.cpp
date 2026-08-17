// ============================================================
//  通用状态机单元测试
//  覆盖：
//    - XML 解析（结构/错误场景）
//    - 从文件读取（fromFile / createFromFile，含文件缺失报错）
//    - 事件驱动转移（匹配/忽略）
//    - 条件短路（条件不满足不转移）
//    - entry/exit/transition 动作执行顺序
//    - 宿主成员函数绑定（虚 bind() 由 start() 自动触发，CONNECT_STATE_MACHINE_ACTION/CONDITION）
//    - 未 start 自动启动 / initial 缺省取第一个状态
// ============================================================
#include "StateMachine.h"
#include "StateMachineFactory.h"

#include <gtest/gtest.h>

#include <QFile>
#include <QTemporaryDir>

#include <utility>

namespace
{
const char *kDemoXml = R"xml(
<stateMachine name="demo" initial="IDLE">
    <state name="IDLE">
        <entryAction name="log_entry"/>
        <transition event="start" target="RUNNING">
            <condition name="gate_open"/>
            <action name="record"/>
        </transition>
    </state>
    <state name="RUNNING">
        <exitAction name="log_exit"/>
        <transition event="stop" target="DONE">
            <action name="capture_ctx"/>
        </transition>
    </state>
    <state name="DONE">
        <entryAction name="log_entry"/>
    </state>
</stateMachine>
)xml";

// 无效 target（state "MISSING" 不存在）→ 解析失败
const char *kBadTargetXml = R"xml(
<stateMachine name="bad" initial="IDLE">
    <state name="IDLE">
        <transition event="go" target="MISSING"/>
    </state>
</stateMachine>
)xml";

// 无 initial → 缺省取第一个状态
const char *kNoInitialXml = R"xml(
<stateMachine name="noinit">
    <state name="A">
        <transition event="go" target="B"/>
    </state>
    <state name="B"/>
</stateMachine>
)xml";

// ---- 宿主成员函数绑定模式（MITK 新版 EventStateMachine 风格）----
// 动作/条件不独立成类，在 StateMachine 派生类里用
// CONNECT_STATE_MACHINE_ACTION / CONNECT_STATE_MACHINE_CONDITION 绑定成员函数。
class DemoMachine : public gsm::StateMachine
{
public:
    explicit DemoMachine(gsm::StateMachineConfig spec) : gsm::StateMachine(std::move(spec)) {}

    void bind() override
    {
        CONNECT_STATE_MACHINE_ACTION("log_entry", &DemoMachine::logEntry);
        CONNECT_STATE_MACHINE_ACTION("log_exit", &DemoMachine::logExit);
        CONNECT_STATE_MACHINE_ACTION("record", &DemoMachine::record);
        CONNECT_STATE_MACHINE_ACTION("capture_ctx", &DemoMachine::captureCtx);
        CONNECT_STATE_MACHINE_CONDITION("gate_open", &DemoMachine::gateOpen);
    }

    void logEntry(gsm::StateMachineContext &ctx) { calls << "entry:" + ctx.machine->currentState(); }
    void logExit(gsm::StateMachineContext &ctx) { calls << "exit:" + ctx.machine->currentState(); }
    void record(gsm::StateMachineContext &) { calls << "record"; }
    void captureCtx(gsm::StateMachineContext &ctx)
    {
        calls << "capture:" + ctx.event;
        lastData = ctx.data;
    }
    bool gateOpen(const gsm::StateMachineContext &) { return gate; }

    bool gate = true;
    QVariantMap lastData;
    QStringList calls;
};
} // namespace

TEST(StateMachineTest, ParseXmlOk)
{
    gsm::StateMachineConfig spec;
    ASSERT_TRUE(spec.fromXml(kDemoXml));
    EXPECT_EQ(spec.name, "demo");
    EXPECT_EQ(spec.initial, "IDLE");
    ASSERT_EQ(spec.states.size(), 3);
    EXPECT_EQ(spec.states[0].name, "IDLE");
    ASSERT_EQ(spec.states[0].transitions.size(), 1);
    EXPECT_EQ(spec.states[0].transitions[0].event, "start");
    EXPECT_EQ(spec.states[0].transitions[0].target, "RUNNING");
    ASSERT_EQ(spec.states[0].transitions[0].conditions.size(), 1);
    EXPECT_EQ(spec.states[0].transitions[0].conditions[0], "gate_open");
    ASSERT_EQ(spec.states[0].transitions[0].actions.size(), 1);
    EXPECT_EQ(spec.states[0].transitions[0].actions[0], "record");
    EXPECT_EQ(spec.states[0].entryActions, QStringList{"log_entry"});
    EXPECT_EQ(spec.states[1].exitActions, QStringList{"log_exit"});
}

TEST(StateMachineTest, ParseXmlBadTarget)
{
    gsm::StateMachineConfig spec;
    EXPECT_FALSE(spec.fromXml(kBadTargetXml));
}

TEST(StateMachineTest, EventDrivenTransitionAndOrder)
{
    gsm::StateMachineConfig spec;
    ASSERT_TRUE(spec.fromXml(kDemoXml));

    DemoMachine sm(std::move(spec));

    // bind 由 start() 自动触发，无需手动调用
    ASSERT_TRUE(sm.start());
    EXPECT_EQ(sm.currentState(), "IDLE");
    EXPECT_EQ(sm.calls, QStringList{"entry:IDLE"});

    // 条件不满足 → 不转移，动作不执行
    sm.gate = false;
    EXPECT_FALSE(sm.handleEvent("start"));
    EXPECT_EQ(sm.currentState(), "IDLE");
    EXPECT_EQ(sm.calls, QStringList{"entry:IDLE"});

    // 条件满足 → IDLE --start--> RUNNING（transition 动作 record）
    sm.gate = true;
    EXPECT_TRUE(sm.handleEvent("start"));
    EXPECT_EQ(sm.currentState(), "RUNNING");
    auto str1 = QStringList{"entry:IDLE", "record"};
    EXPECT_EQ(sm.calls, str1);

    // 当前状态不响应的事件 → 忽略
    EXPECT_FALSE(sm.handleEvent("no_such_event"));
    EXPECT_EQ(sm.currentState(), "RUNNING");

    // RUNNING --stop--> DONE：exit → transition(capture_ctx) → entry
    EXPECT_TRUE(sm.handleEvent("stop", {{"path", "/x.dcm"}}));
    EXPECT_EQ(sm.currentState(), "DONE");
    auto str2 = QStringList{"entry:IDLE", "record", "exit:RUNNING", "capture:stop", "entry:DONE"};
    EXPECT_EQ(sm.calls, str2);
    EXPECT_EQ(sm.lastData.value("path").toString(), "/x.dcm");
}

TEST(StateMachineTest, EventBeforeStartAutoStarts)
{
    gsm::StateMachineConfig spec;
    ASSERT_TRUE(spec.fromXml(kDemoXml));

    DemoMachine sm(std::move(spec));

    // 未显式 start()，事件自动先进入 initial 再转移
    EXPECT_TRUE(sm.handleEvent("start"));
    EXPECT_EQ(sm.currentState(), "RUNNING");
    auto str1 = QStringList{"entry:IDLE", "record"};
    EXPECT_EQ(sm.calls, str1);
}

TEST(StateMachineTest, InitialDefaultsToFirstState)
{
    auto sm = gsm::StateMachineFactory::instance().createFromXml(kNoInitialXml);
    ASSERT_NE(sm, nullptr);
    ASSERT_TRUE(sm->start());
    EXPECT_EQ(sm->currentState(), "A");
    EXPECT_TRUE(sm->handleEvent("go"));
    EXPECT_EQ(sm->currentState(), "B");
}

TEST(StateMachineTest, FromFileReadsXml)
{
    // 写临时 XML 文件
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString path = dir.filePath("demo.xml");
    {
        QFile f(path);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write(kDemoXml);
        f.close();
    }

    // StateMachineConfig::fromFile 直接读文件，无需外部先读取
    gsm::StateMachineConfig spec;
    ASSERT_TRUE(spec.fromFile(path));
    EXPECT_EQ(spec.name, "demo");
    EXPECT_EQ(spec.initial, "IDLE");

    // 工厂一步到位：文件 → 引擎；XML 引用的动作/条件事后绑定（MITK 新版风格）
    auto sm = gsm::StateMachineFactory::instance().createFromFile(path);
    ASSERT_NE(sm, nullptr);
    sm->connectAction("log_entry", [](gsm::StateMachineContext &) {});
    sm->connectAction("log_exit", [](gsm::StateMachineContext &) {});
    sm->connectAction("record", [](gsm::StateMachineContext &) {});
    sm->connectAction("capture_ctx", [](gsm::StateMachineContext &) {});
    sm->connectCondition("gate_open", [](const gsm::StateMachineContext &) { return true; });

    ASSERT_TRUE(sm->start());
    EXPECT_EQ(sm->currentState(), "IDLE");
    EXPECT_TRUE(sm->handleEvent("start"));
    EXPECT_EQ(sm->currentState(), "RUNNING");
    EXPECT_TRUE(sm->handleEvent("stop"));
    EXPECT_EQ(sm->currentState(), "DONE");
}

TEST(StateMachineTest, FromFileMissingReportsError)
{
    QString err;
    gsm::StateMachineConfig spec;
    EXPECT_FALSE(spec.fromFile("no_such_file.xml", &err));
    EXPECT_FALSE(err.isEmpty());

    err.clear();
    auto sm = gsm::StateMachineFactory::instance().createFromFile("no_such_file.xml", &err);
    EXPECT_EQ(sm, nullptr);
    EXPECT_FALSE(err.isEmpty());
}
