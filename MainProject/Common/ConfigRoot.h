/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: ConfigRoot.h
* @Description: 配置根的唯一来源（平台约定 PLATFORM_PATH）。
*               进程内所有"配置根相对"的路径都必须经这里，禁止再依赖进程 cwd：
*                 <root>/appconfig.yaml        平台 AppConfig（与 mctrl/MT 共用）
*                 <root>/mw_config.yaml        进程级共享
*                 <root>/gui_config/...        本模块配置与资源
*                 <root>/Log/<进程名>/          日志
*               未设置 PLATFORM_PATH 时退回 "."（单机调试：cwd = build\<Config>，
*               构建期已把同样的目录形状铺到 exe 旁）。
*********************************************************************************/
#ifndef __GUI_CONFIG_ROOT_H__
#define __GUI_CONFIG_ROOT_H__

#include "istar_platform_env.h"

#include <QString>

#include <string>

namespace ConfigRoot
{
    /// 配置根：PLATFORM_PATH，未设置时用 cwd（F5 时就是 build\<Config>\）
    inline const QString &Path()
    {
        static const char *env = ISTARGetEnvPlatform();
        static const QString root = QString::fromLocal8Bit((env && *env) ? env : ".");
        return root;
    }

    /// <root>/<relative>
    inline QString Under(const QString &relative)
    {
        return Path() + QStringLiteral("/") + relative;
    }

    /// 进程级共享的中间件配置（根上一份，所有进程共用）
    inline std::string MwConfigFile()
    {
        return Under(QStringLiteral("mw_config.yaml")).toStdString();
    }
}

#endif // __GUI_CONFIG_ROOT_H__
