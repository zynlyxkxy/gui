#ifndef BLUEFILMLOADUNLOAD_H
#define BLUEFILMLOADUNLOAD_H

#include "MaterialModule/Tabs/MaterialTabBase.h"
#include "RPCModule/MCTRLClientManager.h"
#include "mctrl_client.h"
#include "ProcessModule/Protocol/ProcessManagerProtocol.h"

/**
 * 蓝膜台上下片（Blue Film Stage Load/Unload，tab 1 = ui->tabWidget->widget(1)，.ui 里叫 blueFilm）。
 * 内容：蓝膜片状态图（BlueFilmChipDiagram）+ 蓝膜信息（缺口角度 / 扩膜深度）
 *       + 三个手动操作按钮（移动到上片位 / 扩膜与映射 / 移动到取片位）。
 */
class BlueFilmLoadUnload : public MaterialTabBase
{
public:
    BlueFilmLoadUnload(int id, QWidget *tabPage, Ui::MaterialPage *ui, QWidget *dialogParent);

    void clearData() override;
    void updateRecipe() override;

protected:
    void setConnect() override;

private:
    void onMoveToLoadPos();        // 移动到上片位
    void onExpansionMapping();     // 扩膜与映射
    void onMoveToPickPos();        // 移动到取片位

    void ReceiveMctrlCommand(const ProcessMessage &data) override;
};

#endif // BLUEFILMLOADUNLOAD_H
