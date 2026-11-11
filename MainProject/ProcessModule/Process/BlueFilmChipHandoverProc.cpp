#include "BlueFilmChipHandoverProc.h"

#include "ui_ProcessPage.h"

#include <QColor>
using namespace ISTAR::GUI;

BlueFilmChipHandoverProc::BlueFilmChipHandoverProc(int id, QWidget *tabPage, Ui::ProcessPage *ui,
                                                   QWidget *dialogParent)
    : ProcessBase(id, tabPage, ui, dialogParent)
{
    // 分组框标题：英文原稿声明（语言切换由 UiTranslator 处理）
    ui->gb_blueFilemChipHandover->SetHeaderText(QStringLiteral("Blue Film Chip Status Diagram"));
    ui->gb_operateStepDesc_10->SetHeaderText(QStringLiteral("Operation Steps"));
    ui->gb_result_10->SetHeaderText(QStringLiteral("Result"));


    setConnect();
}

void BlueFilmChipHandoverProc::setConnect()
{

}

bool BlueFilmChipHandoverProc::sameQPoints(const std::vector<QPointF> &a, const std::vector<QPointF> &b)
{
    if (a.size() != b.size())
    {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (a[i] != b[i])
        {
            return false;
        }
    }
    return true;
}

void BlueFilmChipHandoverProc::clearData()
{
    // 重跑前复原：已取走（黑）的芯片回到未取走（黄），点选标注一并清掉
    for (const QPointF &pos : ui->gv_blueFilemChipHandover->diesOfColor(QColor(Qt::black)))
    {
        ui->gv_blueFilemChipHandover->setDieColor(pos, QColor(Qt::yellow));
    }
    ui->gv_blueFilemChipHandover->clearDieMarks();
}

void BlueFilmChipHandoverProc::updateRecipe()
{
    // 芯片尺寸随处方更新（点位方块大小）；晶圆边不画（圆心不可信时不画，见 BlueFilmChipDiagram）
    ui->gv_blueFilemChipHandover->setChipSize(m_rcpChip.material.upChipMaterialInfo.chipWidth,
                                              m_rcpChip.material.upChipMaterialInfo.chipHeight);

    // 注意：点位快照的"查询"不在这里发。本钩子只刷显示，由本页订阅的 sigRecipeUpdated
    // 刷新链调用；查询由本页在选中处方时发一次，见 ProcessPage::requestBlueFilmChips。
}

void BlueFilmChipHandoverProc::ReceiveMctrlCommand(const ProcessMessage &data)
{
    // 芯片状态由控制层发过来（取走一个置黑一个）；协议未定，暂不处理
    //ui->gv_blueFilemChipHandover->takeChip();
    switch (data.command)
    {
        case BlueFilmChipHandoverCMD_AllChipsRes:
        {

            S_BlueFilmChipPositionData res =
                Serializer::from_rawJson<S_BlueFilmChipPositionData>(data.json);

            // 未就绪（蓝膜台还没上报过任何快照）：清图，不弹窗 —— 正常状态，不是错误
            if (res.state == static_cast<int>(BlueFilmChipDataState_NotReady))
            {
                ui->gv_blueFilemChipHandover->setPoints({});
                break;
            }

            // 就绪：全量快照一次进图（图内部 diff：与上次一致不重绘）。
            // 空列表 = 真的没有芯片，同样是正常状态，只清图不弹窗
            ui->gv_blueFilemChipHandover->setPoints(res.chipPos);
        }
        break;

        default:
            break;
    }
}
