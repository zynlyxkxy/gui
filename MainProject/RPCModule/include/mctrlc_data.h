/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* Mctrl — GUI↔主控层 RPC 数据结构（聚合视图）
* 结构体与子模块协议（motion / io / rm）完全同名同构，命名空间 MctrlModule 隔离。
* 子模块协议更新时，直接复制对应结构体即可，无需再加前缀。
*********************************************************************************/
#ifndef __MCTRLC_DATA_H__
#define __MCTRLC_DATA_H__

#include <string>
#include <vector>

namespace MctrlModule
{

// ==================== 流程（GUI ↔ ProcessModule） ====================

// 流程消息：GUI 下发命令与流程上报共用同一载荷
struct ProcessMessage {
    int process = 0;               // 流程 ID（ProcessID 枚举值，见 ProcessManagerProtocol.h；GUI 请求=目标流程；流程上报=来源流程）
    int command = 0;               // 命令值（0-99 流程私有；100+ 基类公共；manager 协议见 ProcessManagerProtocol.h）
    std::string json;              // 载荷 JSON（S_ChipPosCalibMarkData 等）
};

// ==================== 运动台（对齐 MotionModule） ====================

// ===== Stage 信息 =====
struct StageInfo {
    std::string stageType;              // "acs" 或 "plc"
    std::string stageName;              // 用户指定名，如 "coarse"、"vertical"
    std::vector<std::string> axisNames; // 轴名列表，决定 get_Pos / MoveAbs 等方法的参数顺序
};

// ===== 位置/速度/加速度等数值包装 =====
struct StageVecDouble {
    std::string stageName;
    std::vector<double> data;
};

// ===== Observer 周期推送（单台数据） =====
struct StageObserverData {
    std::string stageName;              // Stage 名称
    std::vector<double> pos;            // 所有轴位置，顺序 = axisNames()
    std::vector<double> vel;            // 所有轴速度
    std::vector<double> acc;            // 所有轴加速度
    bool enable;                        // 使能状态（所有轴 AND）
    bool moving;                        // 运动状态（任一轴运动即为 true）
    bool inPos;                         // 到位状态（所有轴 AND）
};

// ===== Observer 周期推送列表（一次推所有 stage） =====
struct StageObserverDataList {
    std::vector<StageObserverData> items;
};

// ===== 单台 motion server 信息（对齐 MotionModule::ServerInfo） =====
struct ServerInfo {
    std::string systemName;
    std::vector<StageInfo> stages;      // 当前 server 下的所有 stage
};

// ===== 多台 motion server 聚合列表（mctrl 独有，一次推全部 motion* 节点） =====
struct MotionServerInfoList {
    std::vector<ServerInfo> items;
};

// ==================== IO（对齐 IOModule） ====================

// ===== AO write input =====
struct IoAoInput {
    int varId;
    double value;            // target value to write
    bool verify;
    double expectedValue;    // expected observer value when verify=true
};

// ===== DO write input =====
struct IoDoInput {
    int varId;
    bool value;
    bool verify;
    bool expectedValue;      // expected observer value when verify=true
};

// ===== SettingDouble write input =====
struct IoSettingDoubleInput {
    int varId;
    double value;
};

// ===== SettingBool write input =====
struct IoSettingBoolInput {
    int varId;
    bool value;
};

// ===== Parameterless operation (shared by StartBuffer / StopBuffer) =====
struct IoVarInput {
    int varId;
};

// ===== Observer entries =====
struct ObserverDoubleEntry {
    int varId;
    double value;
};

struct ObserverBoolEntry {
    int varId;
    bool value;
};

// ===== ACS Observer periodic push (500ms) =====
struct AcsObserverData {
    std::vector<ObserverDoubleEntry> AI;
    std::vector<ObserverBoolEntry>   DI;
    std::vector<ObserverDoubleEntry> AO;
    std::vector<ObserverBoolEntry>   DO;
    std::vector<ObserverBoolEntry>   Buf;
};

// ===== PLC Observer periodic push (500ms) =====
struct PlcObserverData {
    std::vector<ObserverDoubleEntry> AI;
    std::vector<ObserverBoolEntry>   DI;
    std::vector<ObserverDoubleEntry> AO;
    std::vector<ObserverBoolEntry>   DO;
};

// ===== IO Observer 周期推送（一次推 ACS + PLC 全部缓存） =====
struct IoObserverData {
    AcsObserverData acs;
    PlcObserverData plc;
};

// ===== IO ServerInfo 辅助结构 =====
struct IoVarInfo {
    int id;
    std::string name;
};

struct IoAoVarInfo {
    int id;
    std::string name;
    std::string observer;            // bound observer name
};

struct IoDoVarInfo {
    int id;
    std::string name;
    std::string observer;
};

struct IoArgInfo {
    std::string name;               // argument name (from PLC command template)
};

struct IoCmdVarInfo {
    int id;
    std::string name;
    std::vector<IoArgInfo> args;    // command arguments; empty for 0-param commands
};

struct IoInfo {
    std::vector<IoVarInfo> AI;
    std::vector<IoVarInfo> DI;
    std::vector<IoAoVarInfo> AO;
    std::vector<IoDoVarInfo> DO;
    std::vector<IoVarInfo> SettingDouble;
    std::vector<IoVarInfo> SettingBool;
    std::vector<IoCmdVarInfo> Cmd;
    std::vector<IoVarInfo> Buffer;
};

// ===== IO server 信息（对齐 IOModule::IoServerInfo） =====
struct IoServerInfo {
    IoInfo io;                        // 当前 server 下的 IO 变量
};

// ==================== 配方（对齐 rm 协议） ====================

// ===== 字符串列表 =====
struct StringList {
    std::vector<std::string> items;  // 元素列表
};

// 处方引用
struct RcpReference
{
    std::string name;                             // 引用处方名称
};

// 处方头
struct RcpHeader
{
    // 基础信息
    std::string uid;                            // 唯一标识
    std::string name;                           // 名称
    long long createDate;                       // 创建日期
    std::string author;                         // 作者
    std::vector<RcpReference> references;       // 引用列表
    std::string description;                    // 描述

    // 版本信息
    std::string version;                        // 版本
    bool released;                              // 是否已发布

    // 修改信息
    std::string modifier;                       // 修改人员
    long long modifyTime;                       // 修改时间

    // 发布信息
    std::string releaser;                       // 发布人员
    long long releaseTime;                      // 发布时间
};

struct Rcp
{
    std::string checksum;                       // 校验和
    RcpHeader header;                           // 处方头
    std::string body;                           // 处方体内容
};

// 处方列表
struct RcpList
{
    std::vector<Rcp> items;                     // 元素列表
};

// ===== wafer + chip 处方组合（mctrl 独有，GetCurRecipeJson 返回） =====
struct RcpInfoList {
    RcpList waferInfo;
    RcpList chipInfo;
};

// 机器常数基本信息
struct MCBasicInfo
{
    // 基础信息
    std::string uid;                            // 唯一标识
    std::string constantName;                   // 常数名称
    std::string version;                        // 版本
    std::string description;                    // 描述
    bool executed;                              // 是否已激活（只有一份是激活的）

    // 创建信息
    std::string creator;                        // 创建人员
    std::string createdTime;                    // 创建时间

    // 修改信息
    std::string modifier;                       // 修改人员
    std::string modifiedTime;                   // 修改时间

    // 激活信息（执行信息）
    std::string executer;                       // 执行人员（激活人员）
    std::string executedTime;                   // 执行时间（激活时间）
    std::string sourceName;                     // 激活来源（激活前的源常数名称，未激活常数为空）
};

// 带完整头信息的机器常数
struct MCConstant
{
    std::string checksum;                       // 校验和
    MCBasicInfo basicInfo;                      // 基本信息
    std::string body;                           // 常数体内容
};

// ==================== 系统状态/模块信息（遥测，独立于业务流程） ====================
// 本组信息"结构向后生长"：新增信息只加字段，不开新接口。
// 注:同命名空间 MctrlModule 内的无作用域枚举，成员统一"全大写 + 枚举名作前缀"防冲突
//    (如 MCTRL_STATUS_UNKNOWN / MODULE_ALIVE_STATE_UNKNOWN)

// ===== 主控整机主状态机（MCTRL_STATUS_ALARM 最高优先级：存在未处理报警即报 ALARM） =====
// 注：MCTRL_STATUS_DISCONNECTED 仅由 GUI 侧 StatusManager 看门狗产生，
//     控制层不会推送此值；收到推送即自然回到真实状态
enum MctrlStatus
{
    MCTRL_STATUS_UNKNOWN = 0,       // 未知/未就绪
    MCTRL_STATUS_LOADING,           // 加载处方/数据中
    MCTRL_STATUS_LOAD_FAIL,         // 加载失败（启动阶段任一初始化失败后常驻此状态，只能人工关闭重启）
    MCTRL_STATUS_LOAD_OK,           // 加载完成
    MCTRL_STATUS_INITIALIZING,      // 初始化中
    MCTRL_STATUS_INI_FAIL,          // 初始化失败
    MCTRL_STATUS_IDLE,              // 空闲待命
    MCTRL_STATUS_BUSY,              // 运行中
    MCTRL_STATUS_ALARM,             // 报警态（最高优先级）
    MCTRL_STATUS_DISCONNECTED,      // 链路断（GUI 看门狗超时，控制层不发此值）
};

// ===== 子模块存活状态（三值：无证据 / 有证据 / 证据过期） =====
enum ModuleAliveState
{
    MODULE_ALIVE_STATE_UNKNOWN  = 0,   // 未收到任何证据（探测中/无数据）
    MODULE_ALIVE_STATE_ONLINE   = 1,   // 最近见过证据
    MODULE_ALIVE_STATE_OFFLINE  = 2,   // 超过判活阈值未见证据（可能断连/宕机/失联）
};

// ===== 单模块存活状态 =====
struct ModuleStateInfo {
    std::string name;               // 模块名(mc/eh/io/rp/motionN/rm/rm2/vision/cf…)
    ModuleAliveState state = MODULE_ALIVE_STATE_UNKNOWN;
};

// ===== 系统信息快照（周期推送与按需拉取共用同一结构） =====
struct SystemInfo {
    MctrlStatus mctrlStatus = MCTRL_STATUS_UNKNOWN;    // 主控整机状态
    std::vector<ModuleStateInfo> modules;              // 子模块存活状态
};

} // namespace MctrlModule

#endif // __MCTRLC_DATA_H__
