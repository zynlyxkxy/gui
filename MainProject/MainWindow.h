/***********************************************************************
 * Copyright (C) 2024 ISTAR.
 *
 * File Name:        mainwindow.h
 * Description:      ISTAR GUI Framework Example - UIMA Style Main Window
 *                   This is a simplified version of UIMA MainWindow using
 *                   the ISTAR GUI Framework components.
 * Version:          V0.4.0
 * Author:           iStar Platform
 * Date:             2025-07-24
 ***********************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QVector>
#include <QPair>
#include <QMap>
#include <QStringList>
#include <memory>
#include <string>
#include <QLabel>

#include <set>

#include "ua_client.h"
#include "ua_define.h"
#include "mctrl_client.h"   // MctrlModule::MctrlStatus（onStatusChanged 形参）

namespace Ui
{
    class MainWindow;
}

namespace ISTAR
{
    namespace Framework
    {
        class FunctionArea;
        class NaviButton;
    }
    namespace GUI
    {
        class LoginDialog;
    }
}

class ProcessPage;   // 前向声明：仅持指针（Job → Process 页语言刷新）
class MotionPage;    // 前向声明：仅持指针（System → Motion 页语言刷新）
class ConfigPage;
class ContinuousBondPage;
class AutoProcessPage;
class SystemStatusPage;   // 前向声明：仅持指针（Job → Status 页语言刷新）
class MaterialPage;
class IOPage;
class CameraCalibPage;   // 前向声明：仅持指针（System → 相机标定页语言刷新）
/**
 * @brief UIMA-style main window using ISTAR GUI Framework
 *
 * This example demonstrates how to use the ISTAR GUI Framework to create
 * a main window similar to UIMA's interface with:
 * - Title bar with logo, datetime, message area, info fields, and status lights
 * - Main content area with stacked widget for function areas
 * - Navigation panel with NaviButton and indicator bars
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool IsLicenseRefused();

protected:
    // X 按钮 / WM_CLOSE 关窗也走这里（与 on_btn_Exit_clicked 共用 shutdownModules）：
    // 不补这套清理，zmq 后台线程会吊住进程退出（偶发 GUI_WM.exe 后台残留）
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_btn_Exit_clicked();
    void updateDateTime();
    void on_btn_Login_clicked();

signals:
    // 服务端就绪心跳（RPC 线程发出，跨线程 queued 到 GUI 线程 onUaServerReady）
    void sigUaServerReady(bool ready);
    // 异步登录/会话恢复结果（RPC 线程回调发出，queued 到 GUI 线程处理）
    void sigVisitorLoginResult(int status, LoginResponse resp);
    void sigSessionRestoreResult(int status, UserInfo user, const QString &sessionID);
    // 用户登录框：异步预填上次账号密码 / 异步登录结果
    void sigLastLoginReady(const QString &userName, const QString &password);
    void sigUserLoginResult(int status, LoginResponse resp, bool rememberUser,
                            bool rememberPassword, const QString &userName, const QString &password);

private:
    void initNavigationButtons();
    void initFunctionAreas();
    void initFrameworkManager();
    void initTitleBarStyles();
    void initLanguageSwitch();   // 顶部语言切换按钮
    void refreshTexts();
    QWidget *createContentPage(const QString &title);

    // 整机退出清理（one-shot）：closeEvent 与 on_btn_Exit_clicked 两条路径共用
    void shutdownModules();

    // ===== 整机界面使能（级别 × 权限）=====
    // 两个状态入口，统一经 applyUiEnable 落地：
    //   setUiLockLevel(level) —— 整机状态变化时调（onStatusChanged 的 switch 决定级别）
    //   onSessionChanged()    —— 登录/登出/会话失效时调（内部刷权限缓存并重放）
    // 级别（资格）与权限（许可）AND 合成：
    //   1 = 全部下使能，无视权限（仅常亮三件套：Exit/语言/告警区）；
    //   2 = + 登录按钮；状态页（Job→Status）交权限管，其余仍锁；
    //   3 = 全部页面交权限管理，权限管理亮灭。
    // 告警区与 Exit/语言、LED 同级常开；页面内容由各页自己的 setStatus 自锁，这里不碰。
    void setUiLockLevel(int level);   // 1/2/3；非法值收敛到边界（保守）
    void onSessionChanged();          // 会话变化：刷权限缓存 + 按当前级别重放

private:
    void refreshPermissionCache();    // 权限缓存拉取（同步 RPC，仅 onSessionChanged 调）
    void applyUiEnable();             // 级别 × 权限 → 导航/登录按钮（唯一落地函数）
    void setAreaEnabled(ISTAR::Framework::FunctionArea *area,
                        ISTAR::Framework::NaviButton *naviBtn, bool on);   // 整区开关

    // ===== UA 权限集成 =====
    void doUaLogin();
    void doUaVisitorLogin();
    void doUaLogout();
    bool isVisitorOrNoSession() const;
    void updateUserInfo(const std::string &userName, const std::string &levelName);
    // 异步恢复上次会话，无保存会话或校验失败时转异步 Visitor 登录（不阻塞 GUI 线程）
    void restoreOrVisitorLogin();
    void onVisitorLoginResult(int status, LoginResponse resp);
    void onSessionRestoreResult(int status, UserInfo user, const QString &sessionID);
    // 用户登录框：异步预填 / 异步登录结果处理
    void onLastLoginReady(const QString &userName, const QString &password);
    void onUserLoginResult(int status, LoginResponse resp, bool rememberUser,
                           bool rememberPassword, const QString &userName, const QString &password);
    // 注册整机导航模块树到 UA 服务（异步、幂等，服务端未启动时静默失败，就绪后重试）
    void registerMachineModules();
    // 服务端就绪心跳：模块重注册 + 启动时序兜底 + 控制层重启后复位重登
    void onUaServerReady(bool ready);
    void onStatusChanged(MctrlModule::MctrlStatus prev, MctrlModule::MctrlStatus curr);

private:
    Ui::MainWindow *ui;
    QTimer *m_pClockTimer;

    // shutdownModules() 的 one-shot 门闩（Exit 按钮的 close() 会再触发一次 closeEvent）
    bool m_modulesCleaned = false;

    // 当前 UA 登录会话的用户等级（用于整机二级导航栏权限置灰）
    int m_uaLevel = 0;

    // 当前上锁级别（setUiLockLevel 记录；权限变化后重调刷新）
    int m_uiLockLevel = 1;

    // 权限缓存（refreshPermissionCache 填充；setUiLockLevel 只读，
    // 不随整机状态抖动反复打 RPC）
    bool m_uaAdmin = false;               // 管理员（等级 >= 9）：权限全通过
    bool m_uaSuperAdmin = false;          // 超级管理员（等级 >= 10）：可见控制层内部 IO 号段
    bool m_uaPermValid = false;           // 缓存有效（有会话即有效；无效 = 按全通过，与旧版一致）
    std::set<std::string> m_uaOperable;   // 可操作模块 ID 集合

    // 服务端就绪状态（用于"未就绪→就绪"跃迁时重注册模块树）
    bool m_uaServerReady = false;

    // 人像开关防抖时间戳（登录/退出动作完成时刻，ms）
    qint64 m_lastAvatarActionMs = 0;

    // 当前打开的登录对话框（异步登录结果回 GUI 线程后用于更新/关闭）；exec 期间有效
    ISTAR::GUI::LoginDialog *m_loginDialog = nullptr;

    // Function areas for each navigation section
    ISTAR::Framework::FunctionArea *m_pJobsArea;
    ISTAR::Framework::FunctionArea *m_pSystemArea;
    ISTAR::Framework::FunctionArea *m_pRecipesArea;
    ISTAR::Framework::FunctionArea *m_pDataArea;
    ISTAR::Framework::FunctionArea *m_pSetupArea;
    ISTAR::Framework::FunctionArea *m_pMetrologyArea;
    ISTAR::Framework::FunctionArea *m_pAlarmsArea;
    ISTAR::Framework::FunctionArea *m_pHelpArea;

    // Job → Process 页实例（语言切换/启动恢复时刷新整页文本）
    ProcessPage *m_pProcessPage = nullptr;

    // System → Motion 页实例（语言切换/启动恢复时刷新整页文本）
    MotionPage *m_pMotionPage = nullptr;

    ConfigPage *m_pConfigPage = nullptr;
    bool m_licenseRefused = false;

    AutoProcessPage *m_pAutoProcessPage = nullptr;
    ContinuousBondPage *m_pContinuousBondPage = nullptr;

    // Job → Status 页实例（语言切换/启动恢复时刷新整页文本）
    SystemStatusPage *m_pSystemStatusPage = nullptr;

    MaterialPage *m_pMaterialPage = nullptr;

    IOPage *m_pIoPage = nullptr;

    // System → 相机标定页实例（语言切换/启动恢复时刷新整页文本）
    CameraCalibPage *m_pCameraCalibPage = nullptr;

    // ===== 框架语言切换 =====
    // 占位页标题 label（key → label，refreshTexts 重设文本）
    QMap<QString, QLabel *> m_placeholderLabels;
    // 各 FunctionArea 二级导航按钮文本 key（与 addFunctionPage 顺序一致）
    QVector<QPair<ISTAR::Framework::FunctionArea *, QStringList>> m_areaNaviKeys;
};

#endif // MAINWINDOW_H
