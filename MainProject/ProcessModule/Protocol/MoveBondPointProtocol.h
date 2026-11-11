/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: MoveBondPointProtocol.h
* @Description: 键合点对位流程协议：输入数据由本流程段自管理（调度中心写入，
*               MoveBondPointProc 读取），不借用其他流程的数据段。
*********************************************************************************/
#ifndef __MOVE_BOND_POINT_PROTOCOL_H__
#define __MOVE_BOND_POINT_PROTOCOL_H__

/// 键合点对位流程数据（数据中心 S_ProcessData 的 moveBondPointData 段）
struct S_MoveBondPointData
{
    int pointIndex = -1;      // 输入：当前要处理的下晶圆键合点序号（调度中心下发；-1=未设置）
};

#endif // __MOVE_BOND_POINT_PROTOCOL_H__
