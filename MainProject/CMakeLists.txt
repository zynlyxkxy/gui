# ============================================================
#  GUI_WM.exe —— 整机界面程序
#
#  target 名保持 GUI（.vscode\tasks.json 的 `--target GUI` 依赖它），产物名 GUI_WM。
#  CMAKE_DEBUG_POSTFIX "d" 只作用于库，Debug 下 exe 名仍是 GUI_WM.exe。
#
#  源码清单保持旧 gui/CMakeLists.txt 的原样（166 条，顺序不变），只是把
#  MainProject/ 前缀去掉了 —— 本目录就是 MainProject/。
# ============================================================

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)    # 各页面 .ui
set(CMAKE_AUTORCC ON)    # HelpModule/resources/markdown.qrc

set(GUI_SOURCES
    main.cpp
    MainWindow.cpp
    MainWindow.h
    MainWindow.ui
    # RPCModule
    RPCModule/MCTRLClientManager.h
    RPCModule/MCTRLClientManager.cpp
    RPCModule/UAClientManager.h
    RPCModule/UAClientManager.cpp
    # IOModule
    IOModule/IOPage.h
    IOModule/IOPage.cpp
    IOModule/IOGroup/IOGroup.h
    IOModule/IOGroup/IOGroup.cpp
    IOModule/IOWidget/AIWidget.h
    IOModule/IOWidget/AIWidget.cpp
    IOModule/IOWidget/AIWidget.ui
    IOModule/IOWidget/AOWidget.h
    IOModule/IOWidget/AOWidget.cpp
    IOModule/IOWidget/AOWidget.ui
    IOModule/IOWidget/DIWidget.h
    IOModule/IOWidget/DIWidget.cpp
    IOModule/IOWidget/DIWidget.ui
    IOModule/IOWidget/DOWidget.h
    IOModule/IOWidget/DOWidget.cpp
    IOModule/IOWidget/DOWidget.ui
    IOModule/IOWidget/BufferWidget.h
    IOModule/IOWidget/BufferWidget.cpp
    IOModule/IOWidget/BufferWidget.ui
    IOModule/IOWidget/SettingWidget.h
    IOModule/IOWidget/SettingWidget.cpp
    IOModule/IOWidget/SettingWidget.ui
    # MotionModule
    MotionModule/MotionPage.h
    MotionModule/MotionPage.cpp
    MotionModule/CommandsTab/CommandsTab.h
    MotionModule/CommandsTab/CommandsTab.cpp
    MotionModule/CommandsTab/CommandsTab.ui
    MotionModule/StageGroup/StageGroup.h
    MotionModule/StageGroup/StageGroup.cpp
    MotionModule/StageGroup/StageGroup.ui
    MotionModule/Common/StyleSheet.h
    # PlatformModule
    PlatformModule/Rm/RMPage.h
    PlatformModule/Rm/RMPage.cpp
    PlatformModule/Mc/MCPage.h
    PlatformModule/Mc/MCPage.cpp
    PlatformModule/Eh/EHPage.h
    PlatformModule/Eh/EHPage.cpp
    PlatformModule/Mt/MTPage.h
    PlatformModule/Mt/MTPage.cpp
    PlatformModule/St/STPage.h
    PlatformModule/St/STPage.cpp
    PlatformModule/Rp/RPPage.h
    PlatformModule/Rp/RPPage.cpp
    PlatformModule/Ua/UAPage.h
    PlatformModule/Ua/UAPage.cpp
    # HelpModule
    HelpModule/UpdatePage.h
    HelpModule/UpdatePage.cpp
    HelpModule/Document.h
    HelpModule/Document.cpp
    HelpModule/PreviewPage.h
    HelpModule/PreviewPage.cpp
    HelpModule/resources/markdown.qrc
    # 公共翻译绑定器（各页面各持一个实例）
    Common/UiTranslator.h
    Common/UiTranslator.cpp

    # 公共通用函数（多处共用的转换/工具函数）
    Common/CommonFunc.h
    Common/CommonFunc.cpp

    # 键合曲线采集器（订阅 IO 观察者，把 PLC AI 500/501/502 追加到曲线图）
    Common/BondCurveRecorder.h
    Common/BondCurveRecorder.cpp

    # MC + 处方数据管理单例
    MCRecipeModule/MCRecipeManager.h
    MCRecipeModule/MCRecipeManager.cpp

    # 机器状态管理单例
    StatusModule/StatusManager.h
    StatusModule/StatusManager.cpp

    # ProcessModule
    ProcessModule/processPage.h
    ProcessModule/processPage.cpp
    ProcessModule/processPage.ui
    ProcessModule/Components/LevelingDiagram.h
    ProcessModule/Components/LevelingDiagram.cpp
    Common/RecipeChooseWidget.h
    Common/RecipeChooseWidget.cpp
    ProcessModule/Components/GraphicsView/WaferGraphicsView.h
    ProcessModule/Components/GraphicsView/WaferGraphicsView.cpp
    ProcessModule/Components/GraphicsView/BondPointRectItem.h
    ProcessModule/Components/GraphicsView/BondPointRectItem.cpp
    ProcessModule/Components/GraphicsView/LevelingMarkItem.h
    ProcessModule/Components/GraphicsView/LevelingMarkItem.cpp
    ProcessModule/Components/TableComponent/TableComponent.h
    ProcessModule/Components/TableComponent/TableComponent.cpp
    ProcessModule/Components/DiagramComponent/SimpleDiagram.h
    ProcessModule/Components/DiagramComponent/SimpleDiagram.cpp
    ProcessModule/Components/DiagramComponent/Base/DiagramBase.h
    ProcessModule/Components/DiagramComponent/Base/DiagramBase.cpp
    ProcessModule/Components/DiagramComponent/BlueFilmChipDiagram.h
    ProcessModule/Components/DiagramComponent/BlueFilmChipDiagram.cpp
    ProcessModule/Components/DiagramComponent/DownWaferDiagram.h
    ProcessModule/Components/DiagramComponent/DownWaferDiagram.cpp
    ProcessModule/Components/DiagramComponent/ChipPosCalibDiagram.h
    ProcessModule/Components/DiagramComponent/ChipPosCalibDiagram.cpp
    ProcessModule/Base/ProcessBase.h
    ProcessModule/Base/ProcessBase.cpp
    ProcessModule/Protocol/ProcessManagerProtocol.h
    ProcessModule/Protocol/ChuckLevelProtocol.h
    ProcessModule/Protocol/ChipPosCalibProtocol.h
    ProcessModule/Protocol/WaferLevelCollectProtocol.h
    ProcessModule/Protocol/CoarseLevelProtocol.h
    ProcessModule/Protocol/DownLevelSelectProtocol.h
    ProcessModule/Protocol/FineLevelProtocol.h
    ProcessModule/Protocol/UpLevelSelectProtocol.h
    ProcessModule/Protocol/FineAlignProtocol.h
    ProcessModule/Protocol/AutoProcessProtocol.h
    ProcessModule/Protocol/AutoContinueBondProtocol.h
    ProcessModule/Protocol/BlueFilmChipHandoverProtocol.h
    ProcessModule/Protocol/BondProtocol.h
    ProcessModule/Process/ChipPosCalibProc.h
    ProcessModule/Process/ChipPosCalibProc.cpp
    ProcessModule/Process/ChuckLevelProc.h
    ProcessModule/Process/ChuckLevelProc.cpp
    ProcessModule/Process/CoarseLevelProc.h
    ProcessModule/Process/CoarseLevelProc.cpp
    ProcessModule/Process/DownLevelSelectProc.h
    ProcessModule/Process/DownLevelSelectProc.cpp
    ProcessModule/Process/FineLevelProc.h
    ProcessModule/Process/FineLevelProc.cpp
    ProcessModule/Process/FineAlignProc.h
    ProcessModule/Process/FineAlignProc.cpp
    ProcessModule/Process/UpLevelSelectProc.h
    ProcessModule/Process/UpLevelSelectProc.cpp
    ProcessModule/Process/WaferLevelCollectProc.h
    ProcessModule/Process/WaferLevelCollectProc.cpp
    ProcessModule/Process/BondProc.h
    ProcessModule/Process/BondProc.cpp
    ProcessModule/Process/BlueFilmChipHandoverProc.h
    ProcessModule/Process/BlueFilmChipHandoverProc.cpp

    InfoModule/Status/SystemStatusPage.h
    InfoModule/Status/SystemStatusPage.cpp
    InfoModule/Status/SystemStatusPage.ui

    InfoModule/License/ConfigPage.h
    InfoModule/License/ConfigPage.cpp
    InfoModule/License/ConfigPage.ui
    InfoModule/License/LicenseWatermark.h
    InfoModule/License/LicenseWatermark.cpp

    AutoModule/ContinuousBondPage.h
    AutoModule/ContinuousBondPage.cpp
    AutoModule/ContinuousBondPage.ui
    AutoModule/AutoProcessPage.h
    AutoModule/AutoProcessPage.cpp
    AutoModule/AutoProcessPage.ui

    MaterialModule/MaterialPage.h
    MaterialModule/MaterialPage.cpp
    MaterialModule/MaterialPage.ui
    MaterialModule/Tabs/MaterialTabBase.h
    MaterialModule/Tabs/MaterialTabBase.cpp
    MaterialModule/Tabs/DownWaferLoadUnload.h
    MaterialModule/Tabs/DownWaferLoadUnload.cpp
    MaterialModule/Tabs/BlueFilmLoadUnload.h
    MaterialModule/Tabs/BlueFilmLoadUnload.cpp

    # 相机标定独立页面（流程 SingleProc_CameraCalib 无 ProcessPage tab，由本页触发）
    CameraCalibModule/CameraCalibPage.h
    CameraCalibModule/CameraCalibPage.cpp
    CameraCalibModule/CameraCalibPage.ui
)

istar_add_app(GUI WIN32 SOURCES ${GUI_SOURCES})

# Process/ 下各 Proc.cpp 通过 #include "ui_ProcessPage.h" 访问整页控件，
# 但 ProcessPage.ui 在上层目录，需让 AUTOUIC 能按名定位到它（缺了编译失败）
set_property(TARGET GUI APPEND PROPERTY AUTOUIC_SEARCH_PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/ProcessModule")

# MaterialModule/Tabs/ 下各 tab.cpp 同理：通过 #include "ui_MaterialPage.h" 访问整页控件，
# 而 MaterialPage.ui 在上层目录
set_property(TARGET GUI APPEND PROPERTY AUTOUIC_SEARCH_PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/MaterialModule")

target_include_directories(GUI PRIVATE
    ${PLATFORM_INCLUDE_DIRS}
    ${COMM_INCLUDE_DIRS}
    "${GEN_RPC_DIR}/include"        # 生成的 mctrl_client/_server/_define.h（不再回写源码树）
    "${RPC_ROOT}/include"           # 手写的 mctrl_controller.h / mctrlc_data.h
    "${CMAKE_CURRENT_SOURCE_DIR}"   # 源码写 #include "MCRecipeModule/ReData/S_Recipe.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/MotionModule"
    "${CMAKE_CURRENT_SOURCE_DIR}/IOModule"
)

target_link_directories(GUI PRIVATE
    ${BASIC_LIB}
)
foreach(MOD ${COMM_MODULES})
    target_link_directories(GUI PRIVATE "${COMM_ROOT}/${MOD}/lib")
endforeach()

# 平台基础库（Debug 链接 d 后缀版本）
foreach(LIB ${PLATFORM_LINK_LIBS})
    target_link_libraries(GUI PRIVATE
        $<$<CONFIG:Debug>:${LIB}d>
        $<$<NOT:$<CONFIG:Debug>>:${LIB}>
    )
endforeach()

target_link_libraries(GUI PRIVATE
    rpc_mctrl                     # 本工程生成的 mctrl 协议库
    $<$<CONFIG:Debug>:uad>        # ua 协议：Debug 链 uad.lib
    $<$<NOT:$<CONFIG:Debug>>:ua>  # Release 链 ua.lib
    Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network
    Qt5::WebEngineWidgets Qt5::WebChannel
)

# QCustomPlot（含 QCPAxis 等）由 ISTARGUIComponent.dll 导出：qcustomplot.h 里 QCP_LIB_DECL
# 默认是空的，不声明为「使用该库」时编译器会把 QCustomPlot 当成本地类，直接引用它的符号
# （如 staticMetaObject）会 LNK2001。统一在此声明，各 .h 里不再各自 #define。
target_compile_definitions(GUI PRIVATE QCUSTOMPLOT_USE_LIBRARY)

# 界面库 debug/release 加载区分：Debug 构建定义 GUI_DEBUG（按 CMake 的 CONFIG 判定，
# **不能**改成 _DEBUG 或 QT_DEBUG —— 源码 8 处按这个约定在用）
target_compile_definitions(GUI PRIVATE $<$<CONFIG:Debug>:GUI_DEBUG>)

set_target_properties(GUI PROPERTIES
    OUTPUT_NAME "GUI_WM"
    WIN32_EXECUTABLE TRUE
)

# ============================================================
#  build/<Config>/ 自包含
#  平台基础库（含 Qt5 + platforms + styles）由 istar_runtime_shared 整包拷一次；
#  这里补：本工程的协议库、平台模块 dll / 界面库、WebEngine、re 的界面库。
# ============================================================
istar_deploy_runtime(GUI
    TARGETS rpc_mctrl
    DLLS ${COMM_RUNTIME_DLLS}
)

# WebEngine 19 项：9 个 Qt5*.dll + libEGL/libGLESv2 + d3dcompiler_47.dll
# + QtWebEngineProcess.exe + 5 个资源 + qtwebengine_locales/ 整目录
istar_deploy_webengine(GUI)

# re 界面库（release 与 debug 两份都拷，不看当前 CONFIG）。
# 来源是 re 的 output/interface/gui/lib/<cfg>/ —— re 那个目录就是"给下游的交付包"
# （re 的 output/exe/ 是给顶层装配的另一份）。
# istar_deploy_first 取"第一个存在的候选"，所以每个 lib 单独调一次：
# release 有 exe/lib/module 兜底，debug 没有。
foreach(UIRE_LIB libUIRE_CHIP libUIRE_Wafer)
    istar_deploy_first(GUI CANDIDATES
        "${RE_OUT_DIR}/interface/gui/lib/release/${UIRE_LIB}.dll"
        "${EXE_MODLIB_DIR}/${UIRE_LIB}.dll")
    istar_deploy_first(GUI CANDIDATES
        "${RE_OUT_DIR}/interface/gui/lib/debug/${UIRE_LIB}d.dll")
endforeach()

# ============================================================
#  配置：本工程默认 config → exe 旁；外层 1927project/config 覆盖（后注册的赢）
#  旧的 OUTER_CONFIG_DIR 写成 ../../config（gui 只有 2 层）→ 一直是死代码；
#  共享模块改为向上探测工程根，所以覆盖现在真正生效。
#  build\<Config>\ 就是单机调试时的配置根（cwd），形状与部署后的 exe\config\ 一致。
# ============================================================
istar_stage(GUI SRC "${CMAKE_SOURCE_DIR}/config/appconfig.yaml")
istar_stage(GUI SRC "${CMAKE_SOURCE_DIR}/config/mw_config.yaml")
# 整机多语言词条：进程按 <配置根>/gui_config/Languages 显式加载（见 MainWindow.cpp），
# 不再依赖 cwd。MachineConstantName.csv / RecipeName.csv 分别是 MC 常量名与处方名字典，
# GUI 通过 UIMC_SetMcDictPath / UIRm_SetMcDictPath 指过去。
istar_stage(GUI SRC "${CMAKE_SOURCE_DIR}/config/Languages" TO gui_config/Languages)
if(OUTER_CONFIG_DIR)
    istar_stage(GUI SRC "${OUTER_CONFIG_DIR}/mw_config.yaml" OPTIONAL)
    istar_stage(GUI SRC "${OUTER_CONFIG_DIR}/appconfig.yaml" OPTIONAL)
    istar_stage(GUI SRC "${OUTER_CONFIG_DIR}/gui_config" OPTIONAL)
endif()

# ============================================================
#  output/exe/ 部署形状 —— **只有 Release**
#    GUI_WM.exe + 本工程构建的 mctrl.dll + qt.conf（istar_install_app QT 生成）
#    + WebEngine 里必须与 exe 同级的部分
#  不进 output/exe/：平台基础库、平台模块 dll、libUIRE_*、libUIRm、9 个 Qt5WebEngine*.dll
#  （它们属于顶层运行层：exe\platform\ 镜像或 exe\shared\qt，程序经 PATH 加载）。
#
#  ★ **output/exe 必须原样等于 exe\apps\gui**：顶层只整目录拷贝、不做任何增删。
#    因此凡是不由本工程构建、又能从共享层解析到的东西都不放进来：
#    - libUIRm.dll：全仓只有 GUI_WM.exe 用它（实测二进制扫描），但它是平台
#      rpc_output 的产物 —— 顶层 exe\platform\rpc_output\libUIRM\lib\ 里有，
#      GUI_WM 经 run.bat 的 PATH 加载，这里不再单独发一份。
#    - 9 个 Qt5WebEngine*.dll 模块：本机 Qt 件，归顶层 exe\shared\qt\（PATH）。
#
#  ★ **一个配置文件都不放**：appconfig.yaml / mw_config.yaml 都不进 output/exe/。
#    这个文件夹的含义是"exe 运行时需要的文件"，不是"一个能在原地拉起来的目录"。
#    运行时配置根是 exe\config\（PLATFORM_PATH），配置由顶层 config\ 出
#    （deploy_ops :config）：appconfig.yaml / mw_config.yaml 在根上，
#    gui_config\Languages\ 是模块目录。
#    本工程 config\ 的那几条 istar_stage 只服务 build\<Config>\ 的单独调试
#    （铺出来的就是一棵配置根），不进部署形状。
#  ★ Languages\ 不进 output/exe：MainWindow 现在显式传绝对目录
#    <配置根>/gui_config/Languages 给 ISTARLanguage（它是不依赖 Qt 的纯 CRT 实现，
#    相对路径会随进程 cwd 解析 —— 我们不依赖 cwd），词条由顶层 config/gui_config/
#    镜像到 exe\config\ 后照常加载。MC/RM 的两个 Name 词典用
#    UIMC_SetMcDictPath / UIRm_SetMcDictPath 显式指到同一目录。
# ============================================================
istar_install_app(GUI APP gui
    DIR exe
    TARGETS rpc_mctrl
    QT)

install(FILES
        "${QT5_ROOT}/bin/QtWebEngineProcess.exe"
        "${QT5_ROOT}/bin/libEGL.dll"
        "${QT5_ROOT}/bin/libGLESv2.dll"
        "${QT5_ROOT}/bin/d3dcompiler_47.dll"     # 不带配置后缀
        "${QT5_ROOT}/resources/icudtl.dat"
        "${QT5_ROOT}/resources/qtwebengine_resources.pak"
        "${QT5_ROOT}/resources/qtwebengine_resources_100p.pak"
        "${QT5_ROOT}/resources/qtwebengine_resources_200p.pak"
        "${QT5_ROOT}/resources/qtwebengine_devtools_resources.pak"
    DESTINATION exe CONFIGURATIONS Release)
install(DIRECTORY "${QT5_ROOT}/translations/qtwebengine_locales/"
    DESTINATION exe/qtwebengine_locales CONFIGURATIONS Release)

# 注意：Languages\ / appconfig.yaml / mw_config.yaml 都**不进** output\exe
# （Languages 与前一个 yaml 由顶层 config\ 出到 exe\config\，程序按 PLATFORM_PATH 读），
# 见上面注释。
