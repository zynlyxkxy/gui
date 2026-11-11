#ifndef BLUEFILMCHIPHANDOVERPROC_H
#define BLUEFILMCHIPHANDOVERPROC_H

#include "ProcessModule/Base/ProcessBase.h"
#include "ProcessModule/Protocol/BlueFilmChipHandoverProtocol.h"

#include <QPointF>
#include <vector>

/**
 * 蓝膜台芯片交接（Blue Film Chip Handover）流程：
 * - 页面只有一张蓝膜芯片状态图（黄 = 未取走，黑 = 已取走），底图为蓝膜晶圆；
 * - 处方只提供底图（蓝膜晶圆尺寸），芯片点位与"已取走"事件由控制层下发
 *   （协议见 ProcessModule/Protocol/BlueFilmChipHandoverProtocol.h，命令暂未定义）；
 * - clearData 只把图复原为"未取走"，处方数据不在此清。
 */
class BlueFilmChipHandoverProc : public ProcessBase
{
public:
    BlueFilmChipHandoverProc(int id, QWidget *tabPage, Ui::ProcessPage *ui, QWidget *dialogParent);

    void clearData() override;
    void updateRecipe() override;
    void ReceiveMctrlCommand(const ProcessMessage &data) override;

protected:
    void setConnect() override;

private:

    // 点表逐项比较（真实坐标点；QPointF 无逐项比较工具）
    static bool sameQPoints(const std::vector<QPointF> &a, const std::vector<QPointF> &b);
};

#endif // BLUEFILMCHIPHANDOVERPROC_H
