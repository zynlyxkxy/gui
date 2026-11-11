/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: MarkInspectProtocol.h
* @Description: 标记检查流程（私有单步，用户不可主动触发）协议定义（仅输出段）。
*               流程作用：上下芯片各 4 个标记点，每个标记点旁有横向/纵向两个
*               子标记（下芯片为梳子状梯形模板，上芯片为棒形模板），共 16 次
*               识别并记录位置。不对 GUI 上报，识别记录仅供内部使用。
*               输入参数（S_MarkInspectParam）定义在 AutoContinueBondProtocol.h
*               （GUI 经自动连续键合启动参数下发），本文件 include 该文件。
*********************************************************************************/
#ifndef __MARK_INSPECT_PROTOCOL_H__
#define __MARK_INSPECT_PROTOCOL_H__

#include "AutoContinueBondProtocol.h"   // S_MarkInspectParam（输入参数定义在自动连续键合协议）

#include <vector>

/// 单个标记识别记录（输出，与 S_FineAlignMarkData 同风格；
/// 所在向量的下标即主标记编号，模板名随记录保存）
struct S_MarkInspectMarkData
{
    std::string tplName;      // 识别使用的模板名
    bool ok = false;          // 是否识别到标记
    double nominalX = 0.0;    // 名义坐标 x（标记点名义 + 子标记偏移量，经对准台中心换算，机器坐标 mm）
    double nominalY = 0.0;    // 名义坐标 y
    double actualX = 0.0;     // 实际坐标 x（识别 + 像素修正后的准确位置，机器坐标 mm）
    double actualY = 0.0;     // 实际坐标 y
    double pixelX = 0.0;      // 识别像素坐标 x
    double pixelY = 0.0;      // 识别像素坐标 y
    double angle = 0.0;       // 识别旋转角
    double score = 0.0;       // 匹配分数
    YLT_REFL(S_MarkInspectMarkData, tplName, ok,
             nominalX, nominalY, actualX, actualY,
             pixelX, pixelY, angle, score);
};

/// 标记检查输出（流程执行时填写：上横向/上纵向/下横向/下纵向各 4 条识别记录，
/// 向量下标即主标记编号）
struct S_MarkInspectData
{
    S_MarkInspectParam param;                            // 输入参数（调度中心启动前写入）
    bool valid = false;                                  // 数据是否有效（本流程已执行并写入）
    std::vector<S_MarkInspectMarkData> upXMarksData;     // 上芯片横向子标记识别记录（4 条）
    std::vector<S_MarkInspectMarkData> upYMarksData;     // 上芯片纵向子标记识别记录（4 条）
    std::vector<S_MarkInspectMarkData> downXMarksData;   // 下芯片横向子标记识别记录（4 条）
    std::vector<S_MarkInspectMarkData> downYMarksData;   // 下芯片纵向子标记识别记录（4 条）

    YLT_REFL(S_MarkInspectData, param, valid,
             upXMarksData, upYMarksData, downXMarksData, downYMarksData);
};

#endif // __MARK_INSPECT_PROTOCOL_H__
