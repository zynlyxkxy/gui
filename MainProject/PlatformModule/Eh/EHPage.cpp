/***********************************************************************
 * Copyright (C) 2024 ISTAR.
 *
 * File Name:        EHPage.cpp
 * Description:      Platform module integration - EH (Equipment Handler / Alarm)
 *                   Dynamically loads the libUIEH UI library at runtime.
 * Version:          V0.4.0
 * Author:           iStar Platform
 * Date:             2026-08-07
 ***********************************************************************/

#include "EHPage.h"

#include <QLibrary>
#include <QCoreApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <map>
#include <string>

namespace PlatformModule
{

// Keep QLibrary alive for the entire application lifetime so that
// the DLL stays loaded while EH widgets are in use.
static QLibrary s_uiEhLib;
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

QWidget *getEhModuleWidget(const std::string& moduleName)
{
    auto it = s_cachedWidgets.find(moduleName);
    if (it != s_cachedWidgets.end() && nullptr != it->second)
    {
        return it->second;
    }

    // Minimal dynamic loading of libUIEH via QLibrary (no import .lib needed).
    // Debug build uses the 'd' suffixed DLL to stay on the Debug Qt runtime.
    if (!s_uiEhLib.isLoaded())
    {
#ifdef QT_DEBUG
        s_uiEhLib.setFileName("libUIEHd");
#else
        s_uiEhLib.setFileName("libUIEH");
#endif
        if (!s_uiEhLib.load())
        {
            QWidget* pFallback = createFallbackPage(QString("EH module load failed: ") + s_uiEhLib.errorString());
            s_cachedWidgets[moduleName] = pFallback;
            return pFallback;
        }
    }

    QWidget* pWidget = nullptr;

    // Multi-instance aware interface: QWidget* UIEH_GetWidgetWithName(const char* moduleName)
    using GetWidgetWithNameFunc = QWidget* (*)(const char*);
    auto pGetWidgetWithName = reinterpret_cast<GetWidgetWithNameFunc>(
        s_uiEhLib.resolve("UIEH_GetWidgetWithName"));
    if (nullptr != pGetWidgetWithName)
    {
        pWidget = pGetWidgetWithName(moduleName.c_str());
    }

    // Fallback: single-instance C export: QWidget* UIEH_GetWidget()
    if (nullptr == pWidget)
    {
        using GetWidgetFunc = QWidget *(*)();
        GetWidgetFunc pGetWidgetFunc = reinterpret_cast<GetWidgetFunc>(
            s_uiEhLib.resolve("UIEH_GetWidget"));
        if (nullptr != pGetWidgetFunc)
        {
            pWidget = pGetWidgetFunc();
        }
    }

    if (nullptr == pWidget)
    {
        pWidget = createFallbackPage("EH module load failed");
    }

    s_cachedWidgets[moduleName] = pWidget;
    return pWidget;
}

QWidget *getEhPageWidget(const std::string& page)
{
    auto it = s_cachedWidgets.find("page_" + page);
    if (it != s_cachedWidgets.end() && nullptr != it->second)
    {
        return it->second;
    }

    // 复用同一个 QLibrary（DLL 全局加载一次）
    if (!s_uiEhLib.isLoaded())
    {
        getEhModuleWidget("eh");   // 触发一次加载（含失败回退缓存）
        if (!s_uiEhLib.isLoaded())
        {
            QWidget* pFallback = createFallbackPage(QString("EH page load failed: ") + s_uiEhLib.errorString());
            s_cachedWidgets["page_" + page] = pFallback;
            return pFallback;
        }
    }

    using GetPageFunc = QWidget *(*)(const char*);
    auto pGetPage = reinterpret_cast<GetPageFunc>(s_uiEhLib.resolve("UIEH_GetPageWidget"));
    QWidget* pWidget = nullptr;
    if (nullptr != pGetPage)
    {
        pWidget = pGetPage(page.c_str());
    }
    if (nullptr == pWidget)
    {
        pWidget = createFallbackPage("EH page load failed");
    }
    s_cachedWidgets["page_" + page] = pWidget;
    return pWidget;
}


void setEhLanguage(int type)
{
    // 模块未加载时静默跳过（切换入口在模块创建前后调用都安全）
    if (!s_uiEhLib.isLoaded())
    {
        return;
    }
    // libUIEH 导出 UIEH_SetLanguage(int)：设置平台语言并刷新界面
    using SetLanguageFunc = void (*)(int);
    auto pSetLanguage = reinterpret_cast<SetLanguageFunc>(s_uiEhLib.resolve("UIEH_SetLanguage"));
    if (nullptr != pSetLanguage)
    {
        pSetLanguage(type);
    }
}

void cleanupEhModule()
{
    // Calling close() on the EH widgets triggers MainView::closeEvent(),
    // which calls EHGUIClient::CommunicationExit() to stop ZMQ threads
    // and release other resources held by the EH modules.
    for (auto& pair : s_cachedWidgets)
    {
        if (nullptr != pair.second)
        {
            pair.second->close();
        }
    }
}

} // namespace PlatformModule
