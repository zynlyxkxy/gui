/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* Mctrl — GUI↔主控层 RPC 接口（聚合视图）
* 接口函数名对齐子模块协议（Motion* / IO*），命名空间 MctrlModule。
* 配方部分为 mctrl 自身状态接口（验证并持有当前处方），保留语义命名。
*********************************************************************************/
#ifndef __MCTRL_CONTROLLER_H__
#define __MCTRL_CONTROLLER_H__

#include "istar_platform_basicdata.h"
#include "mctrlc_data.h"

#ifndef request
#define request
#define subscribe
#define publish
#endif

namespace MctrlModule
{

class UIMController
{
public:
    // ==================== 流程（GUI ↔ ProcessModule） ====================
    // GUI 下发命令给流程
    request virtual ISTARResultStatus GuiToMctrlCommand(IN ProcessMessage msg) = 0;

    // 流程上报命令给 GUI
    subscribe virtual ISTARResultStatus MctrlToGuiCommand(OUT ProcessMessage &msg) = 0;

    // ==================== 系统信息（GUI ↔ ProcessModule） ====================
    // 周期推送：系统/模块状态（替代 ProcessMessage+自定义 command 的 hack；与 MotionObserverData 同型）
    subscribe virtual ISTARResultStatus MctrlToGuiSystemInfo(OUT SystemInfo &info) = 0;


    // ==================== 运动台（对齐 UIMotionController；GetServerInfo 为多台聚合） ====================
    request virtual ISTARResultStatus MotionGetServerInfo(OUT MotionServerInfoList &result) = 0;

    request virtual ISTARResultStatus MotionCmdEnable(IN std::string stageName) = 0;

    request virtual ISTARResultStatus MotionCmdDisable(IN std::string stageName) = 0;

    request virtual ISTARResultStatus MotionCmdHome(IN std::string stageName) = 0;

    // 绝对移动，positions.data 顺序 = axisNames()
    request virtual ISTARResultStatus MotionCmdMoveAbs(IN StageVecDouble positions) = 0;

    // 相对移动，deltas.data 顺序 = axisNames()
    request virtual ISTARResultStatus MotionCmdMoveRel(IN StageVecDouble deltas) = 0;

    request virtual ISTARResultStatus MotionCmdStop(IN std::string stageName) = 0;

    // Observer 周期推送（一次推全部 stage，不区分 ACS/PLC）
    subscribe virtual ISTARResultStatus MotionObserverData(OUT StageObserverDataList &data) = 0;

    // ==================== IO（对齐 UIIoController） ====================
    request virtual ISTARResultStatus IOGetServerInfo(OUT IoServerInfo &result) = 0;

    request virtual ISTARResultStatus IOSetAo(IN IoAoInput input) = 0;

    request virtual ISTARResultStatus IOSetDo(IN IoDoInput input) = 0;

    request virtual ISTARResultStatus IOStartBuffer(IN IoVarInput input) = 0;

    request virtual ISTARResultStatus IOStopBuffer(IN IoVarInput input) = 0;

    request virtual ISTARResultStatus IOSetSettingDouble(IN IoSettingDoubleInput input) = 0;

    request virtual ISTARResultStatus IOSetSettingBool(IN IoSettingBoolInput input) = 0;

    // Observer 周期推送（一次推 ACS + PLC 全部缓存）
    subscribe virtual ISTARResultStatus IOObserverData(OUT IoObserverData &data) = 0;

    // ==================== 配方（mctrl 当前处方状态） ====================
    request virtual ISTARResultStatus GetWaferReleasedNameList(OUT StringList& items) = 0;

    request virtual ISTARResultStatus SetWaferReleasedName(IN std::string& name) = 0;

    request virtual ISTARResultStatus GetCurRecipeInfo(OUT RcpInfoList& result) = 0;

    // ==================== 机器常数（mctrl 当前机器常数状态） ====================
    request virtual ISTARResultStatus GetCurMCInfo(OUT MCConstant& result) = 0;
};

} // namespace MctrlModule

#endif // __MCTRL_CONTROLLER_H__
