/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: AccuracyInspectProtocol.h
* @Description: 精度检测流程（私有单步，键合后）协议定义。
*               流程作用：键合后上下芯片标记贴在同一界面高度，下平面相机透过
*               上芯片拍摄，同一拍照位置用梯形模板（下芯片标记）与棒形模板
*               （透过上芯片识别上芯片标记）各识别一次；对准台预先上升
*               alignStageLiftHeight 补偿折射。不对 GUI 上报，仅供内部使用。
*               输入：复用标记检查的 S_MarkInspectParam（模板用键合后 AfterBond
*               系列）+ alignStageLiftHeight；输出：上/下 × 横/纵各 4 条识别记录，
*               向量下标即主标记编号。
*********************************************************************************/
#ifndef __ACCURACY_INSPECT_PROTOCOL_H__
#define __ACCURACY_INSPECT_PROTOCOL_H__

#include "MarkInspectProtocol.h"   // S_MarkInspectParam / S_MarkInspectMarkData（复用标记检查结构）

/// 精度检测输出（流程执行时填写：上横向/上纵向/下横向/下纵向各 4 条识别记录，
/// 向量下标即主标记编号）
struct S_AccuracyInspectData
{
    S_MarkInspectParam param;                            // 输入参数（调度中心启动前写入）
    double alignStageLiftHeight = 0.0;                   // 输入：对准台上升高度（键合前标定，折射补偿用）
    bool valid = false;                                  // 数据是否有效（本流程已执行并写入）
    std::vector<S_MarkInspectMarkData> upXMarksData;     // 上芯片横向子标记识别记录（4 条）
    std::vector<S_MarkInspectMarkData> upYMarksData;     // 上芯片纵向子标记识别记录（4 条）
    std::vector<S_MarkInspectMarkData> downXMarksData;   // 下芯片横向子标记识别记录（4 条）
    std::vector<S_MarkInspectMarkData> downYMarksData;   // 下芯片纵向子标记识别记录（4 条）

    YLT_REFL(S_AccuracyInspectData, param, alignStageLiftHeight, valid,
             upXMarksData, upYMarksData, downXMarksData, downYMarksData);
};

#endif // __ACCURACY_INSPECT_PROTOCOL_H__
