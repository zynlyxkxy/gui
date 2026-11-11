/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: CameraCalibPage.h
* @Description: 相机标定独立页面（图1 传感器标定 / 图2 旋转中心标定）。
*
*               本页【不是】ProcessPage 的 tab：流程 SingleProc_CameraCalib 没有 tab，
*               启停由本页的「开始标定 / 结束标定」两个按钮控制（进页面不自动开标定）：
*                 开始标定 → Start（流程跑起来进交互态）→ 收 EnterInteract 解锁动作区
*                 → 每按一个动作按钮发一条命令（发出即置灰，等流程再推 EnterInteract 解锁）
*                 → 结束标定 → Stop（本流程不会自己结束，会一直占着任务槽）
*
*               使能逻辑沿用 ProcessPage 那套（见 ChipPosCalibProc）：
*                 m_interact 总开关 —— EnterInteract 后动作区全亮，点过任一动作按钮即全灰；
*                 setStatus(MctrlStatus) —— 与 ProcessBase 同源的状态机驱动，
*                   开始/结束标定【只认 mctrl 状态】：BUSY = 有会话在跑（只能结束）、IDLE = 可以开始，
*                   本地不留"我发过 Start"的副本 —— GUI 重启后状态一推回来按钮就回到真实状态，
*                   不会出现"控制层还在跑、界面却按空闲使能"的死锁
*
*               GUI 不碰硬件：算法/运动/视觉调用全在流程侧（mctrl）完成，
*               本页只负责「发命令 + 显示回包 + 显示错误信息」。
*********************************************************************************/
#ifndef CAMERACALIBPAGE_H
#define CAMERACALIBPAGE_H

#include <QWidget>
#include <QList>
#include <QString>

#include "Common/UiTranslator.h"
#include "RPCModule/MCTRLClientManager.h"
#include "mctrl_client.h"
#include "StatusModule/StatusManager.h"                   // MctrlStatus（流程主状态）

#include "ProcessModule/Protocol/CameraCalibProtocol.h"   // CameraCalibCMD + 各步载荷

class QComboBox;
class QLineEdit;
class QPushButton;

namespace Ui {
class CameraCalibPage;
}

/**
 * 相机标定页：两张设计图各自一块 GroupBox —— 图1 传感器标定（相机夹角 / x 像素比 /
 * y 像素比三条平级链）、图2 旋转中心标定（转角度 / 取 5 个模板中心 / 算旋转中心）。
 *
 * 相机信息（模板名 / 标定项名 / 图像分辨率）由初始化模块采集，组装成
 * S_CameraCalibBaseInfo 随初始化命令一次性下发（之后不再重发）；
 * 相机上下方位与相机 id 不由 GUI 指定：流程按标定项推导（TopCameraAlignStage = 上相机，
 * 其余 = 下相机，id 从机台常数取），移动哪个台子同理。
 */
class CameraCalibPage : public QWidget
{
    Q_OBJECT

public:
    explicit CameraCalibPage(QWidget *parent = nullptr);
    ~CameraCalibPage() override;

    // 整页翻译（语言切换 / 启动恢复时 MainWindow 调用）
    void refreshTexts();

private:
    // ---- 初始化 ----
    void initUi();      // 控件初值 + 标定项下拉装配 + 结果框只读
    void initStyle();   // 样式铺底（沿用 ProcessPage 的房屋样式）
    void initConnect();

    // ---- 流程消息（观察者广播，按 process 过滤后按 command 分派）----
    void onMctrlMessage(const MctrlModule::ProcessMessage &data);

    // ---- GUI → 流程：每步一条命令，载荷见 CameraCalibProtocol.h ----
    void sendCommand(int command, const std::string &json);   // 统一异步下发（发出即置灰动作区）
    void onStartCalib();                                      // 开始标定：Start 流程
    void onStopCalib();                                       // 结束标定：Stop 流程
    void sendInit();                                          // 初始化：就一个按钮，流程按标定项分流
    void sendSensorCalib(int command, double moveMm);         // 图1 三条链（共用一份载荷）
    void sendRotateAxis(double deg);                          // 图2 转角度
    void sendMarkCenter(int index);                           // 图2 模板中心 N
    void sendCalRotCenter();                                  // 图2 计算旋转中心
    void sendSave();                                          // 保存标定数据（全量落盘）

    // ---- 使能（同 ProcessPage / ChipPosCalibProc 的模式）----
    // 流程状态变化：BUSY（会话中）按 m_interact 亮动作区，其余一律全灰
    void setStatus(MctrlStatus status);
    // 动作区总开关（上/下使能）：交互期全亮随意点，合法性点击时校验；另含开始/结束标定互斥
    void setActionsEnabled(bool enabled);

    // ---- 显示 ----
    void clearOutputs();                   // 清掉上一轮的结果
    void setDegResult(double deg);
    void setScaleResult(bool horizon, double scale);
    void setMarkResult(int index, double axisX, double axisY);
    void setRotCenterResult(double centerX, double centerY);
    void showError(const QString &msg);      // 失败弹窗（流程回的 errorInfo）
    void showInfo(const QString &msg);       // 成功弹窗（每条请求的结果都要有反馈）

    S_CameraCalibBaseInfo baseInfo() const;   // 组装相机信息（初始化模块：标定项/模板名/分辨率）

private:
    Ui::CameraCalibPage *ui;
    UiTranslator m_uiTranslator;

    QList<QLineEdit *> m_outputEdits;   // 只读结果框（objectName 以 le_out 开头）

    bool m_interact = false;   // EnterInteract 后允许操作（点任一动作按钮即置回 false 并置灰）
};

#endif // CAMERACALIBPAGE_H
