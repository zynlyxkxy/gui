#include "AutoProcessPage.h"
#include "ui_AutoProcessPage.h"
#include "Common/RecipeChooseWidget.h"
#include "Common/UiTranslator.h"          // TrKeyRole（键合表动态格的翻译 key）
#include "Common/BondCurveRecorder.h"     // 键合曲线采集器（订阅 IO 观察者）
#include "istar_language.h"               // Tr()：值/名字文字按当前语言取
#include "MCRecipeModule/MCRecipeManager.h"
#include "istar_ui_stylesheet.h"
#include "istar_enumconverter.hpp"
#include "dialog/messagedialog.h"
#include "groupbox/groupbox.h"
#include "istar_serializer.hpp"
#include "RPCModule/MCTRLClientManager.h"
#include "ProcessModule/Protocol/AutoProcessProtocol.h"
#include "ProcessModule/Protocol/ProcessManagerProtocol.h"
#include "ProcessModule/Protocol/BlueFilmChipHandoverProtocol.h"   // ChipPosition 全量快照载荷

#include <QShowEvent>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>


using namespace ISTAR::UIStyle;


AutoProcessPage::AutoProcessPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AutoProcessPage),
    m_pRecipeDlg(new RecipeChooseWidget(this))
{
    ui->setupUi(this);


    initConnect();
    initStyle();
    ensureChart();

    // 键合曲线采集器：窗口由自动链的 CurProcess(Bond) / Finished / IDLE 驱动
    m_curve = new BondCurveRecorder(p_Chart, this);
}

AutoProcessPage::~AutoProcessPage()
{
    delete ui;
}

void AutoProcessPage::refreshTexts()
{
    m_uiTranslator.refresh(this);
}

void AutoProcessPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // 图表首次显示/每次切到本页时补一次重画：QCustomPlot 的轴矩形是按当前 label 尺寸排布的，
    // 构造期控件还没有真实尺寸，那时 replot 出来的布局会把轴标签里带单位的后缀（(s)/(℃)/(N)）
    // 挤掉；等真正显示后再刷一次，标签才完整。
    if (p_Chart != nullptr)
    {
        p_Chart->replot(QCustomPlot::rpQueuedReplot);
    }
}

void AutoProcessPage::refreshDownWaferDiagram(QPointF pos, DownWaferDiagram::ChipState state)
{
    ui->gv_downWaferStatus->setChipState(pos, state);
}


void AutoProcessPage::initDiagram()
{
    m_bondInfo.clear();                       // 换处方/重进页面：先清上一份
    for (const S_Point2D &p : MCRecipeManager::instance()->waferRecipe().material.oneChipTypeInfo.lst_nominalPoints)
    {
        m_bondInfo[QPointF(p.x_, p.y_)] = DownWaferDiagram::ChipBondInfo{};
    }

    // 蓝膜芯片图：尺寸随处方（下芯片），点位走全量快照（AllChipsRes 订阅见 initConnect）
    ui->gv_blueWaferStatus->setChipSize(MCRecipeManager::instance()->chipRecipe().material.upChipMaterialInfo.chipWidth,
                                        MCRecipeManager::instance()->chipRecipe().material.upChipMaterialInfo.chipHeight);

    ui->gv_downWaferStatus->setChips(MCRecipeManager::instance()->waferRecipe().material.oneChipTypeInfo.lst_nominalPoints,
                                     MCRecipeManager::instance()->chipRecipe().material.dwChipMaterialInfo.chipWidth,
                                     MCRecipeManager::instance()->chipRecipe().material.dwChipMaterialInfo.chipHeight);

    ui->gv_downWaferStatus->setChipState(MCRecipeManager::instance()->waferRecipe().bondPointSelection.lst_bondNominalPoint,
                                         DownWaferDiagram::ChipState::Selected);

    ui->gv_downWaferStatus->setBondInfoProvider([this](const QPointF &chipPos)
    {
        DownWaferDiagram::ChipBondInfo info;
        const auto it = m_bondInfo.find(chipPos);
        if (it != m_bondInfo.end())
        {
            info.count = it->second.count;
            info.times = it->second.times;
        }
        return info;
    });

}
void AutoProcessPage::initConnect()
{
    // 曲线纵轴显隐：勾选框 → 采集器（它记住状态，重设量程时会把隐藏的轴补回去）
    QObject::connect(ui->ckb_upChuckTemperature, &QCheckBox::toggled, this, [this](bool b)
    {
        m_curve->setCurveVisible(QString::fromLatin1(BondCurve::kAxisUpTemp), b);
    });
    QObject::connect(ui->ckb_downChuckTemperature, &QCheckBox::toggled, this, [this](bool b)
    {
        m_curve->setCurveVisible(QString::fromLatin1(BondCurve::kAxisDownTemp), b);
    });
    QObject::connect(ui->ckb_bondForce, &QCheckBox::toggled, this, [this](bool b)
    {
        m_curve->setCurveVisible(QString::fromLatin1(BondCurve::kAxisForce), b);
    });

    //处方弹窗
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
            sendManagerCommand(static_cast<int>(ProcessID::SingleProc_BlueFilmChipHandover),
                               BlueFilmChipHandoverCMD_GetAllChips);
        }
    });

    // 处方刷新：单例内容变化时广播一次，各页各刷自己一次显示（切页不再补刷）。
    // Queued：emit 在弹窗 accept() 之前发出，排到事件循环后弹窗已关，不冻窗。
    // 回调里只刷显示（onRecipeUpdated 里会 initDiagram）—— 蓝膜查询由选中处方的那页自己发。
    connect(MCRecipeManager::instance(), &MCRecipeManager::sigRecipeUpdated, this, [this]()
    {
        onRecipeUpdated();
    }, Qt::QueuedConnection);

    connect(ui->btn_Start, &QPushButton::clicked, this, [this]()
    {
        startAutoProcess();
    });

    connect(ui->btn_Stop, &QPushButton::clicked, this, [this]()
    {
        stopAutoProcess();
    });

    connect(StatusManager::instance(), &StatusManager::sigStatusChanged, this,
            [this](MctrlModule::MctrlStatus prev, MctrlModule::MctrlStatus curr)
    {
        refreshRunControls(prev, curr);
    });

    connect(MCTRLClientManager::getInstance(), &MCTRLClientManager::sig_ProcessObserverData, this,
            [this](const MctrlModule::ProcessMessage &data)
    {
        onMctrlMessage(data);
    });
}

void AutoProcessPage::startAutoProcess()
{
    ui->btn_Stop->setEnabled(false);
    ui->btn_Start->setEnabled(false);
    ui->btn_recipe->setEnabled(false);

    sendManagerCommand(static_cast<int>(ProcessID::AutoProc_AutoProcess), ProcessManagerCMD_Start);
}

void AutoProcessPage::stopAutoProcess()
{
    ui->btn_Stop->setEnabled(false);
    ui->btn_Start->setEnabled(false);
    ui->btn_recipe->setEnabled(false);

    sendManagerCommand(static_cast<int>(ProcessID::AutoProc_AutoProcess), ProcessManagerCMD_Stop);
}

void AutoProcessPage::sendManagerCommand(int processId, int command)
{
    MctrlModule::ProcessMessage msg;
    msg.process = processId;
    msg.command = command;
    // 异步发送：不阻塞 GUI 线程；应答走 sig_ProcessObserverData 广播（onMctrlMessage 已接）
    MCTRLClientManager::getInstance()->getClient()->GuiToMctrlCommandAsync(
                msg, [](ResponseNoDataMessageStruct &)
    {
        return ISTAR_RESULT_SUCCESS;
    });
}

void AutoProcessPage::refreshRunControls(MctrlModule::MctrlStatus prev, MctrlModule::MctrlStatus curr)
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
            // 兜底收尾：整机回 IDLE = 任务结束。自动链正常收尾/异常失败都会走
            // AutoProcessCMD_Finished；但任务被 manager 停掉时只置 IDLE、不发 Finished，
            // 这里补一刀，避免曲线窗口一直开着。
            if (m_curve != nullptr)
            {
                m_curve->finishSession();
            }
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

void AutoProcessPage::onMctrlMessage(const MctrlModule::ProcessMessage &data)
{
    // 蓝膜查询被拒：只有发起本次查询的页面弹错（其它页面静默），弹框与原来一致
    if (data.process == static_cast<int>(ProcessID::SingleProc_BlueFilmChipHandover) &&
        data.command == BlueFilmChipHandoverCMD_CmdRejected)
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

    // 蓝膜芯片全量快照：喂给本页蓝膜图（广播信号，各页面只挑自己要的）
    if (data.process == static_cast<int>(ProcessID::SingleProc_BlueFilmChipHandover) &&
        data.command == BlueFilmChipHandoverCMD_AllChipsRes)
    {
        m_blueFilmReplyPending = false;   // 本页的查询有答复了
        const S_BlueFilmChipPositionData res =
                Serializer::from_rawJson<S_BlueFilmChipPositionData>(data.json);

        // 未就绪（蓝膜台还没上报过任何快照）：清图，不弹窗 —— 正常状态，不是错误
        if (res.state == static_cast<int>(BlueFilmChipDataState_NotReady))
        {
            ui->gv_blueWaferStatus->setPoints({});
            return;
        }

        ui->gv_blueWaferStatus->setPoints(res.chipPos);
        return;
    }

    if(data.process != static_cast<int>(ProcessID::AutoProc_AutoProcess)){
        return;
    }

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

        case ProcessManagerCMD_Progress:
        {
            S_ProcessProgressData res =
                    Serializer::from_rawJson<S_ProcessProgressData>(data.json);
            ui->pb_Process->setValue(res.percent);
            return;
        }

        case AutoProcessCMD_CurProcess:
        {
            S_AutoProcessCurProcessData res =
                    Serializer::from_rawJson<S_AutoProcessCurProcessData>(data.json);

            const QString taskKey =
                QString::fromStdString(ISTAREnumConverter::ConvertIntToString<ProcessID>(res.processId))
                        .section(QLatin1Char('_'), -1);
            ui->lbl_curTaskName->setProperty("trKey", taskKey);                        // 语言切换时原地重译
            ui->lbl_curTaskName->setText(QString::fromStdString(Tr(taskKey.toStdString())));  // 写入即翻译

            m_curChip = QPointF(res.posX,res.posY);
            refreshDownWaferDiagram(m_curChip,DownWaferDiagram::ChipState::Bonding);

            // 键合曲线窗口：processId 语义 = 即将执行的流程（上报在置 Ready 之前），
            // 所以"下一道是键合"= 本颗开始，"下一道不是键合"= 本颗结束。
            // 开始走 startSession（内部先清图、再开窗），结束只关窗（图上数据定格）。
            if (m_curve != nullptr)
            {
                if (res.processId == static_cast<int>(ProcessID::SingleProc_Bond))
                {
                    m_curve->startSession();
                }
                else
                {
                    m_curve->finishSession();
                }
            }

            if(res.processId == static_cast<int>(ProcessID::SingleProc_Bond))
            {
                refreshDownWaferDiagram(m_curChip,DownWaferDiagram::ChipState::Bonded);
                updateBondInfo(m_curChip);
            }

            return;
        }

        case AutoProcessCMD_Finished:
        {
            S_AutoProcessCMDFinishedData res =
                    Serializer::from_rawJson<S_AutoProcessCMDFinishedData>(data.json);

            if (m_curve != nullptr)
            {
                m_curve->finishSession();   // 整场结束：收尾（数据定格，不清图）
            }

            const auto type = (res.status == 0)
                                 ? MessageDialog::Success : MessageDialog::Error;
            const std::string msg = (res.status == 0) ? Tr("Success") : Tr("Failed");
            MessageDialog dlg(type, this, QString::fromStdString(msg));
            dlg.exec();

            return;
        }

        default:
            break;
    }

}

void AutoProcessPage::updateBondInfo(QPointF pos)
{
    QDateTime finishTime = QDateTime::currentDateTime();
    const auto it = m_bondInfo.find(m_curChip);
    if (it != m_bondInfo.end())
    {
        it->second.count++;
        it->second.times.push_back(finishTime);
    }
}

void AutoProcessPage::ensureChart()
{
    if (p_Chart != nullptr)
    {
        return;
    }
    p_Chart = new CustomChart(ui->frm_Plot);
    p_Chart->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables | QCP::iSelectAxes);
    QGridLayout *gly = qobject_cast<QGridLayout *>(ui->frm_Plot->layout());
    if (gly == nullptr)
    {
        gly = new QGridLayout(ui->frm_Plot);
    }
    gly->addWidget(p_Chart);
    configureAxes();
    p_Chart->replot();
}

void AutoProcessPage::configureAxes()
{
    // 量程策略与轴定义都在 BondCurve::applyAxisRange() 里（mode=0 固定基线；
    // 数据超过基线时由 BondCurveRecorder 再调一次把量程扩出去）。
    BondCurve::applyAxisRange(p_Chart, BondCurve::kTempAxisBaselineMax, BondCurve::kForceAxisBaselineMax);
}

void AutoProcessPage::clearData()
{
    // 只清曲线（表格是处方数据的展示，不清）
    if (p_Chart != nullptr)
    {
        p_Chart->ClearView();
        p_Chart->replot();
    }
}


void AutoProcessPage::onRecipeUpdated()
{
    MCRecipeManager *mgr = MCRecipeManager::instance();
    ui->le_repcipeName->setText(mgr->reName());
    ui->le_repcipeDescription->setText(mgr->reDescription());
    // 蓝膜现状的查询不在这里发：本页只在"选中处方"时查一次（见 sig_recipeChosen 的 lambda）；
    // 本函数挂在 sigRecipeUpdated 的刷新链上，只刷显示
    initDiagram();
    m_uiTranslator.refresh(this);   // 图例 key 刚随 initDiagram 登记，立刻按当前语言翻一次
}

void AutoProcessPage::initStyle()
{
    QList<GroupBox*> lst_gb = this->findChildren<GroupBox*>();
    for (auto &i : lst_gb)
    {
        i->setStyleSheet(iStarUIStyleSheet->GroupBoxStyle + iStarUIStyleSheet->DoubleSpinBox_Dark);
        i->SetheaderFontSize(12);
    }

    QList<QPushButton*> lst_btn = this->findChildren<QPushButton*>();
    for(auto &b : lst_btn)
    {
        b->setStyleSheet(iStarUIStyleSheet->Button_Border);
        if (m_pRecipeDlg && m_pRecipeDlg->isAncestorOf(b))
        {
            continue;
        }
    }

    QList<QLabel*> lst_label = this->findChildren<QLabel*>();
    for (auto &l : lst_label)
    {
        l->setStyleSheet(iStarUIStyleSheet->fontStyle_normal_12);
    }

    ui->gb_blueWaferStatus->SetHeaderText(QStringLiteral("Blue Film Chip Status Diagram"));
    ui->gb_downWaferStatus->SetHeaderText(QStringLiteral("Down Wafer Status Diagram"));

    ui->label_cur->setStyleSheet(iStarUIStyleSheet->fontStyle_bold_16);
    ui->lbl_curTaskName->setStyleSheet(iStarUIStyleSheet->fontStyle_bold_16);

    ui->tw_status->setStyleSheet(iStarUIStyleSheet->tableStyle);

    ui->le_repcipeName->setStyleSheet(iStarUIStyleSheet->LineEdit + iStarUIStyleSheet->fontStyle_normal_12);
    ui->le_repcipeDescription->setStyleSheet(iStarUIStyleSheet->LineEdit + iStarUIStyleSheet->fontStyle_normal_12);


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

    // 蓝色装饰条（平台 GroupBox 的 SetDecorateColor 在本 DLL 版本不渲染，手动补）
    QList<GroupBox*> lst_gbStrip = this->findChildren<GroupBox*>();
    for (auto *gb : lst_gbStrip)
    {
        if(gb == ui->sideGroup) continue;
        QLabel *strip = new QLabel(gb);
        strip->setFixedSize(5, 24);
        strip->setStyleSheet("background-color:#4C70FA;border:none;");
        strip->move(0, 12);
        strip->show();
    }

    ui->sideGroup->setStyleSheet(iStarUIStyleSheet->GroupBoxStyle);
    ui->sideGroup->SetDecorateColor("transparent");
    ui->sideGroup->SetHeaderText("");

}
