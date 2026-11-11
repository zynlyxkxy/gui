/***********************************************************************
 * Copyright (C) 2024 ISTAR.
 *
 * File Name:        mainwindow.cpp
 * Description:      ISTAR GUI Framework Example - UIMA Style Main Window
 * Version:          V0.4.0
 * Author:           iStar Platform
 * Date:             2025-07-24
 ***********************************************************************/

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QCloseEvent>

#include "istar_gui_framework.h"
#include "Common/ConfigRoot.h"
#include "istar_ui_stylesheet.h"
#include "istar_language.h"
#include "istar_appconfig.h"
#include "istar_enumconverter.hpp"
#include "PlatformModule/Rm/RMPage.h"
#include "PlatformModule/Eh/EHPage.h"
#include "PlatformModule/Mc/MCPage.h"
#include "PlatformModule/Mt/MTPage.h"
#include "PlatformModule/Rp/RPPage.h"
#include "PlatformModule/St/STPage.h"
#include "PlatformModule/Ua/UAPage.h"
#include "IOModule/IOPage.h"
#include "MotionModule/MotionPage.h"
#include "RPCModule/MCTRLClientManager.h"
#include "HelpModule/UpdatePage.h"
#include "ProcessModule/ProcessPage.h"
#include "AutoModule/AutoProcessPage.h"
#include "AutoModule/ContinuousBondPage.h"
#include "InfoModule/Status/SystemStatusPage.h"
#include "StatusModule/StatusManager.h"
#include "MaterialModule/MaterialPage.h"
#include "CameraCalibModule/CameraCalibPage.h"
#include "IOModule/IOPage.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDateTime>
#include <QFont>
#include <QApplication>
#include <QSet>
#include <functional>
#include "InfoModule/License/ConfigPage.h"

#define TR_STR(msg)  QString::fromStdString(Tr(msg))

// 平台登录对话框与消息确认框（UA 权限登录/退出）
#include "dialog/logindialog.h"
#include "dialog/messagedialog.h"
#include "istar_navi_button.h"

using namespace ISTAR::Framework;
using namespace ISTAR::UIStyle;
using ISTAR::GUI::MessageDialog;

// ===== UA 权限集成：进程级客户端与会话 =====
static std::shared_ptr<UA_Client_Inherit> g_uaClient;
static std::string g_uaSessionID;

// 整机一级导航模块树（注册到 UA 服务，供等级权限配置与二级导航栏置灰）。
// 一级导航 = 整机 GUI 导航区：Job/System/Recipes/Data/Setup/Metrology/Alarms/Help；
// 每个一级下挂其二级导航子模块（对应各 FunctionArea 的子页）。
static const UaModule kMachineModules[] = {
    {"Job", "Job", ""},
    {"Job_Production", "Production", "Job"},
    {"Job_Material", "Material", "Job"},
    {"Job_Process", "Process", "Job"},
    {"Job_Auto", "Auto", "Job"},
    {"Job_Continuous", "Continuous", "Job"},
    {"Job_Status", "Status", "Job"},
    {"System", "System", ""},
    {"System_SM", "SM", "System"},
    {"System_ST", "ST", "System"},
    {"System_IO", "IO", "System"},
    {"System_MotionStage", "MotionStage", "System"},
    {"System_CameraCalib", "CameraCalib", "System"},
    {"Recipes", "Recipes", ""},
    {"Recipes_Chip", "Chip", "Recipes"},
    {"Recipes_Wafer", "Wafer", "Recipes"},
    {"Data", "Data", ""},
    {"Data_Report", "Report", "Data"},
    {"Data_Statistics", "Statistics", "Data"},
    {"Setup", "Setup", ""},
    {"Setup_Constant", "Constant", "Setup"},
    {"Setup_Authority", "Authority", "Setup"},
    {"Setup_Config", "Config", "Setup"},
    {"Metrology", "Metrology", ""},
    {"Metrology_Single", "Single Script", "Metrology"},
    {"Metrology_Queue", "Metrology", "Metrology"},
    {"Metrology_History", "History", "Metrology"},
    // Alarms 有意不注册：报警查看不受权限管理，与 Exit/语言一样常开（安全兜底）
    // {"Alarms", "Alarms", ""},
    // {"Alarms_EH", "EH", "Alarms"},
    {"Help", "Help", ""},
    {"Help_Help", "Help", "Help"},
    {"Help_About", "About", "Help"},
    {"Help_Update", "Update", "Help"},
};

// 区域二级导航按钮列表（都在 objectName=SubNaviFrame 的 QFrame 内，与 kMachineModules 顺序对齐）
QList<ISTAR::Framework::NaviButton *> subNaviButtons(ISTAR::Framework::FunctionArea *area)
{
    if (area == nullptr)
    {
        return {};
    }
    if (auto *naviFrame = area->findChild<QFrame *>(QStringLiteral("SubNaviFrame")))
    {
        return naviFrame->findChildren<ISTAR::Framework::NaviButton *>();
    }
    return {};
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_pClockTimer(nullptr)
    , m_pJobsArea(nullptr)
    , m_pSystemArea(nullptr)
    , m_pRecipesArea(nullptr)
    , m_pDataArea(nullptr)
    , m_pSetupArea(nullptr)
    , m_pMetrologyArea(nullptr)
    , m_pAlarmsArea(nullptr)
    , m_pHelpArea(nullptr)
{
    // Initialize StyleSheet singleton BEFORE setupUi to ensure Qt resources
    // (fonts, images like iStarLogo.png) are registered via Q_INIT_RESOURCE
    // before any stylesheet references them.
    StyleSheet::getInstance();

    ui->setupUi(this);

    // Set frameless window like UIMA
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    //判断许可证
    m_pConfigPage = new ConfigPage(this);
    //m_pConfigPage->Authorize();
    //if(m_pConfigPage->CheckLicenseIsExpiration())
    //{
    //    m_licenseRefused = true;
    //    return;              // 全都不执行 → 一条线程都没有
    //}

    // Initialize components
    initTitleBarStyles();
    initNavigationButtons();

    MCTRLClientManager::getInstance()->Initialize(ConfigRoot::MwConfigFile(), "mctrl");

    // 机器状态：订阅 StatusManager，按状态调上锁级别 + 刷状态灯
    connect(StatusManager::instance(), &StatusManager::sigStatusChanged, this,
            [this](MctrlModule::MctrlStatus prev, MctrlModule::MctrlStatus curr)
    {
        onStatusChanged(prev, curr);
    });

    initFunctionAreas();
    initFrameworkManager();
    initLanguageSwitch();   // 顶部语言切换按钮

    // 初始 1 级锁定（启动期 UNKNOWN，仅 Exit/语言/告警区可用；
    // 之后由 onStatusChanged 按状态调级别，登录事件刷新权限过滤）
    setUiLockLevel(1);

    // Setup datetime timer
    m_pClockTimer = new QTimer(this);
    connect(m_pClockTimer, &QTimer::timeout, this, &MainWindow::updateDateTime);
    m_pClockTimer->start(1000);
    updateDateTime();

    // UA 登录按钮（人像：Visitor→登录，非 Visitor→退出）
    connect(ui->btn_Login, &QPushButton::clicked, this, &MainWindow::on_btn_Login_clicked);

    // UA 权限：初始化客户端、异步注册整机模块树（不阻塞 GUI 启动）
    if (!g_uaClient)
    {
        g_uaClient = std::make_shared<UA_Client_Inherit>();
        g_uaClient->UAClientInit(ConfigRoot::MwConfigFile(), "ua");
    }
    registerMachineModules();

    // 订阅服务端就绪（每 ~3s 心跳，RPC 线程回调）：
    // 覆盖"GUI 先于控制层启动"的补登，以及"控制层重启后会话失效"的复位重登
    g_uaClient->RegisterCallback(SUBSCRIBE_SERVER_STARTED_ID,
        [this](bool &ready) -> ISTARResultStatus
        {
            emit sigUaServerReady(ready);
            return ISTAR_RESULT_SUCCESS;
        });
    connect(this, &MainWindow::sigUaServerReady, this, &MainWindow::onUaServerReady);

    // 异步结果回调在 RPC 线程触发，跨线程 queued 信号需注册元类型
    qRegisterMetaType<LoginResponse>("LoginResponse");
    qRegisterMetaType<UserInfo>("UserInfo");
    connect(this, &MainWindow::sigVisitorLoginResult, this, &MainWindow::onVisitorLoginResult);
    connect(this, &MainWindow::sigSessionRestoreResult, this, &MainWindow::onSessionRestoreResult);
    connect(this, &MainWindow::sigLastLoginReady, this, &MainWindow::onLastLoginReady);
    connect(this, &MainWindow::sigUserLoginResult, this, &MainWindow::onUserLoginResult);

    // 启动不阻塞：异步恢复上次会话或默认 Visitor 登录；
    // 服务端未启动时 RPC 静默失败不卡界面，由就绪心跳兜底重试。
    // 无会话时标题栏用户信息显示空（清掉 .ui 默认的 Visitor 占位），登录成功后由结果回调填充
    updateUserInfo("", "");
    restoreOrVisitorLogin();


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onStatusChanged(MctrlModule::MctrlStatus prev, MctrlModule::MctrlStatus curr)
{
    Q_UNUSED(prev);
    ui->led_RemoteState->setText(TR_STR(ISTAR::ISTAREnumConverter::ConvertToString(curr)));

    // 二级导航按钮（Alarms/Report 页对应的子导航按钮）
    ISTAR::Framework::NaviButton *pSubBtn = m_pAlarmsArea->GetFuncNaviButton("EHReport");
    if (nullptr == pSubBtn)
    {
        ISTARLogWarn("ViewStatus demo: sub navi button not found for page EH");
        return;
    }


    // 状态 → 上锁级别（分类只此一处，要调级别规则改这里；新状态落 default = 最保守 1 级）
    switch (curr)
    {

        case MctrlModule::MCTRL_STATUS_UNKNOWN:
        case MctrlModule::MCTRL_STATUS_LOADING:
        case MctrlModule::MCTRL_STATUS_LOAD_FAIL:   // 控制层启动失败：与 UNKNOWN/LOADING 同为最低等级
        case MctrlModule::MCTRL_STATUS_DISCONNECTED:
            setUiLockLevel(1);   // 1 级：全部下使能，无视权限（仅常亮三件套）
            break;
        case MctrlModule::MCTRL_STATUS_LOAD_OK:
        case MctrlModule::MCTRL_STATUS_INITIALIZING:
        case MctrlModule::MCTRL_STATUS_INI_FAIL:
            setUiLockLevel(2);   // 2 级：+ 登录；状态页交权限管，其余仍锁
            break;
        case MctrlModule::MCTRL_STATUS_IDLE:
        case MctrlModule::MCTRL_STATUS_BUSY:
            setUiLockLevel(3);   // 3 级：全部页面交权限管理，权限管理亮灭
            pSubBtn->SetViewStatus(Idle);
            ui->btn_Alarms->SetViewStatus(Idle);
            break;
        case MctrlModule::MCTRL_STATUS_ALARM:
        {
            setUiLockLevel(3);   // 3 级：全部页面交权限管理，权限管理亮灭
            // 二级按钮闪烁，同时透传到所属一级导航按钮（Alarms）同色闪烁
            pSubBtn->SetViewStatus(Alarm);
            ui->btn_Alarms->SetViewStatus(Alarm);

            MessageDialog dlg(MessageDialog::Error, this, QStringLiteral("出现报警，请先处理！"));
            dlg.exec();
            break;
        }

        default:
            setUiLockLevel(1);
            break;
    }
}

bool MainWindow::IsLicenseRefused()
{
    return m_licenseRefused;
}
void MainWindow::on_btn_Exit_clicked()
{
    shutdownModules();

    // Close the main window
    close();

    // Now exit the application event loop normally
    QApplication::quit();
}

// 整机退出清理（one-shot，X 关窗与 Exit 按钮两条路径共用）。
// 各 cleanup*Module() 会触发模块 View::closeEvent() 里的 Communication_Exit()，
// 停掉 ZMQ 监听线程并释放资源——不清理的话后台线程会吊住 QApplication::quit()。
void MainWindow::shutdownModules()
{
    if (m_modulesCleaned)
        return;
    m_modulesCleaned = true;

    // Clean up RM module first - this triggers View::closeEvent() which calls
    // Communication_Exit() to stop ZMQ threads and release resources.
    PlatformModule::cleanupRmModule();

    // Clean up EH module - triggers MainView::closeEvent() which calls
    // EHGUIClient::CommunicationExit() to stop ZMQ threads and release resources.
    PlatformModule::cleanupEhModule();

    // Clean up MT module - closes the MT pages; UIMT_PageCleanup() stops the
    // Python script process and the shared ZMQ communication threads.
    PlatformModule::cleanupMtModule();

    // Clean up MC module - triggers MainView::closeEvent() which calls
    // MCGUIClient::CommunicationExit() to stop ZMQ threads and release resources.
    PlatformModule::cleanupMcModule();

    // Clean up RP module - triggers MainView::closeEvent() which calls
    // RPGUIClient::CommunicationExit() to stop ZMQ threads and release resources.
    PlatformModule::cleanupRpModule();

    // Clean up UA module - triggers MainView::closeEvent() to stop UA threads/resources.
    PlatformModule::cleanupUaModule();

    // close ZMQ
    MCTRLClientManager::getInstance()->Shutdown();
}

// X 按钮 / WM_CLOSE 关窗：QApplication::exec() 直接返回、不走 on_btn_Exit_clicked，
// 所以在这里补同一套清理，否则进程带着 zmq 后台线程退出不干净（偶发后台残留）。
// shutdownModules 是 one-shot：Exit 按钮路径里 close() 再触发到这里时为空操作。
void MainWindow::closeEvent(QCloseEvent *event)
{
    shutdownModules();
    QMainWindow::closeEvent(event);
}

void MainWindow::updateDateTime()
{
    ui->lbl_DateTime->setText(QDateTime::currentDateTime().toString("yyyy/MM/dd  hh:mm:ss"));
}

void MainWindow::initNavigationButtons()
{
    // Get the icomoon font from StyleSheet (loaded via QFontDatabase::addApplicationFont)
    QFont naviFont = StyleSheet::getInstance()->Font();

    // Use the SAME icon characters as UIMA's naviBtnInitial() in mainwindow.cpp
    // UIMA uses: 0xa024=Job, 0xa024=System, 0xa028=Recipes, 0xa026=Metrology,
    //            0xa030=Setup, 0xa022=Data, 0xa020=Alarms, 0xa023=Help

    // Jobs button
    ui->btn_Jobs->SetVertivalStyle(naviFont, QString(QChar(0xa024)), 30, "Job", 14, QFont::ExtraBold);
    ui->btn_Jobs->ApplyMainNaviStyle();

    // System button
    ui->btn_System->SetVertivalStyle(naviFont, QString(QChar(0xa024)), 30, "System", 14, QFont::ExtraBold);
    ui->btn_System->ApplyMainNaviStyle();

    // Recipes button
    ui->btn_Recipes->SetVertivalStyle(naviFont, QString(QChar(0xa028)), 30, "Recipes", 14, QFont::ExtraBold);
    ui->btn_Recipes->ApplyMainNaviStyle();

    // DataLog button
    ui->btn_DataLog->SetVertivalStyle(naviFont, QString(QChar(0xa022)), 30, "Data", 14, QFont::ExtraBold);
    ui->btn_DataLog->ApplyMainNaviStyle();

    // Setup button
    ui->btn_Setup->SetVertivalStyle(naviFont, QString(QChar(0xa030)), 30, "Setup", 14, QFont::ExtraBold);
    ui->btn_Setup->ApplyMainNaviStyle();

    // Metrology button
    ui->btn_Metrology->SetVertivalStyle(naviFont, QString(QChar(0xa026)), 30, "Metrology", 14, QFont::ExtraBold);
    ui->btn_Metrology->ApplyMainNaviStyle();

    // Alarms button
    ui->btn_Alarms->SetVertivalStyle(naviFont, QString(QChar(0xa020)), 30, "Alarms", 14, QFont::ExtraBold);
    ui->btn_Alarms->ApplyMainNaviStyle();

    // Help button
    ui->btn_Help->SetVertivalStyle(naviFont, QString(QChar(0xa023)), 30, "Help", 14, QFont::ExtraBold);
    ui->btn_Help->ApplyMainNaviStyle();

    // Setup Login/Exit buttons with icon font (matching UIMA titleWidgetInitial)
    QFont iconFont = StyleSheet::getInstance()->Font();
    iconFont.setPixelSize(30);
    ui->btn_Login->setFont(iconFont);
    ui->btn_Login->setText(QString(QChar(0xa001)));
    ui->btn_Login->setStyleSheet(StyleSheet::getInstance()->ButtonStyle_Front);

    ui->btn_Exit->setFont(iconFont);
    ui->btn_Exit->setText(QString(QChar(0xa005)));
    ui->btn_Exit->setStyleSheet(StyleSheet::getInstance()->ButtonStyle_Front);
}

QWidget *MainWindow::createContentPage(const QString &title)
{
    QWidget *pPage = new QWidget(this);
    pPage->setStyleSheet("background-color: #F3F4FA;");

    QVBoxLayout *pLayout = new QVBoxLayout(pPage);
    pLayout->setAlignment(Qt::AlignCenter);

    QLabel *pLabel = new QLabel(title, pPage);
    pLabel->setAlignment(Qt::AlignCenter);
    pLabel->setStyleSheet(StyleSheet::getInstance()->fontStyle_bold_16 +
                          "QLabel{color: #333333; background-color: transparent;}");
    // 保存标题 label 供语言切换时刷新
    m_placeholderLabels[title] = pLabel;
    pLayout->addWidget(pLabel);

    return pPage;
}

void MainWindow::initFunctionAreas()
{
    // 二级导航页添加辅助：buttonText 走平台 Tr，并记录文本 key 供语言切换刷新
    auto addNaviPage = [](ISTAR::Framework::FunctionArea *area, const QString &pageName,
                          const char *textKey, QWidget *widget, QStringList *keys)
    {
        area->addFunctionPage(pageName, QString::fromStdString(Tr(textKey)), widget);
        if (keys)
        {
            keys->append(QString::fromLatin1(textKey));
        }
    };

    // Jobs FunctionArea
    m_pJobsArea = new FunctionArea("Job", this);
    {
        QStringList keys;
        addNaviPage(m_pJobsArea, "Production", "Production", createContentPage("Job - Production"), &keys);
        addNaviPage(m_pJobsArea, "Material", "Material", m_pMaterialPage = new MaterialPage(this) , &keys);
        addNaviPage(m_pJobsArea, "Process", "Process", m_pProcessPage = new ProcessPage(this), &keys);
        addNaviPage(m_pJobsArea, "Auto", "Auto", m_pAutoProcessPage = new AutoProcessPage(this), &keys);
        addNaviPage(m_pJobsArea, "Continuous", "Continuous", m_pContinuousBondPage = new ContinuousBondPage(this), &keys);
        m_pSystemStatusPage = new SystemStatusPage(this);
        addNaviPage(m_pJobsArea, "Status", "Status", m_pSystemStatusPage, &keys);
        m_pJobsArea->setCurrentFuncPage("Production");
        m_areaNaviKeys.append({m_pJobsArea, keys});
    }

    // System FunctionArea
    m_pSystemArea = new FunctionArea("System", this);
    {
        QStringList keys;
        addNaviPage(m_pSystemArea, "SM", "SM", createContentPage("System - SM"), &keys);
        addNaviPage(m_pSystemArea, "ST", "ST", createContentPage("System - ST"), &keys);
        addNaviPage(m_pSystemArea, "IO", "IO", m_pIoPage = new IOPage(this), &keys);
        addNaviPage(m_pSystemArea, "MotionStage", "Motion Stage", m_pMotionPage = new MotionPage(this), &keys);
        addNaviPage(m_pSystemArea, "CameraCalib", "Camera Calibration",
                    m_pCameraCalibPage = new CameraCalibPage(this), &keys);
        m_pSystemArea->setCurrentFuncPage("SM");
        m_areaNaviKeys.append({m_pSystemArea, keys});
    }

    // Recipes FunctionArea - RM module widget from PlatformModule
    // Debug 构建加载带 d 后缀的 RE 编辑器库 (libUIRE_CHIPd.dll / libUIRE_Waferd.dll),
    // 与 libUIRmd 的加载命名约定一致
    m_pRecipesArea = new FunctionArea("Recipes", this);
    {
        QStringList keys;
#ifdef GUI_DEBUG
        ISTARLogInfo("Module loading: RM(rm/libUIRE_CHIPd, rm2/libUIRE_Waferd)");
        addNaviPage(m_pRecipesArea, "RecipeChipEditor", "Chip", PlatformModule::getRmModuleWidget("rm", "libUIRE_CHIPd"), &keys);
        addNaviPage(m_pRecipesArea, "RecipeWaferEditor", "Wafer", PlatformModule::getRmModuleWidget("rm2", "libUIRE_Waferd"), &keys);
#else
        ISTARLogInfo("Module loading: RM(rm/libUIRE_CHIP, rm2/libUIRE_Wafer)");
        addNaviPage(m_pRecipesArea, "RecipeChipEditor", "Chip", PlatformModule::getRmModuleWidget("rm", "libUIRE_CHIP"), &keys);
        addNaviPage(m_pRecipesArea, "RecipeWaferEditor", "Wafer", PlatformModule::getRmModuleWidget("rm2", "libUIRE_Wafer"), &keys);
#endif
        m_pRecipesArea->setCurrentFuncPage("RecipeChipEditor");
        m_areaNaviKeys.append({m_pRecipesArea, keys});
    }

    // DataLog FunctionArea
    m_pDataArea = new FunctionArea("Data", this);
    {
        QStringList keys;
        ISTARLogInfo("Module loading: RP(rp)");
        addNaviPage(m_pDataArea, "Report", "Report", PlatformModule::getRpModuleWidget("rp"), &keys);
        addNaviPage(m_pDataArea, "PS", "PS", createContentPage("Data - PS"), &keys);
        m_pDataArea->setCurrentFuncPage("Report");
        m_areaNaviKeys.append({m_pDataArea, keys});
    }

    // Setup FunctionArea
    m_pSetupArea = new FunctionArea("Setup", this);
    {
        QStringList keys;
        ISTARLogInfo("Module loading: MC(mc)");
        addNaviPage(m_pSetupArea, "MC", "MC", PlatformModule::getMcModuleWidget("mc"), &keys);
        addNaviPage(m_pSetupArea, "Authority", "Authority", PlatformModule::getUaModuleWidget("ua"), &keys);
        addNaviPage(m_pSetupArea, "Config", "Config", m_pConfigPage, &keys);
        m_pSetupArea->setCurrentFuncPage("MC");
        m_areaNaviKeys.append({m_pSetupArea, keys});
    }

    // Metrology FunctionArea - MT module pages from PlatformModule
    // 二级导航与平台 MT 一致：Single Script / Queue / Report（页名别名见 MTPage.h）
    m_pMetrologyArea = new FunctionArea("Metrology", this);
    {
        QStringList keys;
        ISTARLogInfo("Module loading: MT(Single/Queue/History)");
        addNaviPage(m_pMetrologyArea, "MTSingle", "Single Script", PlatformModule::getMtPageWidget("Single"), &keys);
        addNaviPage(m_pMetrologyArea, "MTQueue", "Queue", PlatformModule::getMtPageWidget("Queue"), &keys);
        addNaviPage(m_pMetrologyArea, "MTHistory", "Report", PlatformModule::getMtPageWidget("History"), &keys);
        m_pMetrologyArea->setCurrentFuncPage("MTSingle");
        m_areaNaviKeys.append({m_pMetrologyArea, keys});
    }

    // Alarms FunctionArea - EH module widget from PlatformModule
    m_pAlarmsArea = new FunctionArea("Alarms", this);
    {
        QStringList keys;
        ISTARLogInfo("Module loading: EH(eh)");
        addNaviPage(m_pAlarmsArea, "EHReport", "Report", PlatformModule::getEhPageWidget("Report"), &keys);
        addNaviPage(m_pAlarmsArea, "EHEdit", "Edit", PlatformModule::getEhPageWidget("Edit"), &keys);
        addNaviPage(m_pAlarmsArea, "EHHistory", "History", PlatformModule::getEhPageWidget("History"), &keys);
        m_pAlarmsArea->setCurrentFuncPage("EHReport");
        m_areaNaviKeys.append({m_pAlarmsArea, keys});
    }

    // Help FunctionArea
    m_pHelpArea = new FunctionArea("Help", this);
    {
        QStringList keys;
        addNaviPage(m_pHelpArea, "Manual", "Manual", createContentPage("Help - User Manual"), &keys);
        addNaviPage(m_pHelpArea, "About", "About", createContentPage("Help - About ISTAR"), &keys);
        addNaviPage(m_pHelpArea, "Update", "Update", new UpdatePage("Update", this), &keys);
        m_pHelpArea->setCurrentFuncPage("Manual");
        m_areaNaviKeys.append({m_pHelpArea, keys});
    }

    // Add FunctionAreas to stacked widget
    ui->stackedWidget->addWidget(m_pJobsArea);
    ui->stackedWidget->addWidget(m_pSystemArea);
    ui->stackedWidget->addWidget(m_pRecipesArea);
    ui->stackedWidget->addWidget(m_pDataArea);
    ui->stackedWidget->addWidget(m_pSetupArea);
    ui->stackedWidget->addWidget(m_pMetrologyArea);
    ui->stackedWidget->addWidget(m_pAlarmsArea);
    ui->stackedWidget->addWidget(m_pHelpArea);
}

void MainWindow::initTitleBarStyles()
{
    // Apply platform StyleSheet to title bar labels and line edits
    // matching UIMA's mainwindow.cpp style initialization
    StyleSheet *pStyle = StyleSheet::getInstance();

    // Labels use fontStyle_normal_14 (matching UIMA's lblSetState)
    QString lblStyle = pStyle->fontStyle_normal_14;
    ui->lbl_Machine->setStyleSheet(lblStyle);
    ui->lbl_Version->setStyleSheet(lblStyle);
    ui->lbl_RemoteState->setStyleSheet(lblStyle);
    ui->lbl_Machine_2->setStyleSheet(lblStyle); // User label
    ui->lbl_Version_2->setStyleSheet(lblStyle); // Level label

    // Line edits use LineEdit_Dark + fontStyle_normal_14 (matching UIMA)
    QString ledStyle = pStyle->LineEdit_Dark + pStyle->fontStyle_normal_14;
    ui->led_Machine->setStyleSheet(ledStyle);
    ui->led_Version->setStyleSheet(ledStyle);
    ui->led_RemoteState->setStyleSheet(ledStyle);
    ui->led_UserName->setStyleSheet(ledStyle);
    ui->led_UserLevel->setStyleSheet(ledStyle);

    // Set default values (matching UIMA)
    ui->led_Machine->setText("iCB3000");
    ui->led_Version->setText("0.0.1");
    ui->led_RemoteState->setText("Offline");
}

void MainWindow::initFrameworkManager()
{
    FrameworkManager::getInstance()->SetMainWindow(this);
    FrameworkManager::getInstance()->SetMainStackedWidget(ui->stackedWidget);

    FrameworkManager::getInstance()->InitOpt(ui->btn_Jobs, m_pJobsArea, ui->stackedWidget, this, 0, true);
    FrameworkManager::getInstance()->InitOpt(ui->btn_System, m_pSystemArea, ui->stackedWidget, this, 1, true);
    FrameworkManager::getInstance()->InitOpt(ui->btn_Recipes, m_pRecipesArea, ui->stackedWidget, this, 2, true);
    FrameworkManager::getInstance()->InitOpt(ui->btn_DataLog, m_pDataArea, ui->stackedWidget, this, 3, true);
    FrameworkManager::getInstance()->InitOpt(ui->btn_Setup, m_pSetupArea, ui->stackedWidget, this, 4, true);
    FrameworkManager::getInstance()->InitOpt(ui->btn_Metrology, m_pMetrologyArea, ui->stackedWidget, this, 5, true);
    FrameworkManager::getInstance()->InitOpt(ui->btn_Alarms, m_pAlarmsArea, ui->stackedWidget, this, 6, true);
    FrameworkManager::getInstance()->InitOpt(ui->btn_Help, m_pHelpArea, ui->stackedWidget, this, 7, true);

    FrameworkManager::getInstance()->Initialize();

    // Select first navigation button (Jobs)
    ui->btn_Jobs->click();
}

void MainWindow::initLanguageSwitch()
{
    // 平台多语言初始化：显式给绝对目录 <配置根>/gui_config/Languages（ISTARLanguage 是
    // 纯 CRT 实现不依赖 Qt，相对路径会随 cwd 解析 —— 这里绝不依赖 cwd）。
    // 机器常数/处方 Name 词典为模块各自独立文件，见 MC/RM 的 SetMcDictPath。
    ISTARLanguage *lang = ISTARLanguage::GetInstance();
    lang->SetLanguageType(0);
    const QString langFolder = ConfigRoot::Under(QStringLiteral("gui_config/Languages"));
    const QString langFile = langFolder + QStringLiteral("/Language.csv");
    if (lang->Initialize(langFolder.toStdString()) == ISTAR::BasicPlatform::ISTAR_PLATFORM_SUCCESS)
        ISTARLogInfo("ISTARLanguage loaded OK: %s", langFile.toStdString().c_str());
    else
        ISTARLogWarn("ISTARLanguage load FAILED: %s (Tr() keeps key, UI stays English)", langFile.toStdString().c_str());

    // 语言切换按钮：中/EN 横排，放在人像/退出登录列（gridLayout_3 第 1 列）顶部
    auto *langPanel = new QWidget(ui->frame_Title);
    auto *langLayout = new QHBoxLayout(langPanel);
    langLayout->setContentsMargins(0, 0, 0, 0);
    langLayout->setSpacing(0);

    auto *btnZh = new QPushButton(QStringLiteral("中"), langPanel);
    auto *btnEn = new QPushButton(QStringLiteral("EN"), langPanel);
    const QString langBtnStyle =
        "QPushButton{background-color:#FFFFFF;color:#666666;border:1px solid #D5D9E0;"
        "font-weight:bold;font-size:11px;padding:0px;}"
        "QPushButton:hover{background-color:#EAF0FF;color:#4C70FA;border-color:#A3B6FF;}"
        "QPushButton:checked{background-color:#4C70FA;color:#FFFFFF;border-color:#4C70FA;}";
    btnZh->setStyleSheet(langBtnStyle +
        QStringLiteral("QPushButton{border-top-left-radius:5px;border-bottom-left-radius:5px;border-right:none;}"));
    btnEn->setStyleSheet(langBtnStyle +
        QStringLiteral("QPushButton{border-top-right-radius:5px;border-bottom-right-radius:5px;}"));
    btnZh->setFixedSize(34, 21);
    btnEn->setFixedSize(34, 21);
    btnZh->setCheckable(true);
    btnEn->setCheckable(true);
    btnEn->setChecked(true);   // 默认英文（平台语言 0）
    btnZh->setToolTip(QStringLiteral("切换为中文"));
    btnEn->setToolTip(QStringLiteral("Switch to English"));
    langLayout->addWidget(btnZh);
    langLayout->addWidget(btnEn);

    // 插入到人像/退出登录列（col2 的 verticalLayout）顶部，btn_Login 上方
    if (auto *loginColumn = ui->frame_Title->findChild<QVBoxLayout *>(QStringLiteral("verticalLayout")))
    {
        loginColumn->insertWidget(0, langPanel);
    }
    else if (QGridLayout *grid = ui->frame_Title->findChild<QGridLayout *>(QStringLiteral("gridLayout_3")))
    {
        grid->addWidget(langPanel, 0, 2);
    }

    // 应用语言：平台语言 + 框架界面 + 各平台模块界面（按钮切换与启动恢复共用）
    const auto applyLanguage = [this, btnZh, btnEn](int type) {
        ISTARLanguage::GetInstance()->SetLanguageType(type);
        refreshTexts();                             // 整机框架界面（导航栏/占位页/标题栏）
        // 平台模块界面（需各自模块库导出对应 SetLanguage 接口，缺失时静默跳过）
        PlatformModule::setEhLanguage(type);        // EH 模块界面（libUIEH → UIEH_SetLanguage）
        PlatformModule::setRmLanguage(type);        // RM 模块界面（libUIRm → UIRm_SetLanguage）
        PlatformModule::setRpLanguage(type);        // RP 模块界面（libUIRP → UIRP_SetLanguage）
        PlatformModule::setStLanguage(type);        // ST 模块界面（libUIST → UIST_SetLanguage）
        PlatformModule::setMcLanguage(type);        // MC 模块界面（libUIMC → UIMC_SetLanguage）
        PlatformModule::setUaLanguage(type);        // UA 模块界面（libUIUA → UIUA_SetLanguage）
        PlatformModule::setMtLanguage(type);        // MT 测校界面（libUIMT → UIMT_SetLanguage）

        btnZh->setChecked(type == 1);
        btnEn->setChecked(type == 0);
        ISTARLogInfo("GUI language switched to %s", type == 1 ? "Chinese" : "English");
    };

    // 每次切换中/英时，把 language 属性写回当前读取的 appconfig.yaml
    // （0=English 1=Chinese 2=Japanese；其余字段保持不变，供下次启动恢复）
    const auto saveLanguageToAppConfig = [](int type) {
        ISTAR::BasicPlatform::ISTARAppConfigManager *appCfg =
            ISTAR::BasicPlatform::ISTARAppConfigManager::getInstance();
        if (!appCfg || !appCfg->IsInitialized())
        {
            ISTARLogWarn("AppConfig not initialized, skip saving language=%d", type);
            return;
        }
        ISTAR::BasicPlatform::AppConfigData cfg = appCfg->GetAppConfig();
        cfg.language = type;
        if (appCfg->UpdateAppConfig(cfg) != ISTAR::BasicPlatform::ISTAR_PLATFORM_SUCCESS)
        {
            ISTARLogWarn("AppConfig: failed to save language=%d to %s", type,
                         appCfg->GetConfigFilePath().c_str());
        }
        else
        {
            ISTARLogInfo("AppConfig: language=%d saved to %s", type,
                         appCfg->GetConfigFilePath().c_str());
        }
    };

    connect(btnZh, &QPushButton::clicked, this, [applyLanguage, saveLanguageToAppConfig]() {
        applyLanguage(1);
        saveLanguageToAppConfig(1);
    });
    connect(btnEn, &QPushButton::clicked, this, [applyLanguage, saveLanguageToAppConfig]() {
        applyLanguage(0);
        saveLanguageToAppConfig(0);
    });

    // 程序启动：读取上次保存的 appconfig.yaml 的 language 属性来切换初始界面语言；
    // 未初始化（无配置文件）或上次保存为非中文时保持默认英文（btnEn 已默认选中）
    ISTAR::BasicPlatform::ISTARAppConfigManager *appCfg =
        ISTAR::BasicPlatform::ISTARAppConfigManager::getInstance();
    if (appCfg && appCfg->IsInitialized())
    {
        const int savedLang = appCfg->GetAppConfig().language;
        ISTARLogInfo("AppConfig: startup language from appconfig.yaml language=%d", savedLang);
        if (savedLang == 1)
        {
            applyLanguage(1);   // 上次保存为中文 → 启动即切到中文界面
        }
    }
}

void MainWindow::refreshTexts()
{
    // 一级导航按钮
    ui->btn_Jobs->SetText(QString::fromStdString(Tr("Job")));
    ui->btn_System->SetText(QString::fromStdString(Tr("System")));
    ui->btn_Recipes->SetText(QString::fromStdString(Tr("Recipes")));
    ui->btn_DataLog->SetText(QString::fromStdString(Tr("Data")));
    ui->btn_Setup->SetText(QString::fromStdString(Tr("Setup")));
    ui->btn_Metrology->SetText(QString::fromStdString(Tr("Metrology")));
    ui->btn_Alarms->SetText(QString::fromStdString(Tr("Alarms")));
    ui->btn_Help->SetText(QString::fromStdString(Tr("Help")));

    // 二级导航按钮（按 addFunctionPage 顺序重设文本）
    for (const auto &kv : m_areaNaviKeys)
    {
        if (!kv.first)
        {
            continue;
        }
        if (auto *naviFrame = kv.first->findChild<QFrame *>(QStringLiteral("SubNaviFrame")))
        {
            const auto buttons = naviFrame->findChildren<ISTAR::Framework::NaviButton *>();
            for (int i = 0; i < kv.second.size() && i < buttons.size(); ++i)
            {
                buttons[i]->SetText(QString::fromStdString(Tr(kv.second[i].toStdString())));
            }
        }
    }

    // 占位页标题
    for (auto it = m_placeholderLabels.constBegin(); it != m_placeholderLabels.constEnd(); ++it)
    {
        if (it.value())
        {
            it.value()->setText(QString::fromStdString(Tr(it.key().toStdString())));
        }
    }

    // 标题栏标签与默认值
    ui->lbl_Machine->setText(QString::fromStdString(Tr("Machine：")));
    ui->lbl_Version->setText(QString::fromStdString(Tr("Version：")));
    ui->lbl_RemoteState->setText(QString::fromStdString(Tr("Remote State：")));
    ui->lbl_Machine_2->setText(QString::fromStdString(Tr("User:")));
    ui->lbl_Version_2->setText(QString::fromStdString(Tr("Level:")));
    //ui->led_RemoteState->setText(QString::fromStdString(Tr("Offline")));

    // UA 模块树显示名随语言重新注册（异步、幂等）
    // registerMachineModules();

    // Job → Process 页：整页文本随语言切换刷新
    if (m_pProcessPage)
    {
        m_pProcessPage->refreshTexts();
    }

    // System → Motion 页：tab 标题与 Stage 卡片文案随语言切换刷新
    if (m_pMotionPage)
    {
        m_pMotionPage->refreshTexts();
    }

    if(m_pConfigPage)
    {
        m_pConfigPage->refreshTexts();
    }

    if(m_pAutoProcessPage)
    {
        m_pAutoProcessPage->refreshTexts();
    }

    if(m_pContinuousBondPage)
    {
        m_pContinuousBondPage->refreshTexts();
    }

    // Job → Status 页：整页文本（表头/按钮/图例标签/分组框标题）随语言切换刷新
    if(m_pSystemStatusPage)
    {
        m_pSystemStatusPage->refreshTexts();
    }

    if(m_pMaterialPage)
    {
        m_pMaterialPage->refreshTexts();
    }

    if (m_pIoPage)
    {
        m_pIoPage->refreshTexts();
    }

    // System → 相机标定页：整页文本（分组框标题/标签/按钮/下拉显示名）随语言切换刷新
    if (m_pCameraCalibPage)
    {
        m_pCameraCalibPage->refreshTexts();
    }
}

void MainWindow::on_btn_Login_clicked()
{
    // 防抖：登录/退出动作完成后 500ms 内忽略重复的人像点击，
    // 避免登录框关闭瞬间的排队/双击事件立即走"退出"分支撤销刚完成的登录
    if (QDateTime::currentMSecsSinceEpoch() - m_lastAvatarActionMs < 500)
    {
        return;
    }

    // 人像为登录开关（参考 MainProject\mainwindow.cpp）：
    // Visitor/未登录 -> 弹登录框；已登录真实用户 -> 弹确认框，确认后退出并回到 Visitor
    if (isVisitorOrNoSession())
    {
        doUaLogin();
        // 登录框关闭后把焦点移开人像，避免回车误触再次触发
        ui->stackedWidget->setFocus();
    }
    else
    {
        ISTAR::GUI::MessageDialog confirm(ISTAR::GUI::MessageDialog::Attention, this,
                                          QStringLiteral("确认退出登录？"));
        if (confirm.exec() == QDialog::Accepted)
        {
            doUaLogout();
            doUaVisitorLogin();
        }
    }
    m_lastAvatarActionMs = QDateTime::currentMSecsSinceEpoch();
}

void MainWindow::doUaLogin()
{
    if (!g_uaClient)
    {
        g_uaClient = std::make_shared<UA_Client_Inherit>();
        g_uaClient->UAClientInit(ConfigRoot::MwConfigFile(), "ua");
    }

    // 弹登录框（复用平台 LoginDialog），异步调用 UA 登录（不阻塞 GUI 线程）
    ISTAR::GUI::LoginDialog dlg(this);
    m_loginDialog = &dlg;

    // 异步预填上次记住的账号密码（SuperAdmin 除外；仅记住账号时密码为空，只填账号）
    g_uaClient->UAGetLastLoginAsync("", [this](ResponseMessageStruct<UaLastLogin> &resp) -> ISTARResultStatus
    {
        if (resp.errorCode == UA_OP_OK && !resp.data.userName.empty() &&
            resp.data.userName.compare("SuperAdmin") != 0)
        {
            emit sigLastLoginReady(QString::fromStdString(resp.data.userName),
                                   QString::fromStdString(resp.data.password));
        }
        return ISTAR_RESULT_SUCCESS;
    });

    // 平台登录框未设默认按钮，回车不会触发登录；这里把 OK 设为默认，回车即可提交
    if (auto *okBtn = dlg.findChild<QPushButton *>(QStringLiteral("btn_OK")))
    {
        okBtn->setDefault(true);
    }
    connect(&dlg, &ISTAR::GUI::LoginDialog::sig_SendLoginInfo,
            this, [this](const QString &userName, const QString &password,
                         bool rememberUser, bool rememberPassword)
    {
        // 异步登录，结果经 sigUserLoginResult 回 GUI 线程（onUserLoginResult 更新/关闭对话框）
        LoginRequest req;
        req.userName = userName.toStdString();
        req.password = password.toStdString();
        g_uaClient->UALoginAsync(req, [this, rememberUser, rememberPassword, userName, password](
                                     ResponseMessageStruct<LoginResponse> &resp) -> ISTARResultStatus
        {
            emit sigUserLoginResult((int)resp.errorCode, resp.data, rememberUser, rememberPassword,
                                    userName, password);
            return ISTAR_RESULT_SUCCESS;
        });
    });
    dlg.exec();
    m_loginDialog = nullptr;
}

void MainWindow::onLastLoginReady(const QString &userName, const QString &password)
{
    if (m_loginDialog && m_loginDialog->isVisible())
    {
        m_loginDialog->on_RecivedLastUser(userName, password);
    }
}

void MainWindow::onUserLoginResult(int status, LoginResponse resp, bool rememberUser,
                                   bool rememberPassword, const QString &userName, const QString &password)
{
    if (status != UA_LOGIN_OK)
    {
        // 平台 LoginDialog::on_RecivedLoginInfo 实际语义（与其头文件注释相反）：
        // 0=成功 1=密码错误 2=用户名不存在；其余服务端状态在此显式提示，
        // 避免把“用户被禁用/账号锁定/尝试过多”笼统显示成 unknown error。
        if (m_loginDialog && m_loginDialog->isVisible())
        {
            switch (status)
            {
                case UA_LOGIN_USER_NOT_FOUND:
                    m_loginDialog->on_RecivedLoginInfo(2);
                    break;
                case UA_LOGIN_PASSWORD_WRONG:
                    m_loginDialog->on_RecivedLoginInfo(1);
                    break;
                case UA_LOGIN_USER_DISABLED:
                {
                    MessageDialog dlg(MessageDialog::Error, m_loginDialog,
                                      QStringLiteral("用户已被禁用，请联系管理员"));
                    dlg.exec();
                }
                break;
                case UA_LOGIN_ACCOUNT_LOCKED:
                {
                    MessageDialog dlg(MessageDialog::Error, m_loginDialog,
                                      QStringLiteral("账号已锁定，请联系管理员"));
                    dlg.exec();
                }
                break;
                case UA_LOGIN_TOO_MANY_ATTEMPTS:
                {
                    MessageDialog dlg(MessageDialog::Error, m_loginDialog,
                                      QStringLiteral("登录尝试次数过多，请稍后再试"));
                    dlg.exec();
                }
                break;
                default:
                {
                    // UA_LOGIN_OTHER(6) 或未知错误码：记日志定位服务端原因
                    ISTARLogWarn("UA login failed: userName=%s status=%d",
                                 qPrintable(userName), status);
                    m_loginDialog->on_RecivedLoginInfo(3);
                }
                break;
            }
        }
        return;
    }

    g_uaSessionID = resp.sessionID;
    m_uaLevel = resp.level;
    // 注入共享会话给 UA 模块页面
    PlatformModule::setUASession(resp.sessionID.c_str());
    updateUserInfo(resp.displayName, resp.levelName);
    onSessionChanged();

    // 持久化会话，供 GUI 重启后恢复（控制层未重启时重挂原会话）
    QSettings settings;
    settings.setValue(QStringLiteral("ua/sessionID"), QString::fromStdString(resp.sessionID));
    settings.setValue(QStringLiteral("ua/sessionUser"), QString::fromStdString(resp.userName));
    settings.setValue(QStringLiteral("ua/sessionLevel"), resp.level);
    settings.setValue(QStringLiteral("ua/sessionLevelName"), QString::fromStdString(resp.levelName));

    // 记住账号密码（SuperAdmin 除外）：记住密码隐含记住账号；都不勾选则清空
    if (userName.compare(QStringLiteral("SuperAdmin"), Qt::CaseInsensitive) != 0)
    {
        UaLastLogin ll;
        ll.userName = (rememberUser || rememberPassword) ? userName.toStdString() : "";
        ll.password = rememberPassword ? password.toStdString() : "";
        g_uaClient->UASaveLastLoginAsync(g_uaSessionID, ll, [](ResponseNoDataMessageStruct &resp) -> ISTARResultStatus
        {
            return ISTAR_RESULT_SUCCESS;
        });
    }

    if (m_loginDialog && m_loginDialog->isVisible())
    {
        m_loginDialog->on_RecivedLoginInfo(0);
        m_loginDialog->accept();
    }
}

void MainWindow::doUaVisitorLogin()
{
    if (!g_uaClient)
    {
        g_uaClient = std::make_shared<UA_Client_Inherit>();
        g_uaClient->UAClientInit(ConfigRoot::MwConfigFile(), "ua");
    }
    // 服务端支持内置 Visitor 免密登录；异步非阻塞，结果由 sigVisitorLoginResult 回 GUI 线程
    LoginRequest req;
    req.userName = "Visitor";
    req.password = "";
    g_uaClient->UALoginAsync(req, [this](ResponseMessageStruct<LoginResponse> &resp) -> ISTARResultStatus
    {
        emit sigVisitorLoginResult((int)resp.errorCode, resp.data);
        return ISTAR_RESULT_SUCCESS;
    });
}

void MainWindow::onVisitorLoginResult(int status, LoginResponse resp)
{
    if (status != UA_LOGIN_OK)
    {
        // 服务端未就绪/登录失败：无会话显示空（不显示 Visitor），暂不置灰（容错，就绪心跳会重试）
        m_uaLevel = 0;
        updateUserInfo("", "");
        onSessionChanged();
        return;
    }
    g_uaSessionID = resp.sessionID;
    m_uaLevel = resp.level;
    PlatformModule::setUASession(resp.sessionID.c_str());
    updateUserInfo(resp.displayName, resp.levelName);
    onSessionChanged();
}

void MainWindow::doUaLogout()
{
    if (!g_uaClient || g_uaSessionID.empty())
    {
        return;
    }
    g_uaClient->UALogout(g_uaSessionID);
    g_uaSessionID.clear();
    m_uaLevel = 0;
    PlatformModule::setUASession("");
    updateUserInfo("", "");
    onSessionChanged();   // 会话已清：权限缓存失效，按当前级别重放

    // 登出后清除持久化会话，GUI 重启不再恢复
    QSettings settings;
    settings.remove(QStringLiteral("ua/sessionID"));
    settings.remove(QStringLiteral("ua/sessionUser"));
    settings.remove(QStringLiteral("ua/sessionLevel"));
    settings.remove(QStringLiteral("ua/sessionLevelName"));
}

void MainWindow::restoreOrVisitorLogin()
{
    QSettings settings;
    QString sid = settings.value(QStringLiteral("ua/sessionID")).toString();
    if (sid.isEmpty())
    {
        doUaVisitorLogin();
        return;
    }
    // 异步校验保存的会话（不阻塞 GUI 线程），结果由 sigSessionRestoreResult 回 GUI 线程
    std::string saved = sid.toStdString();
    g_uaClient->UAGetCurrentUserAsync(saved, [this, saved](ResponseMessageStruct<UserInfo> &resp) -> ISTARResultStatus
    {
        emit sigSessionRestoreResult((int)resp.errorCode, resp.data, QString::fromStdString(saved));
        return ISTAR_RESULT_SUCCESS;
    });
}

void MainWindow::onSessionRestoreResult(int status, UserInfo user, const QString &sessionID)
{
    if (status == UA_SESSION_OK && user.level > 0)
    {
        // 服务端在且会话有效：恢复真实用户（Visitor 是默认态，不持久恢复）
        g_uaSessionID = sessionID.toStdString();
        m_uaLevel = user.level;
        PlatformModule::setUASession(sessionID.toStdString().c_str());
        QSettings settings;
        QString levelName = settings.value(QStringLiteral("ua/sessionLevelName")).toString();
        updateUserInfo(user.displayName, levelName.toStdString());
        onSessionChanged();
        return;
    }
    // 会话为 Visitor(level==0) 或已失效(INVALID)：清持久化，转默认 Visitor 登录
    if (status == UA_SESSION_OK || status == UA_SESSION_INVALID)
    {
        QSettings settings;
        settings.remove(QStringLiteral("ua/sessionID"));
        settings.remove(QStringLiteral("ua/sessionUser"));
        settings.remove(QStringLiteral("ua/sessionLevel"));
        settings.remove(QStringLiteral("ua/sessionLevelName"));
    }
    // 其它（服务端未启动等传输错误）：保留持久化，等就绪心跳重试恢复
    doUaVisitorLogin();
}

void MainWindow::registerMachineModules()
{
    if (!g_uaClient)
    {
        g_uaClient = std::make_shared<UA_Client_Inherit>();
        g_uaClient->UAClientInit(ConfigRoot::MwConfigFile(), "ua");
    }
    ModuleList moduleList;
    for (const auto &m : kMachineModules)
    {
        // 模块显示名动态 Tr（kMachineModules 为静态初始化，不能在此前求值 Tr）
        UaModule mod = m;
        mod.moduleName = Tr(mod.moduleName);
        moduleList.modules.push_back(mod);
    }
    // 异步、非阻塞：服务端未启动时静默失败，就绪心跳会在"未就绪→就绪"跃迁时补注册
    g_uaClient->UARegisterModulesAsync(moduleList, [](ResponseNoDataMessageStruct &resp) -> ISTARResultStatus
    {
        return ISTAR_RESULT_SUCCESS;
    });
}

void MainWindow::onUaServerReady(bool ready)
{
    // 模块树注册在"未就绪→就绪"跃迁时重试：
    // GUI 先于控制层启动时构造里的注册会静默失败，这里补注册整机导航模块
    if (ready && !m_uaServerReady)
    {
        m_uaServerReady = true;
        registerMachineModules();
    }
    else if (!ready)
    {
        m_uaServerReady = false;
    }

    // 无会话 → 异步恢复上次会话或默认 Visitor 登录（覆盖 GUI 先启动场景，不阻塞 GUI 线程）
    if (g_uaSessionID.empty())
    {
        restoreOrVisitorLogin();
        return;
    }

    // 有会话 → 心跳校验：控制层重启后会话失效则复位 Visitor 并自动重登
    UserInfo user;
    UaSessionStatus st = (UaSessionStatus)g_uaClient->UAGetCurrentUser(g_uaSessionID, user);
    if (st == UA_SESSION_OK)
    {
        return;
    }
    if (st == UA_SESSION_INVALID)
    {
        g_uaSessionID.clear();
        m_uaLevel = 0;
        PlatformModule::setUASession("");
        updateUserInfo("", "");
        onSessionChanged();   // 会话已清：权限缓存失效，按当前级别重放
        doUaVisitorLogin();
    }
    // 其它状态（传输抖动等）忽略
}

void MainWindow::refreshPermissionCache()
{
    m_uaOperable.clear();
    m_uaAdmin = (m_uaLevel >= 9);
    m_uaSuperAdmin = (m_uaLevel >= 10);   // 超级管理员：IO 页可见控制层内部号段
    m_uaPermValid = (g_uaClient && !g_uaSessionID.empty());
    if (!m_uaPermValid || m_uaAdmin)
    {
        return;   // 无会话 = 缓存无效（applyUiEnable 按全通过处理）；管理员无需模块表
    }

    ModuleAccessList accs;
    UaSessionStatus st = (UaSessionStatus)g_uaClient->UAGetSessionModules(g_uaSessionID, accs);
    if (st == UA_SESSION_OK)
    {
        for (const auto &a : accs.modules)
        {
            if (a.operable)
            {
                m_uaOperable.insert(a.moduleID);
            }
        }
    }
    // 查询失败：缓存有效但集合为空 → 非管理员全锁（不误开）
}

// 级别入口：整机状态变化时调（onStatusChanged 的 switch 决定级别）
void MainWindow::setUiLockLevel(int level)
{
    m_uiLockLevel = qBound(1, level, 3);   // 非法值收敛（保守）
    applyUiEnable();
}

// 会话入口：登录成功/登出/会话失效统一走这里（权限取数是同步 RPC，只有这里调）
void MainWindow::onSessionChanged()
{
    refreshPermissionCache();
    applyUiEnable();

    // IO 页：仅超级管理员（level 10）可见控制层内部号段，其余用户按权限过滤
    if (m_pIoPage)
    {
        m_pIoPage->setSuperAdmin(m_uaSuperAdmin);
    }
}

// 整区开关：一级导航按钮 + 区容器 + 全部二级导航按钮
//（容器关 = 整区子树不可交互；不碰子控件自身状态，重开自然还原）
void MainWindow::setAreaEnabled(ISTAR::Framework::FunctionArea *area,
                                ISTAR::Framework::NaviButton *naviBtn, bool on)
{
    if (naviBtn)
    {
        naviBtn->setEnabled(on);
    }
    if (!area)
    {
        return;
    }
    for (auto *b : subNaviButtons(area))
    {
        b->setEnabled(on);
    }
    area->setEnabled(on);
}

// 唯一落地：级别（资格）× 权限（许可）→ 一级导航/区容器/二级导航/登录按钮。
// 状态只管资格：状态页 2 级起、其余 3 级起；1 级（UNKNOWN/LOADING/LOAD_FAIL/DISCONNECTED）
// 什么门槛都不满足 → 全灰、无视权限。资格够了亮灭全由权限说了算。
// 告警区与 Exit/语言、LED 同级常开；页面内容由各页自己的 setStatus 自锁。
void MainWindow::applyUiEnable()
{
    struct GatedArea
    {
        ISTAR::Framework::FunctionArea *area;
        ISTAR::Framework::NaviButton *naviBtn;
        const char *moduleID;
    };
    // 受级别 + 权限管理的区域（告警区常开，不在此表）
    const GatedArea areas[] = {
        {m_pJobsArea, ui->btn_Jobs, "Job"},
        {m_pSystemArea, ui->btn_System, "System"},
        {m_pRecipesArea, ui->btn_Recipes, "Recipes"},
        {m_pDataArea, ui->btn_DataLog, "Data"},
        {m_pSetupArea, ui->btn_Setup, "Setup"},
        {m_pMetrologyArea, ui->btn_Metrology, "Metrology"},
        {m_pHelpArea, ui->btn_Help, "Help"},
    };

    // 常开：告警区；登录按钮 2 级起
    setAreaEnabled(m_pAlarmsArea, ui->btn_Alarms, true);
    ui->btn_Login->setEnabled(m_uiLockLevel >= 2);

    const bool permAll = !m_uaPermValid || m_uaAdmin;   // 管理员/无会话：权限全通过
    for (const auto &am : areas)
    {
        // 该区二级子模块ID（顺序与 addFunctionPage 一致，按位置对应按钮）
        std::vector<std::string> subIds;
        for (const auto &m : kMachineModules)
        {
            if (m.parentID == am.moduleID)
            {
                subIds.push_back(m.moduleID);
            }
        }

        bool anyEnabled = false;
        const auto buttons = subNaviButtons(am.area);
        for (int i = 0; i < (int)buttons.size(); ++i)
        {
            // 资格门槛：状态页（Job 区 Job_Status）2 级起，其余 3 级起
            const bool statusPage = (qstrcmp(am.moduleID, "Job") == 0 &&
                                     i < (int)subIds.size() && subIds[i] == "Job_Status");
            const int need = statusPage ? 2 : 3;
            // 许可：已注册模块按 operable；未注册按钮仅管理员/无会话（旧版行为）
            const bool allowed = permAll ||
                                (i < (int)subIds.size() && m_uaOperable.count(subIds[i]) > 0);
            const bool enable = (m_uiLockLevel >= need) && allowed;
            buttons[i]->setEnabled(enable);
            anyEnabled = anyEnabled || enable;
        }
        // 一级导航与区容器 = 任一二级可达
        if (am.naviBtn)
        {
            am.naviBtn->setEnabled(anyEnabled);
        }
        if (am.area)
        {
            am.area->setEnabled(anyEnabled);
        }
    }
}

bool MainWindow::isVisitorOrNoSession() const
{
    return g_uaSessionID.empty() || m_uaLevel == 0;
}

void MainWindow::updateUserInfo(const std::string &userName, const std::string &levelName)
{
    ui->led_UserName->setText(QString::fromStdString(userName));
    ui->led_UserLevel->setText(QString::fromStdString(levelName));
}
