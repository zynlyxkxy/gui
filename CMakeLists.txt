cmake_minimum_required(VERSION 3.26)
project(GUI VERSION 1.0.0.0 LANGUAGES CXX)

# ============================================================
#  gui —— 整机界面程序（target 名 GUI，产物 GUI_WM.exe）
#
#  与 io / motion 不同的地方：
#   1) 需要 Qt WebEngine：平台基础库自带的那份 Qt 里没有 WebEngine，必须从
#      Qt 安装目录补 19 项运行时 —— 见 istar_deploy_webengine()。
#      注意 d3dcompiler_47.dll **不带**配置后缀，libEGL/libGLESv2 **带**。
#   2) 平台模块库来自本工程的 platform_output 子模块（通信库 mc/eh/mt/rm/rp/ua
#      + 界面库 libUI*）：只进 build/<Config>/，**不进** output/。
#   3) gui 是链路终点：re 的 libUIRE_*.dll 由上游同步/构建产生，mctrl 的协议头
#      由 mctrl 同步进来，gui 不再向下游输出任何东西。
#   4) 外层 1927project/config 覆盖在旧写法里是**死代码**（旧模板按 3 层算成
#      ../../config，gui 只有 2 层 → 解析到不存在的 1927/config）。共享模块改为
#      向上探测工程根，所以迁移后外层覆盖**真正生效**（见下面的 istar_stage / install）。
#
#  目录约定（两种配置不同）：
#    build/<Config>/   调试用，自包含（Qt + platforms + WebEngine + ISTAR + libUI* + config）
#    output/           部署形状，**只有 Release**（exe + mctrl.dll + qt.conf + WebEngine
#                      里必须与 exe 同级的部分 + 配置），由顶层装配到 exe/apps/gui/
# ============================================================

# ---- 共享构建模块在本工程内（gui/cmake/），独立构建不依赖任何外层工程 ----
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(istar_common)    # BASIC_LIB / BASIC_INC / BASIC_CONFIG / ISTAR_GENERATOR
                         # OUTER_CONFIG_DIR / ISTAR_OUTPUT_DIR / istar_runtime_shared / istar_add_app()
include(istar_runtime)   # istar_deploy_runtime() / istar_deploy_first()
                         # istar_deploy_webengine() / istar_stage()
include(istar_install)   # istar_install_app() / istar_finalize_deploy()
include(istar_rpc)       # istar_gen_rpc()

if(NOT DEFINED BASIC_LIB OR NOT DEFINED BASIC_INC)
    message(FATAL_ERROR "istar_common.cmake 未生效：请检查 CMAKE_MODULE_PATH")
endif()

# ============================================================
#  平台模块库（platform_output 子模块）+ RPC 协议
# ============================================================
set(COMM_ROOT    "${CMAKE_SOURCE_DIR}/platform_output")
set(COMM_MODULES mc eh mt rm rp ua)
set(COMM_UI_DIRS libUIMC libUIEH libUIMT libUIRM libUIST libUIRP libUIUA)

set(RPC_ROOT    "${CMAKE_SOURCE_DIR}/MainProject/RPCModule")
set(GEN_RPC_DIR "${CMAKE_BINARY_DIR}/gen/mctrl")

# Qt5: WebEngineWidgets / WebChannel 是 gui 独有的依赖
find_package(Qt5 REQUIRED COMPONENTS
    Core Gui Widgets Network WebEngineWidgets WebChannel)

# Qt5_DIR = <prefix>/lib/cmake/Qt5 → <prefix>，WebEngine 运行时的来源目录
get_filename_component(QT5_ROOT "${Qt5_DIR}/../../.." ABSOLUTE)

# ============================================================
#  平台基础库头文件目录
# ============================================================
set(PLATFORM_INCLUDE_DIRS
    "${BASIC_INC}/common/include"
    "${BASIC_INC}/istarappconfig/include"
    "${BASIC_INC}/istarbasicvalue/include"
    "${BASIC_INC}/istarjson/include"
    "${BASIC_INC}/istarconverter/include"
    "${BASIC_INC}/istarcsv/include"
    "${BASIC_INC}/istardevicerecognize/include"
    "${BASIC_INC}/istarencryption/include"
    "${BASIC_INC}/istarfilesystem/include"
    "${BASIC_INC}/istargf/include"
    "${BASIC_INC}/istarguiappconfig/include"
    "${BASIC_INC}/istarguicomponent/include"
    "${BASIC_INC}/istarguiframework/include"
    "${BASIC_INC}/istarguischema/include"
    "${BASIC_INC}/istarlanguage/include"
    "${BASIC_INC}/istarlog/include"
    "${BASIC_INC}/istarmarkdown/include"
    "${BASIC_INC}/istarmqtt/include"
    "${BASIC_INC}/istarmw/include"
    "${BASIC_INC}/istarschema/include"
    "${BASIC_INC}/istarserializer/include"
    "${BASIC_INC}/istarsm/include"
    "${BASIC_INC}/istarthreadpool/include"
    "${BASIC_INC}/istaruistyle/include"
    "${BASIC_INC}/istarxml/include"
    "${BASIC_INC}/istaryaml/include"
    "${BASIC_INC}/istarzmqworker/include"
    "${BASIC_INC}/istarlicense/include"
)

# ============================================================
#  平台基础库链接列表（Debug 用 *d.lib；平台基础库 dll 由 istar_runtime_shared 整包拷）
# ============================================================
set(PLATFORM_LINK_LIBS
    ISTARAppConfig
    ISTARBasicValue
    ISTARConverter
    ISTARCsv
    ISTARDeviceRecognize
    ISTAREncryption
    ISTARFileSystem
    ISTARGF
    ISTARGUIAppConfig
    ISTARGUIComponent
    ISTARGUIFramework
    ISTARGUISchema
    ISTARLanguage
    ISTARLog
    ISTARMarkdown
    ISTARMqtt
    ISTARMW
    ISTARSM
    ISTARSchema
    ISTARSerializer
    ISTARThreadPool
    ISTARUIStyle
    ISTARXml
    ISTARYaml
    ISTARZmqWorker
    ISTARLicense
)

# rpc_mctrl 协议库依赖的平台基础库（Debug/Release 成对）
set(RPC_LINK_LIBS
    debug ISTARLogd           optimized ISTARLog
    debug ISTARYamld          optimized ISTARYaml
    debug ISTARMWd            optimized ISTARMW
    debug ISTARGFd            optimized ISTARGF
    debug ISTARThreadPoold    optimized ISTARThreadPool
    debug ISTARCsvd           optimized ISTARCsv
    debug ISTAREncryptiond    optimized ISTAREncryption
    debug ISTARFileSystemd    optimized ISTARFileSystem
    debug ISTARSerializerd    optimized ISTARSerializer
    debug ISTARSMd            optimized ISTARSM
    debug ISTARZmqWorkerd     optimized ISTARZmqWorker
)

# ============================================================
#  平台模块库（通信 + 界面库）头文件目录
# ============================================================
set(COMM_INCLUDE_DIRS)
foreach(MOD ${COMM_MODULES})
    list(APPEND COMM_INCLUDE_DIRS "${COMM_ROOT}/${MOD}/include")
endforeach()
foreach(DIR ${COMM_UI_DIRS})
    list(APPEND COMM_INCLUDE_DIRS "${COMM_ROOT}/${DIR}/include")
endforeach()

# ============================================================
#  平台模块 dll：只进 build/<Config>/（调试自包含），不进 output/
#
#  release / debug 两份都拷，与 istar_runtime_shared 整包拷平台基础库的做法一致
#  （那份目录里 ISTAR*.dll 与 ISTAR*d.dll 也是并存的）。
#  目录名与产物名不完全一致：libUIRM 目录（大写 RM）→ libUIRm.dll（小写 m），
#  循环拼不出来，必须单独写这一条。
# ============================================================
set(COMM_RUNTIME_DLLS)
foreach(MOD ${COMM_MODULES})
    list(APPEND COMM_RUNTIME_DLLS
        "${COMM_ROOT}/${MOD}/lib/${MOD}.dll"
        "${COMM_ROOT}/${MOD}/lib/${MOD}d.dll")
endforeach()
set(COMM_UI_LIB_STEMS
    "${COMM_ROOT}/libUIMC/lib/libUIMC"
    "${COMM_ROOT}/libUIEH/lib/libUIEH"
    "${COMM_ROOT}/libUIMT/lib/libUIMT"
    "${COMM_ROOT}/libUIRM/lib/libUIRm"     # ← 特例：目录大写 RM、产物小写 m
    "${COMM_ROOT}/libUIST/lib/libUIST"
    "${COMM_ROOT}/libUIRP/lib/libUIRP"
    "${COMM_ROOT}/libUIUA/lib/libUIUA"
)
foreach(_stem IN LISTS COMM_UI_LIB_STEMS)
    list(APPEND COMM_RUNTIME_DLLS "${_stem}.dll" "${_stem}d.dll")
endforeach()

# ============================================================
#  re 界面库（libUIRE_CHIP / libUIRE_Wafer）的读取路径
#    release 版：module/re/output/interface/gui/lib/release/X.dll
#                缺失时退回 exe/lib/module/X.dll
#    debug 版：  module/re/output/interface/gui/lib/debug/Xd.dll（**没有**兜底）
#  这份 interface/ 就是 re 的"给下游的交付包"（另一份 output/exe/ 给顶层装配）。
#  两份都拷、不看当前 CONFIG；re 未构建/未部署时跳过（不是错误）
# ============================================================
set(RE_OUT_DIR     "${CMAKE_SOURCE_DIR}/../module/re/output")
set(EXE_MODLIB_DIR "${CMAKE_SOURCE_DIR}/../exe/lib/module")

add_subdirectory(MainProject/RPCModule)   # rpc_mctrl —— mctrl 协议库（构建时生成）
add_subdirectory(MainProject)             # GUI_WM.exe

# 把 install 挂进默认构建 —— Qt Creator / VSCode / 命令行"构建一下"都会产出 output/
istar_finalize_deploy()
