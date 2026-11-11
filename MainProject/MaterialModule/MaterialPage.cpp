#include "MaterialPage.h"
#include "ui_MaterialPage.h"

#include "MCRecipeModule/MCRecipeManager.h"
#include "istar_ui_stylesheet.h"
#include "istar_enumconverter.hpp"
#include "dialog/messagedialog.h"
#include "groupbox/groupbox.h"
#include "istar_serializer.hpp"
#include "istar_language.h"
#include "ProcessModule/Protocol/ProcessManagerProtocol.h"
#include "ProcessModule/Protocol/BlueFilmChipHandoverProtocol.h"

#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDebug>

using namespace ISTAR::UIStyle;
using namespace ISTAR::GUI;

MaterialPage::MaterialPage(QWidget *parent) :
    QWidget(parent)
   , ui(new Ui::MaterialPage)
   , m_pRecipeDlg(new RecipeChooseWidget(this))
{
    ui->setupUi(this);
    initUi();
    initStyle();
    createTabs();
    initConnect();
    // 翻译由外部（MainWindow）在语言初始化 / 切换时触发，不自行调用
}

MaterialPage::~MaterialPage()
{
    delete ui;
}

void MaterialPage::refreshTexts()
{
    m_uiTranslator.refresh(this);
}

// ---------------------------------------------------------------------------
// tab 注册（新增 tab 改这里）
// ---------------------------------------------------------------------------

template<typename TabT>
void MaterialPage::registerTab(ProcessID id, ProcessID id_)
{
    const int idx = int(id);
    const int idx_ = int(id_);
    QWidget *tabPage = ui->tabWidget->widget(idx);   // 下标即 tab；越界返回 nullptr

    // 只 new 一次：上下片一对共用一个 tab 对象。
    // 若各 new 一个，两个实例共享同一批 ui 控件（ui 指针相同），同一个按钮会被 connect 两次
    //（lambda 连接 Qt 不去重，Qt::UniqueConnection 对 functor 无效）→ 一次点击发两条 Start，
    // 第一条启动任务、第二条被 manager 拒绝报 "another task running"。
    auto tab = std::make_shared<TabT>(idx, tabPage, ui, this);

    // 注入 GUI → mctrl 上报回调：process 固定为主流程 id，command / json 由流程给
    //（共用对象只有一份回调；别名流程若需上报，需给基类加显式 process 参数）
    tab->SetMctrlReporter([idx](int command, const std::string &json)
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

    m_tabs[idx] = tab;    // 主流程 id
    m_tabs[idx_] = tab;   // 别名流程 id → 同一对象（消息按流程 id 直取路由，onMctrlMessage 无需改）
}

void MaterialPage::createTabs()
{
    // 上下片一对共用一个 tab 对象；蓝膜上下片同理
    registerTab<DownWaferLoadUnload>(ProcessID::SingleProc_WaferLoad,
                                     ProcessID::SingleProc_WaferUnload);
    registerTab<BlueFilmLoadUnload>(ProcessID::SingleProc_BlueFilmLoad,
                                    ProcessID::SingleProc_BlueFilmUnload);
}

// ---------------------------------------------------------------------------
// 状态（订阅 StatusManager，广播给全部 tab）
// ---------------------------------------------------------------------------

void MaterialPage::onStatusChanged(MctrlModule::MctrlStatus prev, MctrlModule::MctrlStatus curr)
{
    Q_UNUSED(prev);

    for (const auto &kv : m_tabs)
    {
        if (kv.second != nullptr)
        {
            kv.second->setStatus(curr);
        }
    }
}

void MaterialPage::initConnect()
{
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

            // 本页选中的处方：只有本页去查蓝膜现状（以前本页压根不发）；
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

void MaterialPage::onRecipeUpdated()
{
    MCRecipeManager *mgr = MCRecipeManager::instance();
    ui->le_repcipeName->setText(mgr->reName());
    ui->le_repcipeDescription->setText(mgr->reDescription());

    // 本页显示刷新（sigRecipeUpdated 触发），tab 自己决定消费处方里的哪些数据
    for (const auto &kv : m_tabs)
    {
        if (kv.second != nullptr)
        {
            kv.second->updateRecipe();
        }
    }
}

void MaterialPage::requestBlueFilmChips()
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

void MaterialPage::onMcUpdated()
{
    // TODO 机器常量变更：本页暂无消费项，需要时在这里刷新（参考 ProcessPage::onMcUpdated）
}

void MaterialPage::onMctrlMessage(const MctrlModule::ProcessMessage &data)
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
            ui->gv_blueFilemLoadUnload->setPoints({});
            return;
        }

        ui->gv_blueFilemLoadUnload->setPoints(res.chipPos);
        return;
    }

    const auto it = m_tabs.find(data.process);
    if (it == m_tabs.end())
    {
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
                MessageDialog dlg(MessageDialog::Error, this,
                                QString::fromStdString(res.errorInfo));
                dlg.exec();
            }
        }
        break;

        case ProcessManagerCMD_SingleProcFinished:
        {
            S_SingleProcFinishedData res =
                    Serializer::from_rawJson<S_SingleProcFinishedData>(data.json);

            const auto type = (res.status == 0)
                                 ? MessageDialog::Success : MessageDialog::Error;
            const std::string msg = (res.status == 0) ? Tr("Success") : Tr(res.reason);
            MessageDialog dlg(type, this, QString::fromStdString(msg));
            dlg.exec();
        }
        break;

        default:
            break;
    }

    it->second->ReceiveMctrlCommand(data);

}

void MaterialPage::initUi()
{
    // 页面级只读（tab 内部的只读在各 tab 类里做）
    ui->le_repcipeName->setReadOnly(true);
    ui->le_repcipeDescription->setReadOnly(true);
}

void MaterialPage::initStyle()
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
        if(b == ui->btn_recipe) continue;
        b->setFixedSize(150,50);
    }

    QList<QLabel*> lst_label = this->findChildren<QLabel*>();
    for (auto &l : lst_label)
    {
        l->setStyleSheet(iStarUIStyleSheet->fontStyle_normal_12);
    }

    ui->tabWidget->setStyleSheet(iStarUIStyleSheet->TabWidget + iStarUIStyleSheet->fontStyle_normal_12
                                 + QStringLiteral("QTabBar::tab{width:130;}")
                                 + QStringLiteral("QTabBar::tab{font-size:10pt;}"));

    // 蓝色装饰条（平台 GroupBox 的 SetDecorateColor 在本 DLL 版本不渲染，手动补）
    QList<GroupBox*> lst_gbStrip = this->findChildren<GroupBox*>();
    for (auto *gb : lst_gbStrip)
    {
        QLabel *strip = new QLabel(gb);
        strip->setFixedSize(5, 24);
        strip->setStyleSheet("background-color:#4C70FA;border:none;");
        strip->move(0, 12);
        strip->show();
    }

}
