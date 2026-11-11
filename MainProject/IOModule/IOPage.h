#ifndef IOPAGE_H
#define IOPAGE_H

#include <QWidget>
#include "Common/UiTranslator.h"
#include "mctrl_client.h"   // MctrlModule::IoServerInfo（缓存全量 IO 列表）

class IOGroup;

// IO 页面: 直接编译进 GUI_WM.exe(非 DLL 加载)。
// 由 io_template 工程的 MainWindow 移植而来, 改为 QWidget 基类以便嵌入 FunctionArea 页面。
//
// 连接与回调: 不再自持 MCTRL_Client_Inherit, 统一走全局单例 MCTRLClientManager
// (由 mainwindow 启动时 Initialize)。observer 回调由 MCTRLClientManager 统一注册并 emit 信号,
// 本页构造函数把 sig_IoObserverData 连到 IOGroup;
// IO 变量列表通过 IOGetServerInfo 动态发现（控制层返回全量），本页按用户权限裁剪后交给 IOGroup::buildTabs。
class IOPage : public QWidget
{
    Q_OBJECT

public:
    explicit IOPage(QWidget *parent = nullptr);
    ~IOPage();
    void refreshTexts();
    void setSuperAdmin(bool superAdmin);   // 权限变化：超级管理员看全量，其余用户过滤内部号段

private:
    void discoverIo();               // 从 MCTRLClientManager 获取全量 IO 列表并构建 tab
    void rebuildIoView();            // 按当前权限用缓存的全量列表重建 tab
    void registerCallbacks();        // 连接 MCTRLClientManager 的 IO observer 信号到 IOGroup

    IOGroup *m_ioGroup = nullptr;    // IO 分组控件 (AI/DI/AO/DO/Setting/Buffer)
    bool m_ioDiscovered = false;     // 全量 IO 列表是否已拉取
    bool m_superAdmin = false;       // 当前用户是否超级管理员 (level 10)
    MctrlModule::IoServerInfo m_fullIo;   // 控制层返回的全量 IO 列表（未过滤）
};

#endif // IOPAGE_H
