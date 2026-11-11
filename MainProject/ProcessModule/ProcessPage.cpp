#include "ProcessPage.h"
#include "ui_ProcessPage.h"

#include "MCRecipeModule/MCRecipeManager.h"
#include "Common/RecipeChooseWidget.h"
#include "dialog/messagedialog.h"
#include "istar_serializer.hpp"
#include "istar_language.h"
#include "istar_ui_stylesheet.h"

#include <QAbstractItemView>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>

using namespace ISTAR::GUI;
using namespace ISTAR::UIStyle;

ProcessPage::ProcessPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ProcessPage)
    , m_pRecipeDlg(new RecipeChooseWidget(this))
{
    ui->setupUi(this);

    initStyle();
    initUi();
    createProcesses();
    initConnect();
    // 翻译由外部（MainWindow）在语言初始化 / 切换时触发，不自行调用
}

ProcessPage::~ProcessPage()
{
    delete ui;
}

// ---------------------------------------------------------------------------
// 流程注册（新增流程改这里）
// ---------------------------------------------------------------------------

template<typename ProcT>
void ProcessPage::registerProcess(ProcessID id)
{
    const int idx = int(id);
    QWidget *tab = ui->tabWidget->widget(idx);   // 越界返回 nullptr（无 tab 流程如 Init）
    m_procs[idx] = std::make_unique<ProcT>(idx, tab, ui, this);

    // 注入 GUI → mctrl 上报回调：process 固定为本流程 id，command / json 由流程给
    m_procs[idx]->SetMctrlReporter([idx](int command, const std::string &json)
    {
        MctrlModule::ProcessMessage msg;
        msg.process = idx;
        msg.command = command;
        msg.json = json;
        // 异步发送：服务端处理时长（如 GetLastResult ~1.5s）不再阻塞 GUI 线程；
        // 应答仍走观察者广播到各流程 ReceiveMctrlCommand，路径不变
        MCTRLClientManager::getInstance()->getClient()->GuiToMctrlCommandAsync(
                    msg, [](ResponseNoDataMessageStruct &)
        {
            return ISTAR_RESULT_SUCCESS;
        });
    });
}

void ProcessPage::createProcesses()
{
    registerProcess<BlueFilmChipHandoverProc>(ProcessID::SingleProc_BlueFilmChipHandover);
    registerProcess<ChuckLevelProc>(ProcessID::SingleProc_ChuckLevel);
    registerProcess<WaferLevelCollectProc>(ProcessID::SingleProc_WaferLevelCollect);
    registerProcess<ChipPosCalibProc>(ProcessID::SingleProc_ChipPosCalib);
    registerProcess<CoarseLevelProc>(ProcessID::SingleProc_CoarseLevel);
    registerProcess<DownLevelSelectProc>(ProcessID::SingleProc_DownLevelSelect);
    registerProcess<UpLevelSelectProc>(ProcessID::SingleProc_UpLevelSelect);
    registerProcess<FineLevelProc>(ProcessID::SingleProc_FineLevel);
    registerProcess<FineAlignProc>(ProcessID::SingleProc_FineAlign);
    registerProcess<BondProc>(ProcessID::SingleProc_Bond);
}

// ---------------------------------------------------------------------------
// 状态（订阅 StatusManager，常改区）
// ---------------------------------------------------------------------------

void ProcessPage::onStatusChanged(MctrlModule::MctrlStatus prev, MctrlModule::MctrlStatus curr)
{
    refreshRunControls(prev, curr);

    // 广播给全部流程（只由真实状态信号驱动；Response 恢复不走这里，避免重复广播）
    for (const auto &kv : m_procs)
    {
        if (kv.second != nullptr)
        {
            kv.second->setStatus(curr);
        }
    }
}

void ProcessPage::refreshRunControls(MctrlModule::MctrlStatus prev, MctrlModule::MctrlStatus curr)
{
    switch (curr)
    {
        case MCTRL_STATUS_DISCONNECTED:
        case MCTRL_STATUS_UNKNOWN:
        case MCTRL_STATUS_LOADING:
        case MCTRL_STATUS_LOAD_FAIL:
        case MCTRL_STATUS_LOAD_OK:
        case MCTRL_STATUS_INITIALIZING:
        case MCTRL_STATUS_INI_FAIL:
            if (prev == MCTRL_STATUS_DISCONNECTED)
            {
                m_isSelectRecipe = false;
                ui->le_repcipeName->setText("");
                ui->pb_Process->setVisible(false);
                ui->btn_Stop->setEnabled(false);
                ui->btn_Start->setEnabled(false);
                ui->btn_recipe->setEnabled(true);
            }
            break;

        case MCTRL_STATUS_IDLE:
            ui->pb_Process->setVisible(false);
            ui->btn_Stop->setEnabled(false);
            ui->btn_Start->setEnabled(m_isSelectRecipe);
            ui->btn_recipe->setEnabled(true);
            break;

        case MCTRL_STATUS_BUSY:
            ui->pb_Process->setVisible(true);
            ui->pb_Process->setValue(0);
            ui->btn_Stop->setEnabled(true);
            ui->btn_Start->setEnabled(false);
            ui->btn_recipe->setEnabled(false);
            break;

        case MCTRL_STATUS_ALARM:
            ui->btn_Stop->setEnabled(false);
            ui->btn_Start->setEnabled(false);
            ui->btn_recipe->setEnabled(false);
            break;
    }
}

// ---------------------------------------------------------------------------
// 控制层消息（常改区）
// ---------------------------------------------------------------------------

void ProcessPage::onMctrlMessage(const MctrlModule::ProcessMessage &data)
{
    // 蓝膜查询的答复：只有发起本次查询的页面处理"被拒"（弹错，与原来同一个框）；
    // 其它页面静默 —— 谁发谁报错
    if (data.process == static_cast<int>(ProcessID::SingleProc_BlueFilmChipHandover))
    {
        if (data.command == BlueFilmChipHandoverCMD_CmdRejected)
        {
            if (!m_blueFilmReplyPending)
            {
                return;
            }
            m_blueFilmReplyPending = false;
            const S_BlueFilmChipHandoverCMD_CmdRejected res =
                    Serializer::from_rawJson<S_BlueFilmChipHandoverCMD_CmdRejected>(data.json);
            MessageDialog dlg(MessageDialog::Error, this, QString::fromStdString(res.reason));
            dlg.exec();
            return;
        }
        if (data.command == BlueFilmChipHandoverCMD_AllChipsRes)
        {
            m_blueFilmReplyPending = false;   // 本页的查询有答复了（数据本身仍交给流程刷图）
        }
    }

    // 流程消息：按流程 id 路由
    const auto it = m_procs.find(data.process);
    if (it == m_procs.end())
    {
        return;
    }

    // 任务层命令：manager 自己消化，不下发流程
    switch (data.command)
    {
        case ProcessManagerCMD_Response:
        {
            S_ProcessManagerResponseData res =
                    Serializer::from_rawJson<S_ProcessManagerResponseData>(data.json);
            if (!res.success)
            {
                const MctrlModule::MctrlStatus s = StatusManager::instance()->status();
                refreshRunControls(s, s);
                MessageDialog dlg(MessageDialog::Error, this,
                                QString::fromStdString(res.errorInfo));
                dlg.exec();
            }
            return;
        }

        case ProcessManagerCMD_SingleProcFinished:
        {
            S_SingleProcFinishedData res =
                    Serializer::from_rawJson<S_SingleProcFinishedData>(data.json);

            const auto type = (res.status == 0)
                                 ? MessageDialog::Success : MessageDialog::Error;
            const std::string msg = (res.status == 0) ? Tr("Success") : Tr(res.reason);
            MessageDialog dlg(type, this, QString::fromStdString(msg));
            dlg.exec();

            // 任务层事件也转给本流程的 tab：各 tab 按 command 自取所需、认不出就忽略
            //（如 BondProc 用它给键合曲线收尾）。这里只多一次转发，不改任何 tab 的既有行为。
            it->second->ReceiveMctrlCommand(data);
            return;
        }

        case ProcessManagerCMD_Progress:
        {
            S_ProcessProgressData res =
                    Serializer::from_rawJson<S_ProcessProgressData>(data.json);
            ui->pb_Process->setValue(res.percent);
            return;
        }

        default:
            break;
    }

    it->second->ReceiveMctrlCommand(data);
}

// ---------------------------------------------------------------------------
// 运行控制
// ---------------------------------------------------------------------------

void ProcessPage::startCurrentProcess()
{
    const auto it = m_procs.find(ui->tabWidget->currentIndex());
    if (it == m_procs.end())
    {
        return;
    }

    ui->btn_Stop->setEnabled(false);
    ui->btn_Start->setEnabled(false);
    ui->btn_recipe->setEnabled(false);

    it->second->clearData();
    sendManagerCommand(it->first, ProcessManagerCMD_Start);
}

void ProcessPage::stopCurrentProcess()
{
    ui->btn_Stop->setEnabled(false);
    ui->btn_Start->setEnabled(false);
    ui->btn_recipe->setEnabled(false);

    // 控制层会关闭所有流程，这里就不针对某个流程关闭了。gui不再记录当前操作的流程
    sendManagerCommand(0, ProcessManagerCMD_Stop);
}

void ProcessPage::sendManagerCommand(int processId, int command)
{
    MctrlModule::ProcessMessage msg;
    msg.process = processId;
    msg.command = command;
    MCTRLClientManager::getInstance()->getClient()->GuiToMctrlCommand(msg);
}

// ---------------------------------------------------------------------------
// 处方
// ---------------------------------------------------------------------------

void ProcessPage::onRecipeUpdated()
{
    MCRecipeManager *mgr = MCRecipeManager::instance();
    ui->le_repcipeName->setText(mgr->reName());
    ui->le_repcipeDescription->setText(mgr->reDescription());

    const S_ChipRecipe &chip = mgr->chipRecipe();
    const S_RecipeContentWafer &wafer = mgr->waferRecipe();
    ui->dsb_upChipHeight->setValue(chip.material.upChipMaterialInfo.chipHeight);
    ui->dsb_upChipWidth->setValue(chip.material.upChipMaterialInfo.chipWidth);
    ui->dsb_blueWaferSize->setValue(chip.material.blueWaferInfo.waferSize);
    ui->dsb_blueWaferNotchOrient->setValue(chip.material.blueWaferInfo.waferNotchOrient);
    ui->dsb_expansionDepth->setValue(chip.material.blueWaferInfo.expansionDepth);
    ui->dsb_dwChipHeight->setValue(chip.material.dwChipMaterialInfo.chipHeight);
    ui->dsb_dwChipWidth->setValue(chip.material.dwChipMaterialInfo.chipWidth);
    ui->dsb_waferSize->setValue(wafer.material.oneChipTypeInfo.waferSize);
    ui->dsb_waferThickness->setValue(wafer.material.oneChipTypeInfo.waferThickness);

    for (const auto &kv : m_procs)
    {
        if (kv.second != nullptr)
        {
            kv.second->updateRecipe();
        }
    }
    refreshTexts();
}

void ProcessPage::requestBlueFilmChips()
{
    MctrlModule::ProcessMessage msg;
    msg.process = static_cast<int>(ProcessID::SingleProc_BlueFilmChipHandover);
    msg.command = BlueFilmChipHandoverCMD_GetAllChips;
    // 异步发送：不阻塞 GUI 线程；应答走观察者广播（onMctrlMessage 已接）
    MCTRLClientManager::getInstance()->getClient()->GuiToMctrlCommandAsync(
                msg, [](ResponseNoDataMessageStruct &)
    {
        return ISTAR_RESULT_SUCCESS;
    });
}

void ProcessPage::onMcUpdated()
{

    for (const auto &kv : m_procs)
    {
        if (kv.second != nullptr)
        {
            kv.second->updateMc();
        }
    }
    refreshTexts();
}
// ---------------------------------------------------------------------------
// 翻译
// ---------------------------------------------------------------------------

void ProcessPage::refreshTexts()
{
    m_uiTranslator.refresh(this);
}

// ---------------------------------------------------------------------------
// 以下基本定型
// ---------------------------------------------------------------------------

void ProcessPage::initUi()
{
    // 处方预览 tab 的分组框标题
    ui->gb_upChipInfo->SetHeaderText(QStringLiteral("Up Chip Info"));
    ui->gb_blueWaferInfo->SetHeaderText(QStringLiteral("Blue Film Wafer Info"));
    ui->gb_dwChipInfo->SetHeaderText(QStringLiteral("Down Chip Info"));
    ui->gb_dwWaferInfo->SetHeaderText(QStringLiteral("Down Wafer Info"));
    ui->gb_BondingParam->SetHeaderText(QStringLiteral("Bond Parameters"));

    // 处方预览 tab：数值框和输入框只读
    QWidget *preview = ui->tabWidget->findChild<QWidget *>(QStringLiteral("RecipePreview"));
    if (preview == nullptr)
    {
        return;
    }
    for (QDoubleSpinBox *dsb : preview->findChildren<QDoubleSpinBox *>())
    {
        dsb->setReadOnly(true);
    }
    for (QLineEdit *le : preview->findChildren<QLineEdit *>())
    {
        le->setReadOnly(true);
    }
}

void ProcessPage::initConnect()
{
    connect(ui->btn_Start, &QPushButton::clicked, this, [this]()
    {
        startCurrentProcess();
    });

    connect(ui->btn_Stop, &QPushButton::clicked, this, [this]()
    {
        stopCurrentProcess();
    });

    connect(ui->btn_recipe, &QPushButton::clicked, this, [this]()
    {
        QStringList recipes;
        MCRecipeManager::instance()->releasedNameList(recipes);
        m_pRecipeDlg->SetLst(recipes);
        m_pRecipeDlg->exec();
    });

    connect(m_pRecipeDlg, &RecipeChooseWidget::sig_recipeChosen, this, [this](const QString &name)
    {
        QString err;
        if (!MCRecipeManager::instance()->selectRecipe(name, err) && !err.isEmpty())
        {
            MessageDialog dlg(MessageDialog::Error, this,
                              QString::fromStdString(Tr(err.toStdString())));
            dlg.exec();
        }
        else
        {
            m_isSelectRecipe = true;
            ui->btn_Start->setEnabled(true);

            // 本页选中的处方：只有本页去查蓝膜现状（其它页面不被牵连）；
            // 本次查询的答复（含被拒）也由本页处理
            m_blueFilmReplyPending = true;
            requestBlueFilmChips();
        }
    });

    // 处方刷新：单例内容变化时广播一次，各页各刷自己一次显示（切页不再补刷）。
    // Queued：emit 在弹窗 accept() 之前发出，排到事件循环后弹窗已关，不冻窗。
    // 回调里只刷显示 —— 蓝膜查询由"选中处方的那一页"自己发，这里不做也不弹窗。
    connect(MCRecipeManager::instance(), &MCRecipeManager::sigRecipeUpdated, this, [this]()
    {
        onRecipeUpdated();
    }, Qt::QueuedConnection);

    connect(MCRecipeManager::instance(), &MCRecipeManager::sigMcUpdated, this, [this]()
    {
        onMcUpdated();
    });

    connect(StatusManager::instance(), &StatusManager::sigStatusChanged, this,
            [this](MctrlModule::MctrlStatus prev, MctrlModule::MctrlStatus curr)
    {
        onStatusChanged(prev, curr);
    });

    connect(MCTRLClientManager::getInstance(), &MCTRLClientManager::sig_ProcessObserverData, this,
            [this](const MctrlModule::ProcessMessage &data)
    {
        onMctrlMessage(data);
    });
}

void ProcessPage::initStyle()
{
    QList<GroupBox*> lst_gb = this->findChildren<GroupBox*>();
    for (auto &i : lst_gb)
    {
        i->setStyleSheet(iStarUIStyleSheet->GroupBoxStyle + iStarUIStyleSheet->DoubleSpinBox_Dark);
        i->SetheaderFontSize(12);
    }

    ui->sideGroup->setStyleSheet(iStarUIStyleSheet->GroupBoxStyle);
    ui->sideGroup->SetDecorateColor("transparent");
    ui->sideGroup->SetHeaderText("");

    QList<QPushButton*> lst_btn = this->findChildren<QPushButton*>();
    for(auto &b : lst_btn)
    {
        b->setStyleSheet(iStarUIStyleSheet->Button_Border);
        if (m_pRecipeDlg && m_pRecipeDlg->isAncestorOf(b))
        {
            continue;
        }
        b->setEnabled(false);
    }
    ui->btn_recipe->setEnabled(true);

    const QString btnStyle = QString(
        "QPushButton{background-color:#FFFFFF;color:#808080;border-radius:5px;}"
        "QPushButton:hover{border:2px solid #A3B6FF;color:#A3B6FF;}"
        "QPushButton:pressed{border:2px solid #4C70FA;color:#4C70FA;}"
        "QPushButton:checked{border:2px solid #4C70FA;color:#4C70FA;}"
        "QPushButton:disabled{color:#E0E0E0;}")
        + iStarUIStyleSheet->fontStyle_bold_12;
    ui->btn_Start->setStyleSheet(btnStyle);
    ui->btn_Stop->setStyleSheet(btnStyle);

    ui->pb_Process->setRange(0, 100);
    ui->pb_Process->setValue(0);
    ui->pb_Process->setTextVisible(false);
    ui->pb_Process->setStyleSheet(
        "QProgressBar{background-color:#FFFFFF;border:none;border-radius:4px;}"
        "QProgressBar::chunk{background-color:#4C70FA;border-radius:4px;}");
    ui->pb_Process->setVisible(false);

    ui->btn_Stop->setEnabled(false);

    ui->tabWidget->setStyleSheet(iStarUIStyleSheet->TabWidget + iStarUIStyleSheet->fontStyle_normal_12
                                 + QStringLiteral("QTabBar::tab{width:130;}")
                                 + QStringLiteral("QTabBar::tab{font-size:10pt;}"));

    QList<QDoubleSpinBox*> lst_dsb = this->findChildren<QDoubleSpinBox*>();
    for (auto &dsb : lst_dsb)
    {
        dsb->setStyleSheet(iStarUIStyleSheet->DoubleSpinBox_Dark);
    }

    QList<QLineEdit*> lst_le = this->findChildren<QLineEdit*>();
    for (auto &le : lst_le)
    {
        le->setStyleSheet(iStarUIStyleSheet->LineEdit_Dark + iStarUIStyleSheet->fontStyle_normal_12);
    }

    ui->le_repcipeName->setStyleSheet(iStarUIStyleSheet->LineEdit + iStarUIStyleSheet->fontStyle_normal_12);
    ui->le_repcipeDescription->setStyleSheet(iStarUIStyleSheet->LineEdit + iStarUIStyleSheet->fontStyle_normal_12);

    QList<QLabel*> lst_label = this->findChildren<QLabel*>();
    for (auto &l : lst_label)
    {
        l->setStyleSheet(iStarUIStyleSheet->fontStyle_normal_12);
    }

    // 蓝色装饰条（平台 GroupBox 的 SetDecorateColor 在本 DLL 版本不渲染，手动补）
    QList<GroupBox*> lst_gbStrip = this->findChildren<GroupBox*>();
    for (auto *gb : lst_gbStrip)
    {
        if (gb == ui->sideGroup) continue;
        QLabel *strip = new QLabel(gb);
        strip->setFixedSize(5, 24);
        strip->setStyleSheet("background-color:#4C70FA;border:none;");
        strip->move(0, 12);
    }
}
