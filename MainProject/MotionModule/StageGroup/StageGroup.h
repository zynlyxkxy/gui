#ifndef STAGE_GROUP_H
#define STAGE_GROUP_H

#include <QWidget>
#include <QDoubleSpinBox>
#include <QWheelEvent>
#include <QPushButton>
#include <QHash>
#include <QList>
#include <string>
#include <vector>

#include "mctrl_client.h"

#define TR_STR(msg)  QString::fromStdString(Tr(msg))
#define TR_STRQ(msg) QString::fromStdString(Tr((msg).toStdString()))

class NoWheelDoubleSpinBox : public QDoubleSpinBox
{
    Q_OBJECT

protected:
    void wheelEvent(QWheelEvent *event) override
    {
        event->ignore();
    }
};

namespace Ui { class StageGroup; }
class MCTRLClientManager;

class StageGroup : public QWidget
{
    Q_OBJECT

public:
    explicit StageGroup(const MctrlModule::StageInfo &info,
                        MCTRLClientManager *client,
                        QWidget *parent = nullptr);
    ~StageGroup();

    void onObserverData(const MctrlModule::StageObserverData &data);
    void setRpcClient(MCTRLClientManager *client)
    {
        m_client = client;
    }
    std::string stageName() const { return m_stageName; }

    /// @brief 按轴数计算的期望高度(30px/轴 + 210px 固定部分)。
    /// 两列排布时 CommandsTab 用它对齐同一行两张卡片的高度。
    int desiredHeight() const { return 30 * m_axisCount + 210; }

    // 语言切换：按当前平台语言重刷按钮/表头/标题文案
    void refreshTexts();

signals:
    void sig_ObserverReceived(const MctrlModule::StageObserverData &data);

private:
    void setupAxisRows(const std::vector<std::string> &axisNames);
    void connectButtons();
    void applyStyles();

    Ui::StageGroup *ui;
    MCTRLClientManager *m_client;
    std::string m_stageName;
    std::string m_stageType;
    int m_axisCount;

    QHash<int, QDoubleSpinBox *> m_dic_CurrentPos;
    QHash<int, QDoubleSpinBox *> m_dic_CurrentVel;
    QList<QDoubleSpinBox *> m_lst_TargetAbs;
    QList<QDoubleSpinBox *> m_lst_TargetRel;
    QList<QPushButton *> m_lst_RelHold;

    QPushButton *btn_Home;
    QPushButton *btn_Stop;
    QPushButton *btn_MoveRel;
    QPushButton *btn_MoveAbs;
};

#endif // STAGE_GROUP_H
