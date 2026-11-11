/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: CameraCalibProtocol.h
* @Description: 相机标定流程与 GUI 的协议定义（命令枚举 + 载荷结构体）。
*               本文件自包含（仅依赖 istar_serializer.hpp），可直接交给 GUI 侧解析。
*
*               流程 ID：ProcessID::SingleProc_CameraCalib
*               该流程【没有】ProcessPage tab，由独立页面触发，完全由 GUI 指挥：
*               GUI Start → 流程进人机交互态（推 EnterInteract）→ GUI 发一条命令
*               → 流程执行该条链 → 回包 → Record/Finish（任务结束）。
*
*               当前实现：图1 传感器标定
*                 - 相机夹角（移 x）
*                 - x 方向像素比（移 x）
*                 - y 方向像素比（移 y）
*               三条链互相独立、无任何前置条件、顺序不限。
*               图2 旋转中心标定待开发，协议暂不含其命令。
*********************************************************************************/
#ifndef __CAMERA_CALIB_PROTOCOL_H__
#define __CAMERA_CALIB_PROTOCOL_H__

#include "istar_serializer.hpp"   // YLT_REFL（结构体 → JSON）

#include <string>

// ============================================================================
// 与 GUI 的协议枚举（普通 enum，值带流程名前缀避免全局冲突）
// 请求（GUI→流程）与响应（流程→GUI）的 cmd 统一放在一起；
// 注意：结构体名不得与枚举值同名（同名作用域冲突编不过），故结构体统一加 S_ 前缀
// ============================================================================

/// 相机标定流程所有可能的 GUI cmd
enum CameraCalibCMD
{
    CameraCalibCMD_None = 0,

    // ---- 请求（GUI → 流程）----
    // 初始化：就一条命令（GUI 一个按钮），流程按标定项自己分流：
    //   BottomCameraXYRz（旋转中心项） → iStarAxisRotCntResInit → 下发模板
    //   其余（传感器项）             → iStarSensorResInit → iStarSetSensorResolution → 下发模板
    // 没初始化之前，其它命令都会被流程拒（"没有初始化其他操作不可以做"）。
    CameraCalibCMD_Init,               // 初始化（载荷 S_CameraCalibInitReq）

    // 图1 三条独立动作链：载荷完全一致（S_CameraCalibSensorReq），
    // 只是移动方向、调用的 iStar 接口、回包命令各不相同，故保留三个命令。
    // 相机信息（模板名/相机id/相机方位）由 Init 保存，不再逐条下发。
    CameraCalibCMD_CalSensorDeg,       // 图1 计算相机夹角 θ（移动 x）
    CameraCalibCMD_CalSensorScaleX,    // 图1 计算 x 方向像素比（移动 x）
    CameraCalibCMD_CalSensorScaleY,    // 图1 计算 y 方向像素比（移动 y）

    // 图2 旋转中心标定：四个命令各是一条独立分支
    CameraCalibCMD_RotateAxis,         // 图2 硅片台转角度（载荷 S_CameraCalibRotateReq）
    CameraCalibCMD_MarkTemplateCenter, // 图2 模板中心 N：拍照识别 + 换算轴坐标（载荷 S_CameraCalibMarkReq）
    CameraCalibCMD_CalRotCenter,       // 图2 计算旋转中心（无载荷，用已取到的模板中心点）
    CameraCalibCMD_Save,               // 保存标定数据（无载荷，iStarSaveCalibData 全量落盘）

    // ---- 响应（流程 → GUI）----
    CameraCalibCMD_EnterInteract,      // 进入人机交互态（交互就绪，GUI 可下发一条命令）
    CameraCalibCMD_InitResult,         // 初始化结果（载荷 S_CameraCalibCmdAck，无业务数据）
    CameraCalibCMD_SensorDegResult,    // 夹角结果（载荷 S_CameraCalibDegResult）
    CameraCalibCMD_SensorScaleResult,  // 像素比结果（载荷 S_CameraCalibScaleResult，含 horizon 区分 x/y）
    CameraCalibCMD_RotateAxisResult,   // 转角度执行结果（载荷 S_CameraCalibCmdAck）
    CameraCalibCMD_MarkCenterResult,   // 模板中心结果（载荷 S_CameraCalibMarkResult，index 原样回传）
    CameraCalibCMD_RotCenterResult,    // 旋转中心结果（载荷 S_CameraCalibRotCenterResult）
    CameraCalibCMD_SaveResult,         // 保存结果（载荷 S_CameraCalibCmdAck）
    CameraCalibCMD_ErrorMessage,       // 错误信息（载荷 S_CameraCalibCmdAck）——失败/被拒统一走这条
};

// ============================================================================
// 公共信息与错误信息
// ============================================================================

/// 相机标定基础信息（各请求共用的相机/标定项描述，全部由 GUI 下发）
/// 相机 id 与上/下位置不由 GUI 下发：由标定项决定（TopCameraAlignStage = 上相机，其余 = 下相机），
/// 相机 id 从机台常数 alignStage.up/downFineAlignCamera.cameraId 取
struct S_CameraCalibBaseInfo
{
    std::string cameraTemplateName; // 相机识别模板名（流程下发给搜点拍照子流程）
    std::string calibItemName;      // 标定项内部名称，即各 iStar 接口的 sensorNumber 入参
    int resolutionX = 0;            // 图像宽（像素）——喂 iStarSetSensorResolution 的 imgw（<=0 = 没配，初始化会失败）
    int resolutionY = 0;            // 图像高（像素）——喂 imgh
    YLT_REFL(S_CameraCalibBaseInfo, cameraTemplateName, calibItemName, resolutionX, resolutionY);
};

/// 错误信息载荷（CameraCalibCMD_ErrorMessage 的 JSON 数据）
/// 约定：不报警，所有失败（链路失败/参数解析失败/上一条未执行完）都由此命令告知 GUI
/// ⚠ errorInfo 一律填 ASCII 英文：中文经 RPC 传到 GUI 会乱码（各结果结构体的 errorInfo 同此约定）
struct S_CameraCalibCmdAck
{
    bool ok = false;            // 是否执行成功（错误信息固定 false）
    std::string errorInfo;      // 失败原因（ASCII 英文，人读）
    YLT_REFL(S_CameraCalibCmdAck, ok, errorInfo);
};

// ============================================================================
// 初始化
// ============================================================================

/// 初始化请求载荷（CameraCalibCMD_Init 的 JSON 数据）
/// 流程收到后：保存这份相机信息（后面所有命令都用它，不再逐条下发）→ 下发模板
/// → 按标定项类型把标定项字符串下发给对应 iStar 接口（传感器那套还会先设分辨率）。
/// 没初始化之前，其它命令一律被拒。
struct S_CameraCalibInitReq
{
    S_CameraCalibBaseInfo baseInfo;   // 相机/标定项信息（模板名 / 标定项名 / 分辨率）
    YLT_REFL(S_CameraCalibInitReq, baseInfo);
};

// ============================================================================
// 图1 传感器标定
// ============================================================================

/// 图1 请求载荷（CameraCalibCMD_CalSensorDeg / _CalSensorScaleX / _CalSensorScaleY 共用）
/// 三条链要送过来的参数是一致的，故只有移动量一项；相机信息由 Init 保存
struct S_CameraCalibSensorReq
{
    double moveMm = 0.3;              // 第一次取像后移动的距离，单位 mm（夹角与 x 像素比走 x，y 像素比走 y）
    YLT_REFL(S_CameraCalibSensorReq, moveMm);
};

/// 图1 夹角结果（CameraCalibCMD_SensorDegResult 的 JSON 数据）
/// 对应 iStarCalSensorDeg(标定项, 像素点1, 像素点2, out deg)
struct S_CameraCalibDegResult
{
    bool ok = false;            // 是否计算成功
    double deg = 0.0;           // 相机夹角 θ，单位 °
    std::string errorInfo;      // 失败原因（成功时为空）
    YLT_REFL(S_CameraCalibDegResult, ok, deg, errorInfo);
};

/// 图1 像素比结果（CameraCalibCMD_SensorScaleResult 的 JSON 数据）
/// 对应 iStarCalSensorScale(标定项, 像素点1, 光栅点1, 像素点2, 光栅点2, out scale, horizon)
struct S_CameraCalibScaleResult
{
    bool ok = false;            // 是否计算成功
    bool horizon = true;        // true = x 方向；false = y 方向（与 iStarCalSensorScale 的 horizon 同义）
    double scale = 0.0;         // 像素比
    std::string errorInfo;      // 失败原因（成功时为空）
    YLT_REFL(S_CameraCalibScaleResult, ok, horizon, scale, errorInfo);
};

// ============================================================================
// 图2 旋转中心标定
// ============================================================================

/// 图2 转角度请求载荷（CameraCalibCMD_RotateAxis 的 JSON 数据）
/// 硅片台 Rz 旋转：GUI 下发的是相对角度（有方向，正负表示转向），
/// 流程按"当前位置 + deg"换算成绝对目标下发（与对准台移动同口径）
struct S_CameraCalibRotateReq
{
    double deg = 0.0;           // 相对旋转角度，单位 °（有方向）
    YLT_REFL(S_CameraCalibRotateReq, deg);
};

/// 图2 模板中心请求载荷（CameraCalibCMD_MarkTemplateCenter 的 JSON 数据）
/// 5 个按钮做的是同一件事，index 只是序号（流程按序号存点，并原样回包给 GUI）；
/// 模板已在 Init 时下发，这里不再带相机信息
struct S_CameraCalibMarkReq
{
    int index = 1;                    // 模板中心序号 1~5
    YLT_REFL(S_CameraCalibMarkReq, index);
};

/// 图2 模板中心结果（CameraCalibCMD_MarkCenterResult 的 JSON 数据）
/// 对应 VisionManager::PixelToRuler（iStarGetRulerByPixel），即"目标中心实际轴坐标"
struct S_CameraCalibMarkResult
{
    bool ok = false;            // 是否取点成功
    int index = 1;              // 原样回传请求里的序号 1~5
    double axisX = 0.0;         // 目标中心实际轴坐标 x，单位 mm
    double axisY = 0.0;         // 目标中心实际轴坐标 y，单位 mm
    std::string errorInfo;      // 失败原因（成功时为空）
    YLT_REFL(S_CameraCalibMarkResult, ok, index, axisX, axisY, errorInfo);
};

/// 图2 旋转中心结果（CameraCalibCMD_RotCenterResult 的 JSON 数据）
struct S_CameraCalibRotCenterResult
{
    bool ok = false;            // 是否计算成功
    double centerX = 0.0;       // 旋转中心 x，单位 mm
    double centerY = 0.0;       // 旋转中心 y，单位 mm
    std::string errorInfo;      // 失败原因（成功时为空）
    YLT_REFL(S_CameraCalibRotCenterResult, ok, centerX, centerY, errorInfo);
};

#endif // __CAMERA_CALIB_PROTOCOL_H__
