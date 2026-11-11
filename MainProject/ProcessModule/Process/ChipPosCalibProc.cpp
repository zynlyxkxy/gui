#include "ChipPosCalibProc.h"

#include "ui_ProcessPage.h"
#include "Common/CommonFunc.h"          // S_Point2DtoQPoints（原局部 toQPoints 已收归公共）
#include "ProcessModule/Components/DiagramComponent/ChipPosCalibDiagram.h"
#include "istar_serializer.hpp"
#include "dialog/messagedialog.h"
#include "istar_language.h"

#include <QList>
#include <QPointF>
#include <QPushButton>
#include <utility>

using ISTAR::GUI::MessageDialog;

using ISTAR::BasicPlatform::BasicValue::S_Point2D;

ChipPosCalibProc::ChipPosCalibProc(int id, QWidget *tabPage, Ui::ProcessPage *ui, QWidget *dialogParent)
    : ProcessBase(id, tabPage, ui, dialogParent)
    , m_table(ui->tw_waferMarkPosition)
{
    // 分组框标题：英文原稿声明（语言切换由 UiTranslator 处理）
    ui->gb_operateStepDesc_3->SetHeaderText(QStringLiteral("Operation Steps"));
    ui->gb_waferMarkPosition->SetHeaderText(QStringLiteral("Wafer Mark Position Collect (Wafer Stage XY)"));
    ui->gb_siliconStageXyControl->SetHeaderText(QStringLiteral("Wafer Stage XY Control"));
    ui->gb_downWaferMarkPositionDiagram->SetHeaderText(QStringLiteral("Down Wafer Mark Position 2D Diagram"));

    m_table.setReadOnly();
    m_table.setSingleRowSelection();   // 整行单选 + 失焦保持高亮：currentRow 有值即可见
    setActionsEnabled(m_interact);          // 初始下使能
    setConnect();

}

void ChipPosCalibProc::setConnect()
{
    // 拍照
    QObject::connect(ui->btn_takePhoto, &QPushButton::clicked, m_dialogParent, [this]()
    {
        m_interact = false;
        setActionsEnabled(m_interact);
        sendCommand(ChipPosCalibCMD_Capture);
    });

    // 确认：全表提交
    QObject::connect(ui->btn_confirm, &QPushButton::clicked, m_dialogParent, [this]()
    {
        submitAccept();
    });

    // 清空：清选中行数据（本地）
    QObject::connect(ui->btn_clear, &QPushButton::clicked, m_dialogParent, [this]()
    {
        clearSelectedRow();
    });

    // 移动到选中行的名义坐标（绝对）
    QObject::connect(ui->btn_moveToDestination, &QPushButton::clicked, m_dialogParent, [this]()
    {
        const int row = m_table.currentRow();
        if (row < 0 || row >= m_table.rows())
        {
            MessageDialog dlg(MessageDialog::Error, m_dialogParent,
                        QString::fromStdString(Tr("Please select a row first")));
            dlg.exec();
            return;
        }

        double nx = 0.0;
        double ny = 0.0;
        if (m_table.cellPoint(row, ColNominal, nx, ny))
        {
            moveStage(nx, ny, 0);
        }
    });

    // 硅片台相对移动：8 方向按钮共用，方向系数不同
    struct MoveDir { QPushButton *btn; double fx; double fy; };
    const QList<MoveDir> moveDirs = {
        { ui->btn_up,         0.0,  1.0 },
        { ui->btn_down,       0.0, -1.0 },
        { ui->btn_right,      1.0,  0.0 },
        { ui->btn_left,      -1.0,  0.0 },
        { ui->btn_upRight,    1.0,  1.0 },
        { ui->btn_upLeft,    -1.0,  1.0 },
        { ui->btn_downRight,  1.0, -1.0 },
        { ui->btn_downLeft,  -1.0, -1.0 },
    };
    for (const MoveDir &d : moveDirs)
    {
        QObject::connect(d.btn, &QPushButton::clicked, m_dialogParent, [this, d]()
        {
            const double distance = ui->dsb_relativeMove->value();
            moveStage(d.fx * distance, d.fy * distance, 1);
        });
    }

    // 选中行变化不再驱动按钮使能（总开关模式：开=全亮随意点，合法性点击时校验）
}

void ChipPosCalibProc::setStatus(MctrlStatus status)
{
    switch (status)
    {
        case MCTRL_STATUS_BUSY:
            setActionsEnabled(m_interact);
            setTabEnabled(true);
            break;

        case MCTRL_STATUS_IDLE:
            setActionsEnabled(false);
            setTabEnabled(true);
            break;

        default:
            setActionsEnabled(false);
            setTabEnabled(false);
            break;
    }
}

void ChipPosCalibProc::clearData()
{
    m_table.clearColumns({ ColActual, ColPixel });
    m_interact = false;
}

void ChipPosCalibProc::updateRecipe()
{
    // 表格：只吃处方点表，独立快照
    TableData now;
    now.valid = true;
    now.points = toPoints(m_rcpWafer.chipCalibration.lst_chipCalibrationPoints);
    if (!(m_tableData.valid && m_tableData.sameAs(now)))
    {
        m_tableData = now;
        m_table.removeAllRows();
        for (int i = 0; i < int(now.points.size()); ++i)
        {
            m_table.setCell(i, ColNominal, now.points[i].x_, now.points[i].y_);
        }
    }

    // 图：处方 + MC 圆心
    refreshCalibDiagram();

    sendCommand(ChipPosCalibCMD_GetLastResult);
}

void ChipPosCalibProc::updateMc()
{
    // 机器常数更新（圆心）也刷图；消费数据没变时 refresh 内部快照会跳过
    refreshCalibDiagram();
}

void ChipPosCalibProc::refreshCalibDiagram()
{
    // 图消费：处方（芯片布局/标定点/尺寸）+ 机器常量（晶圆圆心）
    const auto &info = m_rcpWafer.material.oneChipTypeInfo;
    const auto &centerXYRz = m_mc.process.commonProc.downChuckCenterWaferTableXYRz;
    const S_Point2D &center = {centerXYRz.x_, centerXYRz.y_};
    DiagramData now;
    now.valid = true;
    now.waferSize = info.waferSize;
    now.chipW = info.chipRecipe.chipWidth;
    now.chipH = info.chipRecipe.chipHeight;
    now.centerX = center.x_;
    now.centerY = center.y_;
    now.chips = info.lst_nominalPoints;
    now.marks = m_rcpWafer.chipCalibration.lst_allChipMarkCalibrationPoints;        // 图上全部标记点
    now.redMarks = toPoints(m_rcpWafer.chipCalibration.lst_chipCalibrationPoints);  // 需标红的子集
    if (m_diagramData.valid && m_diagramData.sameAs(now))
    {
        return;
    }
    m_diagramData = now;

    // 标定图：名义阶段（芯片黄、全部标记点黑、待标定子集标红，名义值由图上加中心换算成
    // 理论坐标后画点/标注；直径 inch）
    ui->gv_downWaferMarkPosition->setNominal(S_Point2DtoQPoints(now.chips), S_Point2DtoQPoints(now.marks),
                                             now.centerX, now.centerY,
                                             now.chipW, now.chipH, now.waferSize);
    ui->gv_downWaferMarkPosition->setRedPoints(S_Point2DtoQPoints(now.redMarks));
}

void ChipPosCalibProc::ReceiveMctrlCommand(const ProcessMessage &data)
{
    switch (data.command)
    {
        case ChipPosCalibCMD_EnterInteract:
            m_interact = true;
            setActionsEnabled(m_interact);
            break;

        case ChipPosCalibCMD_MarkResult:
        {
            S_ChipPosCalibCMD_MarkResult res =
                    Serializer::from_rawJson<S_ChipPosCalibCMD_MarkResult>(data.json);
            if (!res.ok)
            {
                MessageDialog dlg(MessageDialog::Error, m_dialogParent,
                                  QString::fromStdString(Tr("Mark not recognized")));
                dlg.exec();
                break;
            }
            const int row = m_table.currentRow();
            if (row >= 0 && row < m_table.rows())
            {
                m_table.setCell(row, ColActual, res.markX, res.markY);
                m_table.setCell(row, ColPixel, res.posX, res.posY);
            }
        }
        break;

        case ChipPosCalibCMD_CmdRejected:
        {
            S_ChipPosCalibCMD_CmdRejected res =
                    Serializer::from_rawJson<S_ChipPosCalibCMD_CmdRejected>(data.json);
            MessageDialog dlg(MessageDialog::Error, m_dialogParent,
                              QString::fromStdString(res.reason));
            dlg.exec();
        }
        break;

        case ChipPosCalibCMD_AllMarkTurePos:
        {
            // 全部名义点的真实坐标（与图上点集同序同长）：绑定到图（点位不动、不清图）
            S_ChipPosCalibCMD_AllMarkTurePos res =
                    Serializer::from_rawJson<S_ChipPosCalibCMD_AllMarkTurePos>(data.json);
            std::vector<QPointF> trues;
            trues.reserve(res.markPoints.size());
            for (const S_ChipPosCalibMarkTurePos &p : res.markPoints)
            {
                trues.emplace_back(p.truePosX, p.truePosY);
            }
            ui->gv_downWaferMarkPosition->setRealPoints(trues);   // 绑定成功后点可点击，显示 真实+名义
        }
        break;

        default:
            break;
    }
}

void ChipPosCalibProc::sendCommand(int command, const std::string &json)
{
    if (m_mctrlReporter)
    {
        m_mctrlReporter(command, json);
    }
}

void ChipPosCalibProc::moveStage(double x, double y, int moveMode)
{
    S_ChipPosCalibCMD_Move payload;
    payload.moveMode = moveMode;
    payload.x = x;
    payload.y = y;

    m_interact = false;
    setActionsEnabled(m_interact);
    sendCommand(ChipPosCalibCMD_Move, Serializer::to_rawJson(payload));
}

void ChipPosCalibProc::submitAccept()
{
    // 读全表：名义 + 实测都有值的行进入提交
    S_ChipPosCalibCMD_Accept payload;
    const int total = m_table.rows();
    for (int r = 0; r < total; ++r)
    {
        double nx = 0.0;
        double ny = 0.0;
        double ax = 0.0;
        double ay = 0.0;
        if (m_table.cellPoint(r, ColNominal, nx, ny) && m_table.cellPoint(r, ColActual, ax, ay))
        {
            S_ChipPosCalibMarkTurePos p;
            p.nominalPosX = nx;
            p.nominalPosY = ny;
            p.truePosX = ax;
            p.truePosY = ay;
            payload.markPoints.push_back(p);
        }
    }

    const int measured = int(payload.markPoints.size());
    if (measured < 2)
    {
        MessageDialog dlg(MessageDialog::Error, m_dialogParent,
                          QString::fromStdString(Tr("At least 2 measured points are required")));
        dlg.exec();
        return;
    }
    if (measured < total)
    {
        MessageDialog confirm(MessageDialog::Attention, m_dialogParent,
                              QString::fromStdString(Tr("Unmeasured points exist / submit anyway?")));
        if (confirm.exec() != QDialog::Accepted)
        {
            return;
        }
    }

    m_interact = false;
    setActionsEnabled(m_interact);
    sendCommand(ChipPosCalibCMD_Accept, Serializer::to_rawJson(payload));
}

void ChipPosCalibProc::clearSelectedRow()
{
    const int row = m_table.currentRow();
    if (row >= 0 && row < m_table.rows())
    {
        m_table.clearRowExcept(row, ColNominal);   // 名义坐标保留
    }
}

void ChipPosCalibProc::setActionsEnabled(bool enabled)
{
    // 全开/全关：交互期（上使能）用户随意点，合法性由点击时校验；其余状态（下使能）全灰不可点
    const QList<QPushButton *> actionButtons = {
        ui->btn_takePhoto, ui->btn_moveToDestination, ui->btn_clear, ui->btn_confirm,
        ui->btn_upLeft, ui->btn_up, ui->btn_upRight, ui->btn_left,
        ui->btn_right, ui->btn_downLeft, ui->btn_down, ui->btn_downRight
    };
    for (QPushButton *btn : actionButtons)
    {
        btn->setEnabled(enabled);
    }
}
