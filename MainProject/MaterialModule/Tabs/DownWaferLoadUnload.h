#ifndef DOWNWAFERLOADUNLOAD_H
#define DOWNWAFERLOADUNLOAD_H

#include "MaterialModule/Tabs/MaterialTabBase.h"
#include "RPCModule/MCTRLClientManager.h"
#include "mctrl_client.h"
#include "ProcessModule/Protocol/WaferLoadProtocol.h"
#include "ProcessModule/Protocol/WaferUnLoadProtocol.h"
#include "ProcessModule/Protocol/ProcessManagerProtocol.h"
/**
 * 下晶圆上下片（Down Wafer Load/Unload，tab 0 = ui->tabWidget->widget(0)，.ui 里叫 downWafer）。
 * 四个手动操作按钮：移动到上片位 / 手动上片完成 / 移动到下片位 / 手动下片完成。
 * 按钮文本在 MaterialPage.ui 里声明（英文原稿），语言切换由 UiTranslator 懒惰捕获，
 * 本类不重复声明文本。
 */
class DownWaferLoadUnload : public MaterialTabBase
{
public:
    DownWaferLoadUnload(int id, QWidget *tabPage, Ui::MaterialPage *ui, QWidget *dialogParent);

    void clearData() override;

protected:
    void setConnect() override;

private:
    void onMoveToLoadPos();        // 移动到上片位
    void onManualLoadComplete();   // 手动上片完成
    void onMoveToPickPos();        // 移动到取片位
    void onManualUnLoadComplete(); // 手动下片完成

    void sendManagerCommand(int processId, int command);

    void ReceiveMctrlCommand(const ProcessMessage &data) override;
};

#endif // DOWNWAFERLOADUNLOAD_H
