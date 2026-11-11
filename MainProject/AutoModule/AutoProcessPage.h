#ifndef AUTOPROCESSPAGE_H
#define AUTOPROCESSPAGE_H

#include <QWidget>
#include "Common/UiTranslator.h"
#include "mctrl_client.h"
#include "ProcessModule/Components/DiagramComponent/DownWaferDiagram.h"
#include "StatusModule/StatusManager.h"
#include "chart/customchart.h"

#include <map>
#include <QDateTime>

using namespace ISTAR::GUI;

namespace Ui {
class AutoProcessPage;
}

class RecipeChooseWidget;
class BondCurveRecorder;   // 键合曲线采集器（Common/BondCurveRecorder.h）

struct QPointFLess
{
    bool operator()(const QPointF &a, const QPointF &b) const
    {
        return a.x() != b.x() ? a.x() < b.x() : a.y() < b.y();
    }
};


class AutoProcessPage : public QWidget
{
    Q_OBJECT

public:
    explicit AutoProcessPage(QWidget *parent = nullptr);
    ~AutoProcessPage();

    void refreshTexts();

protected:
    // 切到本页时补一次 replot —— 首次显示时布局才最终确定，否则 QCustomPlot 按"尺寸未定"
    // 时的布局画，轴标签里带单位的后缀（如 (s)/(℃)/(N)）会画不出来。
    // 处方显示不在这里补刷：订阅 MCRecipeManager::sigRecipeUpdated 即可。
    void showEvent(QShowEvent *event) override;
private:
    void initConnect();
    void initStyle();

    // ---- 下晶圆图-----
    void initDiagram();
    void refreshDownWaferDiagram(QPointF pos, DownWaferDiagram::ChipState state);
    void updateBondInfo(QPointF pos);

    void startAutoProcess();
    void stopAutoProcess();
    void sendManagerCommand(int processId, int command);

    // ---- 处方 MC ----
    void onRecipeUpdated();       // 预览填充 + initDiagram（sigRecipeUpdated 触发，只刷显示）

    void refreshRunControls(MctrlModule::MctrlStatus prev, MctrlModule::MctrlStatus curr);

    void onMctrlMessage(const MctrlModule::ProcessMessage &data);

    // ---- 键合曲线 ----
    void ensureChart();      // 建图 + 固定轴（空数据也有坐标轴）
    void configureAxes();    // 只转发到 BondCurve::applyAxisRange()（轴定义在那里）
    void clearData();

private:
    Ui::AutoProcessPage *ui;
    RecipeChooseWidget *m_pRecipeDlg;
    UiTranslator m_uiTranslator;

    std::map<QPointF, DownWaferDiagram::ChipBondInfo , QPointFLess> m_bondInfo;

    bool m_isSelectRecipe = false;   // 已选处方（Start 门槛；控制层重启后要求重选）
    bool m_blueFilmReplyPending = false;   // 本页刚发过蓝膜查询，等它的答复（谁发谁报错）

    QPointF m_curChip;

    CustomChart *p_Chart = nullptr;

    BondCurveRecorder *m_curve = nullptr;   // 键合曲线采集器（父对象 = this）
};

#endif // AUTOPROCESSPAGE_H
