/***********************************************************************
 * Copyright (C) 2024 ISTAR.
 *
 * File Name:        MTPage.cpp
 * Description:      Platform module integration - MT (Metrology)
 *                   Dynamically loads the libUIMT UI library at runtime.
 * Version:          V0.1.0
 * Author:           iStar Platform
 * Date:             2026-10-16
 ***********************************************************************/

#include "MTPage.h"

#include <QLibrary>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QSizePolicy>
#include <map>
#include <string>

namespace PlatformModule
{

// 库句柄与页面缓存都存活到进程结束（与 EH/MC/ST 一致：DLL 卸载时机不可控，
// 收尾交给 cleanupMtModule 里的 UIMT_PageCleanup，句柄本身留到进程退出回收）
static QLibrary s_uiMtLib;
static std::map<std::string, QWidget*> s_cachedWidgets;

static QWidget *createFallbackPage(const QString &reason)
{
    QWidget *pPage = new QWidget();
    pPage->setStyleSheet("background-color: #F3F4FA;");
    QVBoxLayout *pLayout = new QVBoxLayout(pPage);
    pLayout->setAlignment(Qt::AlignCenter);
    QLabel *pLabel = new QLabel(reason, pPage);
    pLabel->setAlignment(Qt::AlignCenter);
    pLabel->setStyleSheet("QLabel{color:#999999; font-size:14px;}");
    pLayout->addWidget(pLabel);
    return pPage;
}

// 加载 libUIMT（Debug 用带 d 后缀版本），返回是否已就绪；失败时 error 带回原因
static bool ensureMtLibLoaded(QString *pError)
{
    if (s_uiMtLib.isLoaded())
    {
        return true;
    }
    // Minimal dynamic loading of libUIMT via QLibrary (no import .lib needed).
#ifdef GUI_DEBUG
    s_uiMtLib.setFileName(QStringLiteral("libUIMTd"));
#else
    s_uiMtLib.setFileName(QStringLiteral("libUIMT"));
#endif
    if (!s_uiMtLib.load())
    {
        if (nullptr != pError)
        {
            *pError = s_uiMtLib.errorString();
        }
        return false;
    }
    return true;
}

QWidget *getMtPageWidget(const std::string& page)
{
    auto it = s_cachedWidgets.find(page);
    if (it != s_cachedWidgets.end() && nullptr != it->second)
    {
        return it->second;
    }

    // 只加载 DLL 并创建单页，不创建一体式 MainView（见头文件说明）
    QString error;
    if (!ensureMtLibLoaded(&error))
    {
        QWidget* pFallback = createFallbackPage(QString("MT page load failed: ") + error);
        s_cachedWidgets[page] = pFallback;
        return pFallback;
    }

    using GetPageFunc = QWidget *(*)(const char*);
    auto pGetPage = reinterpret_cast<GetPageFunc>(s_uiMtLib.resolve("UIMT_GetPageWidget"));
    QWidget* pWidget = nullptr;
    if (nullptr != pGetPage)
    {
        pWidget = pGetPage(page.c_str());
    }
    else
    {
        qWarning("MT module: resolve UIMT_GetPageWidget failed: %s",
                 qPrintable(s_uiMtLib.errorString()));
    }

    if (nullptr != pWidget)
    {
        // MT 页面内部控件的最小宽度一度会把整机主窗口撑出屏幕（右侧二级导航被截断）。
        // 平台修复后已能容纳 1920 屏，这里保留"忽略页面尺寸提示"作为窄屏兜底 ——
        // 宁可页面内部被压缩，也不要整机被撑出屏幕。
        pWidget->setMinimumSize(0, 0);
        pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    }
    else
    {
        pWidget = createFallbackPage("MT page load failed");
    }

    s_cachedWidgets[page] = pWidget;
    return pWidget;
}

void setMtLanguage(int type)
{
    // 模块未加载时静默跳过（切换入口在模块创建前后调用都安全）
    if (!s_uiMtLib.isLoaded())
    {
        return;
    }
    // libUIMT 导出 UIMT_SetLanguage(int)：设置平台语言并刷新界面
    using SetLanguageFunc = void (*)(int);
    auto pSetLanguage = reinterpret_cast<SetLanguageFunc>(s_uiMtLib.resolve("UIMT_SetLanguage"));
    if (nullptr != pSetLanguage)
    {
        pSetLanguage(type);
    }
}

void cleanupMtModule()
{
    // 关闭已创建的页面；随后由库统一收尾（UIMT_PageCleanup 内部：关单页、
    // 停止 Python 脚本进程、CommunicationExit 释放 ZMQ 客户端）
    for (auto& pair : s_cachedWidgets)
    {
        if (nullptr != pair.second)
        {
            pair.second->close();
        }
    }
    s_cachedWidgets.clear();

    if (s_uiMtLib.isLoaded())
    {
        using CleanupFunc = void (*)();
        auto pCleanup = reinterpret_cast<CleanupFunc>(s_uiMtLib.resolve("UIMT_PageCleanup"));
        if (nullptr != pCleanup)
        {
            pCleanup();
        }
    }
}

} // namespace PlatformModule
