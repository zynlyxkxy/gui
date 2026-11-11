#include "CommandsTab.h"
#include "StageGroup/StageGroup.h"
#include "ui_CommandsTab.h"
#include "Common/StyleSheet.h"

#include <QGridLayout>

CommandsTab::CommandsTab(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CommandsTab)
{
    ui->setupUi(this);
    ui->scrollArea->setStyleSheet(StyleSheet::scrollArea);
    ui->scrollContent->setStyleSheet("background-color:transparent;");
    ui->scrollLayout->setAlignment(Qt::AlignTop);
}

CommandsTab::~CommandsTab()
{
    delete ui;
}

void CommandsTab::buildUI(const std::vector<MctrlModule::StageInfo> &stages,
                          MCTRLClientManager *client)
{
    m_client = client;

    for (auto *g : m_groups)
    {
        delete g;
    }
    m_groups.clear();

    // 两列排布：第 i/2 行、第 i%2 列；只有 1 张卡片时跨两列占满整行
    const int kColumns = 2;
    const int n = static_cast<int>(stages.size());
    for (int i = 0; i < n; ++i)
    {
        auto *group = new StageGroup(stages[i], client, this);
        m_groups.append(group);
        if (n == 1)
            ui->scrollLayout->addWidget(group, 0, 0, 1, kColumns);
        else
            ui->scrollLayout->addWidget(group, i / kColumns, i % kColumns);
    }

    // 两列等宽
    ui->scrollLayout->setColumnStretch(0, 1);
    ui->scrollLayout->setColumnStretch(1, 1);

    // 同一行的两张卡片等高：以较高者为准（矮的那张被拉高补齐）
    // 卡片按 row=i/2, col=i%2 排布, 故 i 与 i+1 同处一行 (n==1 时已跨两列, 无需处理)
    for (int i = 0; i + 1 < n; i += kColumns)
    {
        StageGroup *a = m_groups[i];
        StageGroup *b = m_groups[i + 1];
        int h = a->desiredHeight() > b->desiredHeight()
                    ? a->desiredHeight()
                    : b->desiredHeight();
        a->setFixedHeight(h);
        b->setFixedHeight(h);
    }

    refreshTexts();
}

void CommandsTab::refreshTexts()
{
    for (auto *g : m_groups)
    {
        if (g)
        {
            g->refreshTexts();
        }
    }
}

void CommandsTab::setRpcClient(MCTRLClientManager *client)
{
    m_client = client;
    for (auto *g : m_groups)
        g->setRpcClient(client);
}

StageGroup *CommandsTab::findStage(const std::string &stageName)
{
    for (auto *g : m_groups)
    {
        if (g->stageName() == stageName)
            return g;
    }
    return nullptr;
}
