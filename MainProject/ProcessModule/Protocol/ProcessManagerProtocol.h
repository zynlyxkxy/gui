/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: ProcessManagerProtocol.h
* @Description: 流程管理器协议：GUI 与 mctrl 两侧共用（自包含，可直接 cp 给 GUI）。
*               消息为 (process, command, json)：
*                 - 请求（GUI → mctrl）：Start / Stop；
*                 - 答复（mctrl → GUI）：Response——GUI 请求的执行情况（请求-回复式）；
*                 - 事件（调度中心 → GUI）：SingleProcFinished——单步任务完成（主动上报式）；
*                   自动流程完成走各自动流程自己的协议（如 AutoContinueBondProtocol.h）。
*********************************************************************************/
#ifndef __PROCESS_MANAGER_PROTOCOL_H__
#define __PROCESS_MANAGER_PROTOCOL_H__

#include "istar_serializer.hpp"

/// 主流程名（GUI 对 manager 发 Start/Stop 的流程名，如"全自动"）
inline constexpr char kAutoRunProcessName[] = "AutoRun";

// ============================================================================
// 业务流程 ID 枚举：枚举值即流程 ID（ProcessManager 按此统一分配，保持稳定，
// 不依赖注册顺序）。流程路由键字符串由平台 ISTAREnumConverter 从本枚举转换
// （ConvertToString，枚举值名即字符串，GUI 与 mctrl 两侧共用保证一致）。
// 两类流程：
//   - SingleProc_*：单流程，注册到 ThreadProc，manager 点对点启停；
//   - AutoProc_*  ：组装式流程（由单流程按序组装，前序结束自动启动后序），
//                   不注册，由 manager 在流程结束回调中编排。
// GUI 可引用本枚举取流程 ID / 路由键。
// ============================================================================

/// 流程 ID 枚举（枚举值即流程 ID；枚举值名即流程路由键字符串）
/// ⚠ 单步流程编号 = ProcessPage tab 下标（tab 0 为处方预览，无流程）。
///   tab 顺序即协议值，冻结：新 tab 只许追加尾部；调整顺序必须与 mctrl 同步。
enum class ProcessID
{
    None = 0,                       // 无效流程
    // ---- 用户可以直接控制的单步流程：编号 = ProcessPage tab 下标 ----
    SingleProc_BlueFilmChipHandover = 1,   // 蓝膜台芯片交接
    SingleProc_ChuckLevel           = 2,   // 上下吸盘调平
    SingleProc_WaferLevelCollect    = 3,   // 晶圆调平数据采集
    SingleProc_ChipPosCalib         = 4,   // 芯片位置标定
    SingleProc_CoarseLevel          = 5,   // 粗调平
    SingleProc_DownLevelSelect      = 6,   // 下调平点位选择
    SingleProc_UpLevelSelect        = 7,   // 上调平点位选择
    SingleProc_FineLevel            = 8,   // 精调平
    SingleProc_FineAlign            = 9,   // 精对准
    SingleProc_Bond                 = 10,   // 键合（单步，框架）
    SingleProc_Init                 = 11,  // 初始化流程（无 GUI 数据上报，无 tab）
    SingleProc_WaferLoad            = 12,  // 晶圆上片（上下片为操作员动作，MaterialPage 触发，无 ProcessPage tab）
    SingleProc_WaferUnload          = 13,  // 晶圆下片
    SingleProc_BlueFilmLoad         = 14,  // 蓝膜上片
    SingleProc_BlueFilmUnload       = 15,  // 蓝膜下片
    SingleProc_CameraCalib          = 16,  // 相机标定（独立页面 CameraCalibPage 触发，无 ProcessPage tab）
    // ⚠ 上面新增一项使其后各隐式编号整体 +1（PrivateSingleProc_* 由 16..20 → 17..21）；
    //    GUI 与 mctrl 两份本文件必须同步重建，切勿只更新一侧。
    _SingleProc_MAX,                    // 单步流程边界：None < id < _SingleProc_MAX 即单步流程
    // ---- 用户不可直接控制的单步（无对应 tab），继续编号 ----
    PrivateSingleProc_MoveBondPoint,
    PrivateSingleProc_MarkInspect,      // 标记检查（查看 键合芯片4 个标记点位旁 A/B 共 8 个标记并记录）
    PrivateSingleProc_AccuracyInspect,  // 精度检测（键合后，透过上芯片测上下标记位置差）
    PrivateSingleProc_ChipReturnToFlip, // 芯片返回翻转模块
    PrivateSingleProc_BondHeadPickChip, // 键合头吸取芯片
    _PrivateSingleProc_MAX,             // 用户不可直接控制的单步：_SingleProc_MAX < id < _PrivateSingleProc_MAX 即是
    // 自动流程
    AutoProc_AutoProcess,               //自动流程
    AutoProc_AutoContinueBond,          // 自动连续键合流程
};

// ============================================================================
// 命令协议：0-99 为流程私有命令（各流程自己的协议，本文件不含）；
// 100+ 为任务层命令（本文件）：请求 / 答复 / 完成事件
// ============================================================================

/// 任务层命令（请求 / 答复 / 事件）
enum ProcessManagerCMD
{
    ProcessManagerCMD_None = 0,

    // ---- 请求（GUI → mctrl）----
    ProcessManagerCMD_Start = 100,        // 启动任务（json 带启动参数，由流程解析）
    ProcessManagerCMD_Stop = 101,         // 停止当前任务

    // ---- 答复（mctrl → GUI，GUI 请求的执行情况，请求-回复式）----
    ProcessManagerCMD_Response = 102,     // 答复（载荷 S_ProcessManagerResponseData）

    // ---- 事件（调度中心 → GUI，单步任务完成，主动上报式）----
    ProcessManagerCMD_SingleProcFinished = 103,  // 单步流程完成（载荷 S_SingleProcFinishedData；process=单步流程 ID）
    // 自动流程完成：走各自动流程自己的协议（如 AutoContinueBondProtocol.h，命令从 1 起）

    // ---- 事件（mctrl → GUI，进度上报，主动上报式）----
    ProcessManagerCMD_Progress = 104,            // 进度上报（载荷 S_ProcessProgressData；process=流程 ID）
};

/// 答复载荷（ProcessManagerCMD_Response 的 JSON 数据）：GUI 请求是否执行成功
struct S_ProcessManagerResponseData
{
    bool success = false;     // 请求是否执行成功
    std::string errorInfo;    // 失败原因（成功时为空；如"重复启动/无活动任务/未知流程"）
    YLT_REFL(S_ProcessManagerResponseData, success, errorInfo);
};

/// 单步流程完成载荷（ProcessManagerCMD_SingleProcFinished 的 JSON 数据）
struct S_SingleProcFinishedData
{
    int status = 0;           // 0=正常完成 1=异常失败
    std::string reason;       // 失败原因（非报警失败时 GUI 的唯一线索；正常完成可空）
    YLT_REFL(S_SingleProcFinishedData, status, reason);
};

/// 进度载荷（ProcessManagerCMD_Progress 的 JSON 数据）
struct S_ProcessProgressData
{
    int percent = 0;          // 0~100，由控制层决定
    YLT_REFL(S_ProcessProgressData, percent);
};

#endif // __PROCESS_MANAGER_PROTOCOL_H__
