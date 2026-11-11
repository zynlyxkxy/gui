#ifndef COMMANDS_TAB_H
#define COMMANDS_TAB_H

#include <QWidget>
#include <vector>
#include <string>

#include "mctrl_client.h"

namespace Ui { class CommandsTab; }
class MCTRLClientManager;
class StageGroup;

class CommandsTab : public QWidget
{
    Q_OBJECT

public:
    explicit CommandsTab(QWidget *parent = nullptr);
    ~CommandsTab();

    void buildUI(const std::vector<MctrlModule::StageInfo> &stages,
                 MCTRLClientManager *client);

    void setRpcClient(MCTRLClientManager *client);

    // 语言切换：按当前平台语言重刷本 tab 内所有 StageGroup 文案
    void refreshTexts();

    StageGroup *findStage(const std::string &stageName);

private:
    Ui::CommandsTab *ui;
    MCTRLClientManager *m_client = nullptr;
    QList<StageGroup *> m_groups;
};

#endif // COMMANDS_TAB_H
