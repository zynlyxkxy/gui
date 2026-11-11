# ============================================================
#  rpc_mctrl —— 主控层(mctrl)协议库，GUI 只与主控通信
#
#  生成物只落 build/gen/mctrl/，**不回写源码树**（旧做法把 3 个生成头拷回
#  MainProject/RPCModule/include 再用"删掉它们"驱动重生成）。重新生成的时机
#  完全交给 CMake 的 OUTPUT/DEPENDS。
#
#  手写协议头（入库）：MainProject/RPCModule/include/mctrl_controller.h、mctrlc_data.h
#  生成头（build 树）：build/gen/mctrl/include/{mctrl_client,mctrl_server,mctrl_define}.h
#
#  **不打** mctrl 的 rpc_patch_mctrl.cmake 补丁：那行 `using namespace ::MctrlModule;`
#  正是 gui 依赖的（MainWindow.h / ProcessBase.h 等直接写 MctrlStatus 这类非限定名）。
#  只有 mctrl 业务代码因为同时 include 主控与子模块协议才会撞 C2872 二义性，
#  所以这个补丁按工程选择，只对 mctrl 生效。
# ============================================================

istar_gen_rpc(mctrl
    API_DIR "${RPC_ROOT}/include"
    GEN_DIR "${GEN_RPC_DIR}")

add_library(rpc_mctrl SHARED ${ISTAR_RPC_mctrl_SOURCES})

set_target_properties(rpc_mctrl PROPERTIES
    OUTPUT_NAME mctrl              # 产物 mctrl.dll / mctrld.dll
    DEBUG_POSTFIX "d"
    WINDOWS_EXPORT_ALL_SYMBOLS ON)

target_compile_definitions(rpc_mctrl PRIVATE ISTAR_BUILDING)

# 手写头 + 生成头都对消费方可见
target_include_directories(rpc_mctrl PUBLIC
    "${RPC_ROOT}/include"
    "${GEN_RPC_DIR}/include")

target_include_directories(rpc_mctrl PRIVATE
    "${BASIC_INC}/common/include"
    "${BASIC_INC}/istarlog/include"
    "${BASIC_INC}/istarmw/include"
    "${BASIC_INC}/istargf/include"
    "${BASIC_INC}/istargf/include/istargf/file"
    "${BASIC_INC}/istarthreadpool/include"
    "${BASIC_INC}/istarcsv/include"
    "${BASIC_INC}/istarencryption/include"
    "${BASIC_INC}/istarfilesystem/include"
    "${BASIC_INC}/istarserializer/include"
    "${BASIC_INC}/istarsm/include"
    "${BASIC_INC}/istaryaml/include"
    "${BASIC_INC}/istarzmqworker/include"
)

target_link_directories(rpc_mctrl PUBLIC "${BASIC_LIB}")
target_link_libraries(rpc_mctrl PUBLIC ${RPC_LINK_LIBS})
