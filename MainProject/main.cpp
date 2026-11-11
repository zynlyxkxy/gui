/***********************************************************************
* Copyright (C) 2024 ISTAR.
*
* File Name:        main.cpp
* Description:      ISTAR GUI Framework Example - Entry Point
* Version:          V0.4.0
* Author:           iStar Platform
* Date:             2025-07-24
***********************************************************************/

#include <QApplication>
#include "MainWindow.h"
#include "Common/ConfigRoot.h"
#include "istar_log.h"
#include "istar_appconfig.h"
#include "istar_language.h"

#ifdef GUI_DEBUG
#include <windows.h>
#include <cstdio>
#include <QDateTime>

// ============================================================================
// 调试输出（仅 Debug 构建，GUI_DEBUG 由 CMake 按 CONFIG 定义；Release 不编译这段）：
// 1) AllocConsole 弹一个控制台，qDebug/printf 直接可见（双击 exe 调试也有输出）；
// 2) OutputDebugStringW 同步一份，VSCode(cppvsdbg) 调试控制台 / DbgView 都能收到。
// 控制台代码页设 UTF-8，输出按 UTF-8 写，中文不乱码。
// ============================================================================
static void guiDebugMessageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    const char *level = "Info";
    switch (type)
    {
    case QtDebugMsg:    level = "Debug";    break;
    case QtInfoMsg:     level = "Info";     break;
    case QtWarningMsg:  level = "Warning";  break;
    case QtCriticalMsg: level = "Critical"; break;
    case QtFatalMsg:    level = "Fatal";    break;
    }

    QString text = QStringLiteral("[%1 %2] %3")
                       .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz")),
                            QLatin1String(level), msg);
    if (ctx.file != nullptr)
    {
        text += QStringLiteral("  (%1:%2)").arg(QString::fromUtf8(ctx.file)).arg(ctx.line);
    }
    text += QLatin1Char('\n');

    const QByteArray raw = text.toUtf8();
    fwrite(raw.constData(), 1, static_cast<size_t>(raw.size()), stderr);
    fflush(stderr);
    OutputDebugStringW(reinterpret_cast<const wchar_t *>(text.utf16()));
}
#endif

int main(int argc, char *argv[])
{
#ifdef GUI_DEBUG
    if (AllocConsole())
    {
        SetConsoleOutputCP(CP_UTF8);
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
    qInstallMessageHandler(guiDebugMessageHandler);
#endif

    QApplication a(argc, argv);

    // 配置根（平台约定 PLATFORM_PATH）：日志、appconfig、mw_config、Languages 都在它下面
    const QString configRoot = ConfigRoot::Path();

    ISTARResultStatus ret = ISTAR_RESULT_FAILED;
    // 日志根与配置根同一个：<root>/Log/<进程名>/
    ret = ISTARLogInitWithPath(configRoot.toStdString().c_str(), "GUI");
    if(ISTAR_RESULT_SUCCESS != ret)
    {
        ISTARLogDebug("ISTARLogInitWithPath failed");
    }

    // 压日志噪声：控制台与文件只留 INFO~ERROR（TRACE/DEBUG 不再刷屏）
    ISTARLogSetConsoleLevel(ISTARGF_LOG_INFO, ISTARGF_LOG_ERROR);
    ISTARLogSetFileLevel(ISTARGF_LOG_INFO, ISTARGF_LOG_ERROR);

    // 初始化应用配置：配置根上的 appconfig.yaml（与 mctrl / MT 同一份）
    ISTAR::BasicPlatform::ISTARAppConfigManager::getInstance()->Initialize(
        ConfigRoot::Under(QStringLiteral("appconfig.yaml")).toStdString());

    MainWindow w;
    if (w.IsLicenseRefused())
    {
        ISTARLogShutdown();
        return 0;            // 不 show()、不 exec() → 进程干净退出，无需任何 cleanup
    }
    w.show();

    int exitCode = a.exec();
    ISTARLogShutdown();
    return exitCode;
}
