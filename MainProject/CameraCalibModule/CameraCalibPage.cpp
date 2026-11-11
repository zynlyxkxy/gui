#include "CameraCalibPage.h"
#include "ui_CameraCalibPage.h"

#include "ProcessModule/Protocol/ProcessManagerProtocol.h"   // ProcessID / ProcessManagerCMD

#include "button/effectbutton.h"   // ISTAR::GUI::EffectButton（开始/停止按钮）
#include "dialog/messagedialog.h"
#include "groupbox/groupbox.h"
#include "istar_language.h"
#include "istar_serializer.hpp"
#include "istar_ui_stylesheet.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#define TR_STR(msg)  QString::fromStdString(Tr(msg))

using namespace ISTAR;
using namespace ISTAR::GUI;
using namespace ISTAR::UIStyle;

namespace {

// 宽容解析：载荷缺失/语法损坏时不抛异常炸掉界面，按"解析失败"返回 false。
// 流程的部分响应（如 EnterInteract）不带载荷，空 json 属正常情况。
template <typename T>
bool tryParseJson(const std::string &json, T &out)
{
    if (json.empty())
    {
        return false;
    }
    try
    {
        out = Serializer::from_rawJson<T>(json);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

// 流程回的失败文案（约定是 ASCII 英文）：有就用，没有给通用兜底，避免弹空框
QString errText(const std::string &errorInfo)
{
    return errorInfo.empty() ? TR_STR("Camera Calibration Failed")
                             : QString::fromStdString(errorInfo);
}

// 轴坐标读数："(x, y)"（与设计图上模板中心 / 旋转中心的显示形式一致）
QString xyText(double x, double y)
{
    return QStringLiteral("(%1, %2)")
            .arg(QString::number(x, 'f', 3), QString::number(y, 'f', 3));
}

}  // namespace

CameraCalibPage::CameraCalibPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CameraCalibPage)
{
    ui->setupUi(this);
    initUi();
    initStyle();
    initConnect();
    // 按钮初值按当前 mctrl 状态（未连上 → 全灰）：开始/结束不靠本地标志，
    // 所以 GUI 重启后只要状态推回来，按钮就跟控制层真实状态一致
    setStatus(StatusManager::instance()->status());
    // 翻译由外部（MainWindow）在语言初始化 / 切换时触发，不自行调用
}

CameraCalibPage::~CameraCalibPage()
{
    delete ui;
}

void CameraCalibPage::refreshTexts()
{
    // 标定项下拉的条目是英文标识符（要原样下发给流程），结果框里是数字，
    // 都不在翻译范围内；按钮/标签/分组框标题交给 UiTranslator
    m_uiTranslator.refresh(this);
}

// ---------------------------------------------------------------------------
// 初始化
// ---------------------------------------------------------------------------

void CameraCalibPage::initUi()
{
    // 初始化模块：标定项是下拉（4 个可选值，条目名就是下发给流程的标定项名，
    // 流程按它推导相机上下方位、相机 id 与要移动的台子）；模板名手填，分辨率是数字框
    ui->cbo_calibItem->addItem(QStringLiteral("TopCameraAlignStage"));
    ui->cbo_calibItem->addItem(QStringLiteral("BottomCameraAlignStage"));
    ui->cbo_calibItem->addItem(QStringLiteral("BottomCameraWaferStage"));
    ui->cbo_calibItem->addItem(QStringLiteral("BottomCameraXYRz"));
    ui->cbo_calibItem->setCurrentIndex(0);

    // 数字框的初值（.ui 里只给了范围/小数位，没给初值）：分辨率 3000×3000、位移量默认 0.3mm、
    // 转动角度默认 2°（是 0 的话标定没有意义）。单位写在各自标签里，框里只放数
    ui->dsb_resolutionX->setValue(3000.0);
    ui->dsb_resolutionY->setValue(3000.0);
    ui->dsb_degMove->setValue(0.3);
    ui->dsb_scaleXMove->setValue(0.3);
    ui->dsb_scaleYMove->setValue(0.3);
    ui->dsb_rotateDeg->setValue(2.0);

    // 结果框（.ui 里 objectName 统一以 le_out 开头）：只读 + 居中，另存一份供清值用
    for (QLineEdit *edit : this->findChildren<QLineEdit *>())
    {
        if (!edit->objectName().startsWith(QStringLiteral("le_out")))
        {
            continue;
        }
        edit->setReadOnly(true);
        edit->setAlignment(Qt::AlignCenter);
        m_outputEdits.append(edit);
    }
}

void CameraCalibPage::initStyle()
{
    // 5 个分组框（初始化 / 三条传感器链 / 旋转中心）都是平级的，样式统一：
    // 平台分组框样式 + 深底数字框 + 标题左侧蓝条（见函数末尾）
    const QList<GroupBox *> boxes = {ui->gb_init, ui->gb_chainDeg, ui->gb_chainScaleX,
                                     ui->gb_chainScaleY, ui->gb_rot};

    // 左侧栏（sideGroup）：与 SystemStatusPage 一致 —— 平台分组框样式 + 隐藏装饰条/标题
    ui->sideGroup->setStyleSheet(iStarUIStyleSheet->GroupBoxStyle);
    ui->sideGroup->SetDecorateColor("transparent");
    ui->sideGroup->SetHeaderText("");

    for (auto *gb : boxes)
    {
        gb->setStyleSheet(iStarUIStyleSheet->GroupBoxStyle + iStarUIStyleSheet->DoubleSpinBox_Dark);
        gb->SetheaderFontSize(12);
    }

    // 分组框标题：英文原稿挂在这（语言切换时由 UiTranslator 捕获 lbl_Title 后翻译）
    ui->gb_init->SetHeaderText(QStringLiteral("Initialization"));
    ui->gb_chainDeg->SetHeaderText(QStringLiteral("Camera Angle"));
    ui->gb_chainScaleX->SetHeaderText(QStringLiteral("X Pixel Scale"));
    ui->gb_chainScaleY->SetHeaderText(QStringLiteral("Y Pixel Scale"));
    ui->gb_rot->SetHeaderText(QStringLiteral("Rotation Center Calibration"));

    // ---- 按钮：只上边框样式；尺寸/策略由 .ui 定（各按钮 .ui 里已统一 200x40）----
    // 开始/停止是平台的 EffectButton（QPushButton 派生）：给它们单独上会话开关样式（见下），
    // 这里跳过，免得被通用按钮样式覆盖
    for (QPushButton *b : this->findChildren<QPushButton *>())
    {
        if (qobject_cast<EffectButton *>(b) != nullptr)
        {
            continue;
        }
        b->setStyleSheet(iStarUIStyleSheet->Button_Border);
    }

    // 会话开关（开始/停止标定）：沿用 SystemStatusPage 那套按钮样式 + 粗体 12 号字
    const QString sessionBtnStyle = QString(
        "QPushButton{background-color:#FFFFFF;color:#808080;border-radius:5px;}"
        "QPushButton:hover{border:2px solid #A3B6FF;color:#A3B6FF;}"
        "QPushButton:pressed{border:2px solid #4C70FA;color:#4C70FA;}"
        "QPushButton:checked{border:2px solid #4C70FA;color:#4C70FA;}"
        "QPushButton:disabled{color:#E0E0E0;}")
        + iStarUIStyleSheet->fontStyle_bold_12;
    ui->btn_startCalib->setStyleSheet(sessionBtnStyle);
    ui->btn_stopCalib->setStyleSheet(sessionBtnStyle);

    for (auto *dsb : this->findChildren<QDoubleSpinBox *>())
    {
        dsb->setStyleSheet(iStarUIStyleSheet->DoubleSpinBox_Dark);
    }

    for (auto *le : this->findChildren<QLineEdit *>())
    {
        // 只读结果框：平台的 LineEdit 样式是白底、只有 hover/focus 才有边框，
        // 在浅色页面上跟透明一样看不出要显示在哪 —— 补一圈常显边框
        // 可输入项用深底（LineEdit_Dark）：一眼看出能不能改
        le->setStyleSheet((le->isReadOnly() ? iStarUIStyleSheet->LineEdit + QStringLiteral("QLineEdit{border:1px solid #B4B4C8;}")
                                            : iStarUIStyleSheet->LineEdit_Dark)
                          + iStarUIStyleSheet->fontStyle_normal_12);
    }

    for (auto *cbo : this->findChildren<QComboBox *>())
    {
        cbo->setStyleSheet(iStarUIStyleSheet->ComboBox + iStarUIStyleSheet->fontStyle_normal_12);
    }

    for (auto *l : this->findChildren<QLabel *>())
    {
        l->setStyleSheet(iStarUIStyleSheet->fontStyle_normal_12);
    }

    // 蓝色装饰条（平台 GroupBox 的 SetDecorateColor 在本 DLL 版本不渲染，手动补）：
    // 5 个平级框都补，贴各自标题左侧
    for (auto *gb : boxes)
    {
        QLabel *strip = new QLabel(gb);
        strip->setFixedSize(5, 24);
        strip->setStyleSheet("background-color:#4C70FA;border:none;");
        strip->move(0, 12);
        strip->show();
    }
}

void CameraCalibPage::initConnect()
{
    // ---- 会话开关：开始 / 结束标定 ----
    connect(ui->btn_startCalib, &QPushButton::clicked, this, [this]()
    {
        onStartCalib();
    });
    connect(ui->btn_stopCalib, &QPushButton::clicked, this, [this]()
    {
        onStopCalib();
    });

    // ---- 初始化：就一个按钮，流程按标定项自己分流（之后模板不再重发）----
    // （BottomCameraXYRz = 旋转中心项 → 轴标定项资源初始化；
    //   其余三个 = 传感器项 → sensor 资源 + 分辨率 + 模板）
    connect(ui->btn_init, &QPushButton::clicked, this, [this]()
    {
        sendInit();
    });

    // ---- 图1：三条完全独立的动作链，各自读自己的位移量，互不引用 ----
    connect(ui->btn_calDeg, &QPushButton::clicked, this, [this]()
    {
        sendSensorCalib(CameraCalibCMD_CalSensorDeg, ui->dsb_degMove->value());
    });
    connect(ui->btn_calScaleX, &QPushButton::clicked, this, [this]()
    {
        sendSensorCalib(CameraCalibCMD_CalSensorScaleX, ui->dsb_scaleXMove->value());
    });
    connect(ui->btn_calScaleY, &QPushButton::clicked, this, [this]()
    {
        sendSensorCalib(CameraCalibCMD_CalSensorScaleY, ui->dsb_scaleYMove->value());
    });

    // ---- 图2：转角度 → 取 5 个模板中心 → 算旋转中心 → 保存 ----
    connect(ui->btn_rotate, &QPushButton::clicked, this, [this]()
    {
        sendRotateAxis(ui->dsb_rotateDeg->value());
    });

    const QList<QPushButton *> markBtns = {ui->btn_mark1, ui->btn_mark2, ui->btn_mark3,
                                           ui->btn_mark4, ui->btn_mark5};
    for (int i = 0; i < markBtns.size(); ++i)
    {
        connect(markBtns[i], &QPushButton::clicked, this, [this, i]()
        {
            sendMarkCenter(i + 1);   // 序号从 1 起
        });
    }

    connect(ui->btn_calRotCenter, &QPushButton::clicked, this, [this]()
    {
        sendCalRotCenter();
    });

    // ---- 保存：就一个按钮（iStarSaveCalibData 落的是整份标定数据，与具体标定项无关）----
    connect(ui->btn_save, &QPushButton::clicked, this, [this]()
    {
        sendSave();
    });

    // 流程状态（与 ProcessPage 的 ProcessBase 同源）：BUSY 才可能亮动作区
    connect(StatusManager::instance(), &StatusManager::sigStatusChanged, this,
            [this](MctrlModule::MctrlStatus, MctrlModule::MctrlStatus curr)
    {
        setStatus(curr);
    });

    connect(MCTRLClientManager::getInstance(), &MCTRLClientManager::sig_ProcessObserverData, this,
            [this](const MctrlModule::ProcessMessage &data)
    {
        onMctrlMessage(data);
    });
}

// ---------------------------------------------------------------------------
// 会话开关（开始 / 结束标定：能不能点只由 mctrl 状态决定，见 setStatus）
// ---------------------------------------------------------------------------

void CameraCalibPage::onStartCalib()
{
    clearOutputs();   // 新会话：上一轮的结果清掉
    // 先按"两个都不许点"发出去，真实状态等 mctrl 状态推回来（同 ProcessPage，不写本地标志）
    ui->btn_startCalib->setEnabled(false);
    ui->btn_stopCalib->setEnabled(false);
    sendCommand(ProcessManagerCMD_Start, "");
}

void CameraCalibPage::onStopCalib()
{
    ui->btn_startCalib->setEnabled(false);
    ui->btn_stopCalib->setEnabled(false);
    sendCommand(ProcessManagerCMD_Stop, "");
}

// ---------------------------------------------------------------------------
// GUI → 流程
// ---------------------------------------------------------------------------

void CameraCalibPage::sendCommand(int command, const std::string &json)
{
    // 发出命令即退出交互态：动作区全灰，等流程再推 EnterInteract 才解锁
    // （与 ChipPosCalibProc 的 m_interact 总开关同口径）
    m_interact = false;
    setActionsEnabled(false);

    MctrlModule::ProcessMessage msg;
    msg.process = static_cast<int>(ProcessID::SingleProc_CameraCalib);
    msg.command = command;
    msg.json = json;
    // 异步发送：不阻塞 GUI 线程；应答走 sig_ProcessObserverData 广播（onMctrlMessage 已接）
    MCTRLClientManager::getInstance()->getClient()->GuiToMctrlCommandAsync(
                msg, [](ResponseNoDataMessageStruct &)
    {
        return ISTAR_RESULT_SUCCESS;
    });
}

S_CameraCalibBaseInfo CameraCalibPage::baseInfo() const
{
    // 相机信息就来自初始化模块那三个控件：标定项下拉（条目名即协议值）、模板名、分辨率。
    // 相机上下方位与相机 id 不由 GUI 指定，流程按标定项自己推导
    S_CameraCalibBaseInfo info;
    info.calibItemName = ui->cbo_calibItem->currentText().toStdString();
    info.cameraTemplateName = ui->le_template->text().toStdString();
    info.resolutionX = static_cast<int>(ui->dsb_resolutionX->value());   // 图像宽（像素）
    info.resolutionY = static_cast<int>(ui->dsb_resolutionY->value());   // 图像高（像素）
    return info;
}

void CameraCalibPage::sendInit()
{
    // 初始化：把相机信息发给流程（保存参数 + 建标定项资源 + 下发模板；传感器项还要设分辨率）
    // 流程按标定项自己分流，结果回 CameraCalibCMD_InitResult（无业务数据）
    S_CameraCalibInitReq req;
    req.baseInfo = baseInfo();
    sendCommand(CameraCalibCMD_Init, Serializer::to_rawJson(req));
}

void CameraCalibPage::sendSensorCalib(int command, double moveMm)
{
    // 相机信息在 Init 时已下发，这里只有移动量
    S_CameraCalibSensorReq req;
    req.moveMm = moveMm;
    sendCommand(command, Serializer::to_rawJson(req));
}

void CameraCalibPage::sendRotateAxis(double deg)
{
    S_CameraCalibRotateReq req;
    req.deg = deg;
    sendCommand(CameraCalibCMD_RotateAxis, Serializer::to_rawJson(req));
}

void CameraCalibPage::sendMarkCenter(int index)
{
    // 模板在 Init 时已下发，这里只有序号
    S_CameraCalibMarkReq req;
    req.index = index;
    sendCommand(CameraCalibCMD_MarkTemplateCenter, Serializer::to_rawJson(req));
}

void CameraCalibPage::sendCalRotCenter()
{
    // 无载荷：流程用前面已取到的模板中心点算
    sendCommand(CameraCalibCMD_CalRotCenter, "");
}

void CameraCalibPage::sendSave()
{
    // 无载荷：流程调 iStarSaveCalibData 全量落盘
    sendCommand(CameraCalibCMD_Save, "");
}

// ---------------------------------------------------------------------------
// 流程 → GUI
// ---------------------------------------------------------------------------

void CameraCalibPage::onMctrlMessage(const MctrlModule::ProcessMessage &data)
{
    if (data.process != static_cast<int>(ProcessID::SingleProc_CameraCalib))
    {
        return;
    }

    switch (data.command)
    {
        case ProcessManagerCMD_Response:   // 任务层答复：Start / Stop 是否成功
        {
            S_ProcessManagerResponseData res;
            if (tryParseJson(data.json, res) && !res.success)
            {
                // 启停被拒（如"another task running"）：按当前 mctrl 状态重刷一遍按钮。
                // 状态没变就不会再有 sigStatusChanged，这里不自己刷的话，刚点灰的按钮会一直灰
                setStatus(StatusManager::instance()->status());
                showError(errText(res.errorInfo));
            }
        }
        break;

        case CameraCalibCMD_EnterInteract:      // 流程就绪 → 上使能动作区
        {
            m_interact = true;
            setActionsEnabled(m_interact);
        }
        break;

        case CameraCalibCMD_InitResult:         // 初始化结果（无业务数据，和保存一样）
        {
            S_CameraCalibCmdAck res;
            if (!tryParseJson(data.json, res))
            {
                return;
            }
            if (!res.ok)
            {
                // 初始化失败：闸门没开，后续命令都会被流程拒
                showError(errText(res.errorInfo));
                return;
            }
            clearOutputs();   // 换标定项重新初始化：上一项的结果已无意义
            showInfo(TR_STR("Initialization succeeded"));
        }
        break;

        case CameraCalibCMD_SensorDegResult:
        {
            S_CameraCalibDegResult res;
            if (!tryParseJson(data.json, res))
            {
                return;
            }
            if (!res.ok)
            {
                showError(errText(res.errorInfo));
                return;
            }
            setDegResult(res.deg);
            showInfo(TR_STR("Camera angle calculated"));
        }
        break;

        case CameraCalibCMD_SensorScaleResult:
        {
            S_CameraCalibScaleResult res;
            if (!tryParseJson(data.json, res))
            {
                return;
            }
            if (!res.ok)
            {
                showError(errText(res.errorInfo));
                return;
            }
            setScaleResult(res.horizon, res.scale);
            showInfo(TR_STR("Pixel scale calculated"));
        }
        break;

        case CameraCalibCMD_RotateAxisResult:
        {
            // 转动只回执行确认，没有业务数值
            S_CameraCalibCmdAck res;
            if (!tryParseJson(data.json, res))
            {
                return;
            }
            if (!res.ok)
            {
                showError(errText(res.errorInfo));
                return;
            }
            showInfo(TR_STR("Rotation done"));
        }
        break;

        case CameraCalibCMD_MarkCenterResult:
        {
            S_CameraCalibMarkResult res;
            if (!tryParseJson(data.json, res))
            {
                return;
            }
            if (!res.ok)
            {
                showError(errText(res.errorInfo));
                return;
            }
            setMarkResult(res.index, res.axisX, res.axisY);
            showInfo(TR_STR("Center taken"));
        }
        break;

        case CameraCalibCMD_RotCenterResult:
        {
            S_CameraCalibRotCenterResult res;
            if (!tryParseJson(data.json, res))
            {
                return;
            }
            if (!res.ok)
            {
                showError(errText(res.errorInfo));
                return;
            }
            setRotCenterResult(res.centerX, res.centerY);
            showInfo(TR_STR("Rotation center calculated"));
        }
        break;

        case CameraCalibCMD_SaveResult:
        {
            S_CameraCalibCmdAck res;
            if (!tryParseJson(data.json, res))
            {
                return;
            }
            if (!res.ok)
            {
                showError(errText(res.errorInfo));
                return;
            }
            showInfo(TR_STR("Calibration data saved"));
        }
        break;

        case CameraCalibCMD_ErrorMessage:   // 流程不报警：失败/被拒统一走这条
        {
            S_CameraCalibCmdAck res;
            if (!tryParseJson(data.json, res))
            {
                // 连载荷都坏了：至少让用户知道出错了，别无声无息
                showError(TR_STR("Camera Calibration Failed"));
                return;
            }
            showError(errText(res.errorInfo));
        }
        break;

        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// 使能（同 ProcessPage / ChipPosCalibProc 的模式）
// ---------------------------------------------------------------------------

void CameraCalibPage::setStatus(MctrlStatus status)
{
    // 开始/结束标定只认 mctrl 状态（同 ProcessPage::refreshRunControls）：
    // 本地不留"我发过 Start"的副本，GUI 重启后状态一推回来按钮就回到真实状态
    switch (status)
    {
        case MCTRL_STATUS_BUSY:   // 有任务在跑：只能结束；动作区再按流程是否在交互态决定
            ui->btn_startCalib->setEnabled(false);
            ui->btn_stopCalib->setEnabled(true);
            setActionsEnabled(m_interact);
            break;

        case MCTRL_STATUS_IDLE:   // 空闲：可以开始；动作区要流程交互态，一律灰
            ui->btn_startCalib->setEnabled(true);
            ui->btn_stopCalib->setEnabled(false);
            setActionsEnabled(false);
            break;

        default:                  // 断连 / 装载 / 初始化中 / 报警 / 未知：全灰
            ui->btn_startCalib->setEnabled(false);
            ui->btn_stopCalib->setEnabled(false);
            setActionsEnabled(false);
            break;
    }
}

void CameraCalibPage::setActionsEnabled(bool enabled)
{
    // 全开/全关：交互期用户随意点，合法性由点击时校验；其余状态全灰不可点
    // （init 也是普通动作按钮：交互期才亮，没初始化时点它正是它的用途）
    // 只管动作区，不碰开始/结束 —— 那两个由 setStatus 按 mctrl 状态定
    const QList<QPushButton *> actionButtons = {
        ui->btn_init,
        ui->btn_calDeg, ui->btn_calScaleX, ui->btn_calScaleY,
        ui->btn_rotate,
        ui->btn_mark1, ui->btn_mark2, ui->btn_mark3, ui->btn_mark4, ui->btn_mark5,
        ui->btn_calRotCenter,
        ui->btn_save,
    };
    for (QPushButton *btn : actionButtons)
    {
        btn->setEnabled(enabled);
    }
}

// ---------------------------------------------------------------------------
// 显示
// ---------------------------------------------------------------------------

void CameraCalibPage::clearOutputs()
{
    for (QLineEdit *edit : m_outputEdits)
    {
        if (edit != nullptr)
        {
            edit->clear();
        }
    }
}

void CameraCalibPage::setDegResult(double deg)
{
    ui->le_outDeg->setText(QString::number(deg, 'f', 3));
}

void CameraCalibPage::setScaleResult(bool horizon, double scale)
{
    // horizon 区分 x/y（与 iStarCalSensorScale 的入参同义）：两条链共用一个响应命令
    QLineEdit *edit = horizon ? ui->le_outScaleX : ui->le_outScaleY;
    edit->setText(QString::number(scale, 'f', 6));
}

void CameraCalibPage::setMarkResult(int index, double axisX, double axisY)
{
    QLineEdit *edit = nullptr;
    switch (index)
    {
        case 1: edit = ui->le_outMark1; break;
        case 2: edit = ui->le_outMark2; break;
        case 3: edit = ui->le_outMark3; break;
        case 4: edit = ui->le_outMark4; break;
        case 5: edit = ui->le_outMark5; break;
        default: break;
    }
    if (edit == nullptr)
    {
        return;   // 越界序号（协议只有 1~5）：丢弃，绝不写空指针
    }
    edit->setText(xyText(axisX, axisY));
}

void CameraCalibPage::setRotCenterResult(double centerX, double centerY)
{
    ui->le_outRotCenter->setText(xyText(centerX, centerY));
}

void CameraCalibPage::showError(const QString &msg)
{
    if (msg.isEmpty())
    {
        return;
    }
    MessageDialog dlg(MessageDialog::Error, this, msg);
    dlg.exec();
}

void CameraCalibPage::showInfo(const QString &msg)
{
    // 每条请求的结果都要让操作员看得见：成功也弹一下，不然点了按钮不知道做完没有
    if (msg.isEmpty())
    {
        return;
    }
    MessageDialog dlg(MessageDialog::Success, this, msg);
    dlg.exec();
}

// ---------------------------------------------------------------------------
// 标定项
// ---------------------------------------------------------------------------

