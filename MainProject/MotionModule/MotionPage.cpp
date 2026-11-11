#include "MotionPage.h"
#include "CommandsTab/CommandsTab.h"
#include "StageGroup/StageGroup.h"
#include "RPCModule/MCTRLClientManager.h"

#include "istar_ui_stylesheet.h"
#include "istar_language.h"

#include <QVBoxLayout>
#include <iostream>

using namespace ISTAR::UIStyle;

MotionPage::MotionPage(QWidget *parent)
    : QWidget(parent)
{
    // 跨线程 queued 信号需先注册元类型
    qRegisterMetaType<MctrlModule::StageObserverDataList>("MctrlModule::StageObserverDataList");

    setStyleSheet("background-color:#F3F4FA;");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    m_tabWidget = new QTabWidget(this);
    // 平台封装的 QTabWidget 样式 (istar_ui_stylesheet.h -> StyleSheet::TabWidget)
    m_tabWidget->setStyleSheet(StyleSheet::getInstance()->TabWidget + StyleSheet::getInstance()->fontStyle_bold_12);
    layout->addWidget(m_tabWidget);

    registerCallbacks();
    discoverStages();
}

MotionPage::~MotionPage()
{

}

void MotionPage::registerCallbacks()
{
    // MCTRLClientManager 在 Initialize 时已注册 observer 回调并 emit 信号;
    // 这里直接连信号路由到对应 StageGroup, 不再像旧代码那样逐个 RegisterCallback。
    // RPC 回调线程 emit -> queued 信号 -> GUI 主线程执行, 跨线程安全。
    MCTRLClientManager *mc = MCTRLClientManager::getInstance();
    auto route = [this](const MctrlModule::StageObserverDataList &list)
    {
        // 一次推全部 stage 的列表, 逐个路由到对应 server tab 下的 StageGroup
        for (const MctrlModule::StageObserverData &data : list.items)
        {
            for (CommandsTab *t : m_tabs)
            {
                StageGroup *group = t->findStage(data.stageName);
                if (group)
                {
                    emit group->sig_ObserverReceived(data);
                    break;
                }
            }
        }
    };
    connect(mc, &MCTRLClientManager::sig_StageObserverData, this, route);
}

void MotionPage::discoverStages()
{
    if (stagesDiscovered)
    {
        return;
    }

    MCTRLClientManager *mc = MCTRLClientManager::getInstance();

    // mctrl 一个 client 可管理多个 motion server, 每个 server 建一个 tab,
    // 所有 tab 共用同一个 MCTRLClientManager (命令按 stageName 路由)
    MctrlModule::MotionServerInfoList list;
    if (mc->getClient()->MotionGetServerInfo(list) != ISTAR_RESULT_SUCCESS || list.items.empty())
    {
        std::cout << "[Motion] no motion server" << std::endl;
        return;
    }

    int index = 0;
    for (const MctrlModule::ServerInfo &server : list.items)
    {
        std::string title = server.systemName;
        if (title.empty())
        {
            title = "server" + std::to_string(index);
        }
        std::cout << "[Motion:" << title << "] Discovered " << server.stages.size() << " stage(s)" << std::endl;

        CommandsTab *tab = new CommandsTab(m_tabWidget);
        tab->buildUI(server.stages, mc);
        tab->setRpcClient(mc);
        m_tabWidget->addTab(tab, QString::fromStdString(Tr(title)));
        m_tabTitles.push_back(title);
        m_tabs.push_back(tab);
        index++;
    }

    stagesDiscovered = true;
    std::cout << "[Motion] Stages ready" << std::endl;
}

void MotionPage::refreshTexts()
{
    const int tabCount = m_tabWidget->count();
    for (size_t i = 0; i < m_tabs.size(); ++i)
    {
        if (static_cast<int>(i) < tabCount && i < m_tabTitles.size())
        {
            m_tabWidget->setTabText(static_cast<int>(i),
                                    QString::fromStdString(Tr(m_tabTitles[i])));
        }
        m_tabs[i]->refreshTexts();
    }
}
