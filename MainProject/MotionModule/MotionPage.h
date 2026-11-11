#ifndef MOTION_PAGE_H
#define MOTION_PAGE_H

#include <QWidget>
#include <QTabWidget>
#include <vector>
#include <string>
#include "RPCModule/MCTRLClientManager.h"

class CommandsTab;

// MotionStage 页面: 直接编译进 GUI_WM.exe(非 DLL 加载)。
// 由 motionstage_template 的 MainWindow 移植而来, 改为 QWidget 基类以便嵌入 FunctionArea 页面。
//
// 连接与回调: 统一走全局单例 MCTRLClientManager (由 mainwindow 启动时 Initialize),
// observer 回调由 MCTRLClientManager 统一注册并 emit 信号, 本页按 stageName 路由到各 StageGroup。
// 一个 client 可管理多个 motion server, 每个 server 对应一个 CommandsTab。
class MotionPage : public QWidget
{
    Q_OBJECT

public:
    explicit MotionPage(QWidget *parent = nullptr);
    ~MotionPage();

    // 语言切换：重刷各 tab 标题 + 各 StageGroup 文案
    void refreshTexts();

private:
    void discoverStages();           // 从 MCTRLClientManager 获取 motion server, 每个 server 建一个 tab
    void registerCallbacks();        // 连接 MCTRLClientManager 的 stage observer 信号并按 stageName 路由

    QTabWidget *m_tabWidget = nullptr;    // server tab 容器
    std::vector<CommandsTab *> m_tabs;    // 一个 client 下可能有多个 server, 每个 server 一个 tab
    std::vector<std::string> m_tabTitles; // 与 m_tabs 对齐的原始标题 key(供 refreshTexts 重译)
    bool stagesDiscovered = false;        // 是否已发现 stage
};

#endif // MOTION_PAGE_H
