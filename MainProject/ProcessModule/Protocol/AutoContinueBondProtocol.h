/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: AutoContinueBondProtocol.h
* @Description: 自动连续键合流程协议：GUI 与 mctrl 两侧共用（自包含，可直接 cp 给 GUI）。
*               自动流程没有注册的流程对象，消息由对应调度中心代发：
*               process=AutoProc_AutoContinueBond，command=本协议命令（从 1 起）。
*               当前为纯发送（流程 → GUI），不接收 GUI 命令。
*********************************************************************************/
#ifndef __AUTO_CONTINUE_BOND_PROTOCOL_H__
#define __AUTO_CONTINUE_BOND_PROTOCOL_H__

#include "istar_serializer.hpp"
#include <QDateTime>

/// 自动连续键合流程命令（流程 → GUI，主动上报）
enum AutoContinueBondCMD
{
    AutoContinueBondCMD_None = 0,
    AutoContinueBondCMD_Finished,   // 自动流程完成（载荷 S_AutoContinueBondFinishedData）
};

/// 单个标记点旁横向/纵向两个子标记相对该标记点的偏移量
/// （上下两侧共用：横向梯形与横向棒形的偏移量一致，纵向同理）
struct S_MarkInspectPointOffset
{
    double xOffsetX = 0.0;    // 横向子标记相对于标记点的偏移量x
    double xOffsetY = 0.0;    // 横向子标记相对于标记点的偏移量y
    double yOffsetX = 0.0;    // 纵向子标记相对于标记点的偏移量x
    double yOffsetY = 0.0;    // 纵向子标记相对于标记点的偏移量y

    YLT_REFL(S_MarkInspectPointOffset, xOffsetX, xOffsetY, yOffsetX, yOffsetY);
};

/// 标记检查输入（自动连续键合启动参数下发：4 个标记点的子标记偏移量 + 识别模板名）
struct S_MarkInspectParam
{
    std::vector<S_MarkInspectPointOffset> points;   // 标记点旁的子标记偏移量（下标即标记点序号，4 个）

    // 识别模板名：梯形（梳子状，下芯片标记）/ 棒形（上芯片标记），
    // 各分键合前（BeforeBond）/ 键合后（AfterBond）两套
    std::string xTrapezoidTplNameBeforeBond;    // 横向梯形识别模板名（键合前）
    std::string xTrapezoidTplNameAfterBond;     // 横向梯形识别模板名（键合后）
    std::string yTrapezoidTplNameBeforeBond;    // 纵向梯形识别模板名（键合前）
    std::string yTrapezoidTplNameAfterBond;     // 纵向梯形识别模板名（键合后）
    std::string xBarTplNameBeforeBond;          // 横向棒形识别模板名（键合前）
    std::string xBarTplNameAfterBond;           // 横向棒形识别模板名（键合后）
    std::string yBarTplNameBeforeBond;          // 纵向棒形识别模板名（键合前）
    std::string yBarTplNameAfterBond;           // 纵向棒形识别模板名（键合后）
    std::string circleTplNameBeforeBond;        // 圆形识别模板名（键合前拍照）
    std::string circleTplNameAfterBond;         // 圆形识别模板名（键合后拍照）

    YLT_REFL(S_MarkInspectParam, points,
             xTrapezoidTplNameBeforeBond, xTrapezoidTplNameAfterBond,
             yTrapezoidTplNameBeforeBond, yTrapezoidTplNameAfterBond,
             xBarTplNameBeforeBond, xBarTplNameAfterBond,
             yBarTplNameBeforeBond, yBarTplNameAfterBond,
             circleTplNameBeforeBond, circleTplNameAfterBond);
};

struct S_AutoContinueBondStartData
{
    bool enable = false;                    // 是否启用
    bool enableAutoStandardSample = false;  // 是否启用自动标样
    bool enableTiming = false;              // 是否启用定时功能
    std::string startTime;                  // 设置开始时间
    double alignStageLiftHeight = 0.0;      // 对准台上升高度

    S_MarkInspectParam markInspect;         // 标记检查参数（子标记偏移量 + 识别模板名，含圆形模板）

    double bondArmDownPos = 0.0;            // 键合臂下降位置
    double pickupWaitTime = 0.0;            // 吸取等待时间
    int singleFieldBondCount = 0;           // 单场循环键合次数
    int totalFieldBondCount = 0;            // 全部场循环键合次数

    YLT_REFL(S_AutoContinueBondStartData, enable, enableAutoStandardSample, enableTiming,
             startTime, alignStageLiftHeight, markInspect,
             bondArmDownPos, pickupWaitTime, singleFieldBondCount, totalFieldBondCount);
};


/// 自动流程完成载荷（AutoContinueBondCMD_Finished 的 JSON 数据）

struct S_AutoContinueBondFinishedData
{
    int status = 0;           // 0=正常完成 1=异常失败
    std::string reason;       // 失败/中止原因（可空）
    double posX = 0.0;        // 完成时键合点 X（可空）
    double posY = 0.0;        // 完成时键合点 Y（可空）
    YLT_REFL(S_AutoContinueBondFinishedData, status, reason, posX, posY);
};

#endif // __AUTO_CONTINUE_BOND_PROTOCOL_H__
