#include "IOModule/IOGroup/IOGroup.h"
#include "IOModule/IOWidget/DIWidget.h"
#include "IOModule/IOWidget/DOWidget.h"
#include "IOModule/IOWidget/AIWidget.h"
#include "IOModule/IOWidget/AOWidget.h"
#include "IOModule/IOWidget/SettingWidget.h"
#include "IOModule/IOWidget/BufferWidget.h"
#include "RPCModule/MCTRLClientManager.h"

#include "istar_ui_stylesheet.h"

#include <QScrollArea>
#include <QLabel>

using namespace ISTAR::UIStyle;


static inline bool isPlc(int varId)
{
    return (varId / 1000) % 2 == 0;
}

template<typename T>
static QList<int> extractIds(const std::vector<T> &entries)
{
    QList<int> ids;
    for (const auto &e : entries)
    {
        ids.append(e.id);
    }
    return ids;
}

template<typename T>
static QMap<int, QString> extractNameMap(const std::vector<T> &entries)
{
    QMap<int, QString> map;
    for (const auto &e : entries)
    {
        map.insert(e.id, QString::fromStdString(e.name));
    }
    return map;
}

static QScrollArea *makeScrollArea(QWidget *parent)
{
    auto *area = new QScrollArea(parent);
    area->setStyleSheet("QWidget{background-color: transparent;}");
    area->setWidgetResizable(true);
    area->setWidget(new QWidget());
    return area;
}

static QLabel *makeHeader(const QString &text)
{
    auto *lbl = new QLabel(text);
    lbl->setStyleSheet("font-weight: bold; font-size: 14px; color: #4C70FA;"
                       "border-bottom: 2px solid #4C70FA;"
                       "padding-bottom: 2px;");
    lbl->setMaximumHeight(24);
    return lbl;
}

// ═══════════════════════════════════════════════════════════════
// 两列 tab — ACS / PLC
// ═══════════════════════════════════════════════════════════════

template<typename TWidget>
static void buildTwoColTab(const QList<int> &varIds,
                            const QMap<int, QString> &nameMap,
                            QGridLayout *layout,
                            QMap<int, TWidget *> &widgetMap)
{
    layout->addWidget(makeHeader("PLC"), 0, 0);
    layout->addWidget(makeHeader("ACS"), 0, 1);

    for (int id : varIds)
    {
        int col = isPlc(id) ? 0 : 1;
        int row = (id % 1000) + 1;

        TWidget *w = new TWidget(id, nameMap.value(id, QString::number(id)));
        w->setFixedHeight(50);
        layout->addWidget(w, row, col);
        widgetMap.insert(id, w);
    }
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(2, 0);
}

// ═══════════════════════════════════════════════════════════════
// 单列 tab
// ═══════════════════════════════════════════════════════════════

template<typename TWidget>
static void buildSingleColTab(const QList<int> &varIds,
                               const QMap<int, QString> &nameMap,
                               QGridLayout *layout,
                               QMap<int, TWidget *> &widgetMap)
{
    for (int i = 0; i < varIds.size(); ++i)
    {
        int id = varIds[i];
        TWidget *w = new TWidget(id, nameMap.value(id, QString::number(id)));
        w->setFixedHeight(50);
        layout->addWidget(w, i, 0);
        widgetMap.insert(id, w);
    }
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 1);
}

// Setting: 左列 Bool，右列 Double
static void buildSettingGrid(const QList<int> &dblIds,
                              const QMap<int, QString> &dblNameMap,
                              const QList<int> &boolIds,
                              const QMap<int, QString> &boolNameMap,
                              QGridLayout *layout,
                              QMap<int, SettingWidget *> &dblMap,
                              QMap<int, SettingWidget *> &boolMap)
{
    layout->addWidget(makeHeader("Bool"), 0, 0);
    layout->addWidget(makeHeader("Double"), 0, 1);

    int rowBool = 1;
    for (int i = 0; i < boolIds.size(); ++i)
    {
        int id = boolIds[i];
        SettingWidget *w = new SettingWidget(id, boolNameMap.value(id, QString::number(id)),
                                              false);
        w->setFixedHeight(50);
        layout->addWidget(w, rowBool + i, 0);
        boolMap.insert(id, w);
    }

    int rowDbl = 1;
    for (int i = 0; i < dblIds.size(); ++i)
    {
        int id = dblIds[i];
        SettingWidget *w = new SettingWidget(id, dblNameMap.value(id, QString::number(id)),
                                              true);
        w->setFixedHeight(50);
        layout->addWidget(w, rowDbl + i, 1);
        dblMap.insert(id, w);
    }
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 1);
}

// ═══════════════════════════════════════════════════════════════
// IOGroup
// ═══════════════════════════════════════════════════════════════

IOGroup::IOGroup(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    m_tabWidget = new QTabWidget(this);
    // 平台封装的 QTabWidget 样式 (istar_ui_stylesheet.h -> StyleSheet::TabWidget)
    m_tabWidget->setStyleSheet(StyleSheet::getInstance()->TabWidget + StyleSheet::getInstance()->fontStyle_bold_12);
    mainLayout->addWidget(m_tabWidget);

    // 固定 6 个空 tab，连上 RPC 后由 buildTabs 填充
    m_tabWidget->addTab(makeScrollArea(this), QStringLiteral("AI"));
    m_tabWidget->addTab(makeScrollArea(this), QStringLiteral("DI"));
    m_tabWidget->addTab(makeScrollArea(this), QStringLiteral("AO"));
    m_tabWidget->addTab(makeScrollArea(this), QStringLiteral("DO"));
    m_tabWidget->addTab(makeScrollArea(this), QStringLiteral("Setting"));
    m_tabWidget->addTab(makeScrollArea(this), QStringLiteral("Buffer"));

    connect(this, &IOGroup::sig_IoObserverReceived,
            this, &IOGroup::onIoObserverData, Qt::QueuedConnection);
}

IOGroup::~IOGroup()
{
}

// ═══════════════════════════════════════════════════════════════
// buildTabs — 从 ServerInfo 填充内容
// ═══════════════════════════════════════════════════════════════

void IOGroup::buildTabs(const MctrlModule::IoServerInfo &info)
{
    // 删除旧页面：QTabWidget::clear() 不销毁页面控件，权限切换重建时旧控件会堆积。
    while (m_tabWidget->count() > 0)
    {
        QWidget *page = m_tabWidget->widget(0);
        m_tabWidget->removeTab(0);
        delete page;
    }

    m_aiWidgets.clear();
    m_aoWidgets.clear();
    m_diWidgets.clear();
    m_doWidgets.clear();
    m_settingDoubleWidgets.clear();
    m_settingBoolWidgets.clear();
    m_bufferWidgets.clear();

    // AI
    {
        QScrollArea *area = makeScrollArea(this);
        QGridLayout *lay = new QGridLayout(area->widget());
        lay->setSpacing(0);
        lay->setHorizontalSpacing(20);
        lay->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        auto nameMap = extractNameMap(info.io.AI);
        buildTwoColTab<AIWidget>(extractIds(info.io.AI), nameMap, lay, m_aiWidgets);

        m_tabWidget->addTab(area, QStringLiteral("AI"));
    }

    // DI
    {
        QScrollArea *area = makeScrollArea(this);
        QGridLayout *lay = new QGridLayout(area->widget());
        lay->setSpacing(0);
        lay->setHorizontalSpacing(20);
        lay->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        auto nameMap = extractNameMap(info.io.DI);
        buildTwoColTab<DIWidget>(extractIds(info.io.DI), nameMap, lay, m_diWidgets);

        m_tabWidget->addTab(area, QStringLiteral("DI"));
    }

    // AO
    {
        QScrollArea *area = makeScrollArea(this);
        QGridLayout *lay = new QGridLayout(area->widget());
        lay->setSpacing(0);
        lay->setHorizontalSpacing(20);
        lay->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        auto nameMap = extractNameMap(info.io.AO);
        buildTwoColTab<AOWidget>(extractIds(info.io.AO), nameMap, lay, m_aoWidgets);

        m_tabWidget->addTab(area, QStringLiteral("AO"));
    }

    // DO
    {
        QScrollArea *area = makeScrollArea(this);
        QGridLayout *lay = new QGridLayout(area->widget());
        lay->setSpacing(0);
        lay->setHorizontalSpacing(20);
        lay->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        auto nameMap = extractNameMap(info.io.DO);
        buildTwoColTab<DOWidget>(extractIds(info.io.DO), nameMap, lay, m_doWidgets);

        m_tabWidget->addTab(area, QStringLiteral("DO"));
    }

    // Setting — Double + Bool 合在一个 tab
    {
        QScrollArea *area = makeScrollArea(this);
        QGridLayout *lay = new QGridLayout(area->widget());
        lay->setSpacing(0);
        lay->setHorizontalSpacing(20);
        lay->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        auto dblNameMap = extractNameMap(info.io.SettingDouble);
        auto boolNameMap = extractNameMap(info.io.SettingBool);
        buildSettingGrid(extractIds(info.io.SettingDouble), dblNameMap,
                         extractIds(info.io.SettingBool), boolNameMap,
                         lay, m_settingDoubleWidgets, m_settingBoolWidgets);

        m_tabWidget->addTab(area, QStringLiteral("Setting"));
    }

    // Buffer
    {
        QScrollArea *area = makeScrollArea(this);
        QGridLayout *lay = new QGridLayout(area->widget());
        lay->setSpacing(0);
        lay->setHorizontalSpacing(20);
        lay->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        lay->addWidget(makeHeader("Buffer"), 0, 0);

        auto nameMap = extractNameMap(info.io.Buffer);
        auto ids = extractIds(info.io.Buffer);
        for (int i = 0; i < ids.size(); ++i)
        {
            int id = ids[i];
            BufferWidget *w = new BufferWidget(id, nameMap.value(id, QString::number(id)));
            w->setFixedHeight(50);
            lay->addWidget(w, i + 1, 0);
            m_bufferWidgets.insert(id, w);
        }
        lay->setColumnStretch(0, 1);
        lay->setColumnStretch(1, 1);

        m_tabWidget->addTab(area, QStringLiteral("Buffer"));
    }

    if (m_client)
    {
        connectSignals();
    }

    // 本函数会整页重建（权限变化时 IOPage::rebuildIoView 会再调一次），新控件的名字是
    // 服务端直出的英文原稿、且没被 UiTranslator 捕获过 —— 必须在这里立刻刷一次翻译，
    // 否则重建后 IO 页会退回英文，直到用户手动切换语言才恢复。
    m_uiTranslator.refresh(this);
}

// ═══════════════════════════════════════════════════════════════
// setRpcClient
// ═══════════════════════════════════════════════════════════════

void IOGroup::setRpcClient(MCTRLClientManager *client)
{
    m_client = client;
    connectSignals();
}

void IOGroup::connectSignals()
{
    for (auto it = m_aoWidgets.begin(); it != m_aoWidgets.end(); ++it)
    {
        connect(it.value(), &AOWidget::setAo, this, [this](int varId, double value)
        {
            if (!m_client)
            {
                return;
            }
            MctrlModule::IoAoInput in;
            in.varId = varId;
            in.value = value;
            in.verify = false;
            in.expectedValue = value;
            m_client->getClient()->IOSetAo(in);
        });
    }

    for (auto it = m_doWidgets.begin(); it != m_doWidgets.end(); ++it)
    {
        connect(it.value(), &DOWidget::setDo, this, [this](int varId, bool value)
        {
            if (!m_client)
            {
                return;
            }
            MctrlModule::IoDoInput in;
            in.varId = varId;
            in.value = value;
            in.verify = false;
            in.expectedValue = value;
            m_client->getClient()->IOSetDo(in);
        });
    }

    for (auto it = m_settingDoubleWidgets.begin(); it != m_settingDoubleWidgets.end(); ++it)
    {
        connect(it.value(), &SettingWidget::setSettingDouble, this,
                [this](int varId, double value)
        {
            if (!m_client)
            {
                return;
            }
            MctrlModule::IoSettingDoubleInput in;
            in.varId = varId;
            in.value = value;
            m_client->getClient()->IOSetSettingDouble(in);
        });
    }

    for (auto it = m_settingBoolWidgets.begin(); it != m_settingBoolWidgets.end(); ++it)
    {
        connect(it.value(), &SettingWidget::setSettingBool, this,
                [this](int varId, bool value)
        {
            if (!m_client)
            {
                return;
            }
            MctrlModule::IoSettingBoolInput in;
            in.varId = varId;
            in.value = value;
            m_client->getClient()->IOSetSettingBool(in);
        });
    }

    for (auto it = m_bufferWidgets.begin(); it != m_bufferWidgets.end(); ++it)
    {
        connect(it.value(), &BufferWidget::startBuffer, this, [this](int varId)
        {
            if (!m_client)
            {
                return;
            }
            MctrlModule::IoVarInput in;
            in.varId = varId;
            m_client->getClient()->IOStartBuffer(in);
        });
        connect(it.value(), &BufferWidget::stopBuffer, this, [this](int varId)
        {
            if (!m_client)
            {
                return;
            }
            MctrlModule::IoVarInput in;
            in.varId = varId;
            m_client->getClient()->IOStopBuffer(in);
        });
    }
}

// ═══════════════════════════════════════════════════════════════
// Observer 数据分发
// ═══════════════════════════════════════════════════════════════

void IOGroup::onIoObserverData(const MctrlModule::IoObserverData &data)
{
    // 新协议一次推 ACS + PLC 全部缓存, 分别分发到对应 widget
    dispatchAcs(data.acs);
    dispatchPlc(data.plc);
}

void IOGroup::dispatchAcs(const MctrlModule::AcsObserverData &data)
{
    for (const auto &e : data.AI)
    {
        if (m_aiWidgets.contains(e.varId))
        {
            m_aiWidgets[e.varId]->display(e.value);
        }
    }
    for (const auto &e : data.DI)
    {
        if (m_diWidgets.contains(e.varId))
        {
            m_diWidgets[e.varId]->display(e.value);
        }
    }
    for (const auto &e : data.AO)
    {
        if (m_aoWidgets.contains(e.varId))
        {
            m_aoWidgets[e.varId]->display(e.value);
        }
    }
    for (const auto &e : data.DO)
    {
        if (m_doWidgets.contains(e.varId))
        {
            m_doWidgets[e.varId]->display(e.value);
        }
    }
    for (const auto &e : data.Buf)
    {
        if (m_bufferWidgets.contains(e.varId))
        {
            m_bufferWidgets[e.varId]->display(e.value);
        }
    }
}

void IOGroup::dispatchPlc(const MctrlModule::PlcObserverData &data)
{
    for (const auto &e : data.AI)
    {
        if (m_aiWidgets.contains(e.varId))
        {
            m_aiWidgets[e.varId]->display(e.value);
        }
    }
    for (const auto &e : data.DI)
    {
        if (m_diWidgets.contains(e.varId))
        {
            m_diWidgets[e.varId]->display(e.value);
        }
    }
    for (const auto &e : data.AO)
    {
        if (m_aoWidgets.contains(e.varId))
        {
            m_aoWidgets[e.varId]->display(e.value);
        }
    }
    for (const auto &e : data.DO)
    {
        if (m_doWidgets.contains(e.varId))
        {
            m_doWidgets[e.varId]->display(e.value);
        }
    }
}

void IOGroup::refreshTexts()
{
    m_uiTranslator.refresh(this);
}
