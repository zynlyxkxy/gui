/***********************************************************************
 * Copyright (C) 2024 ISTAR.
 *
 * File Name:        RPPage.cpp
 * Description:      Platform module integration - RP (Report)
 *                   Dynamically loads the libUIRP UI library at runtime.
 * Version:          V0.1.0
 * Author:           iStar Platform
 * Date:             2026-08-31
 ***********************************************************************/

#include "RPPage.h"

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
// the DLL stays loaded while RP widgets are in use.
static QLibrary s_uiRpLib;
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

QWidget *getRpModuleWidget(const std::string& moduleName)
{
    auto it = s_cachedWidgets.find(moduleName);
    if (it != s_cachedWidgets.end() && nullptr != it->second)
    {
        return it->second;
    }

    // Minimal dynamic loading of libUIRP via QLibrary (no import .lib needed).
    if (!s_uiRpLib.isLoaded())
    {
    #ifdef GUI_DEBUG
        QString lib = "libUIRPd";
    #else
        QString lib = "libUIRP";
    #endif
        s_uiRpLib.setFileName(lib);
        if (!s_uiRpLib.load())
        {
            QWidget* pFallback = createFallbackPage(QString("RP module load failed: ") + s_uiRpLib.errorString());
            s_cachedWidgets[moduleName] = pFallback;
            return pFallback;
        }
    }

    QWidget* pWidget = nullptr;

    // Multi-instance aware interface: QWidget* UIRP_GetWidgetWithName(const char* moduleName)
    using GetWidgetWithNameFunc = QWidget* (*)(const char*);
    auto pGetWidgetWithName = reinterpret_cast<GetWidgetWithNameFunc>(
        s_uiRpLib.resolve("UIRP_GetWidgetWithName"));
    if (nullptr != pGetWidgetWithName)
    {
        pWidget = pGetWidgetWithName(moduleName.c_str());
    }

    // Fallback: single-instance C export: QWidget* UIRP_GetWidget()
    if (nullptr == pWidget)
    {
        using GetWidgetFunc = QWidget *(*)();
        GetWidgetFunc pGetWidgetFunc = reinterpret_cast<GetWidgetFunc>(
            s_uiRpLib.resolve("UIRP_GetWidget"));
        if (nullptr != pGetWidgetFunc)
        {
            pWidget = pGetWidgetFunc();
        }
    }

    if (nullptr == pWidget)
    {
        pWidget = createFallbackPage("RP module load failed");
    }

    s_cachedWidgets[moduleName] = pWidget;

    return pWidget;
}

void setRpLanguage(int type)
{
    // 模块未加载时静默跳过（切换入口在模块创建前后调用都安全）
    if (!s_uiRpLib.isLoaded())
    {
        return;
    }
    // libUIRP 导出 UIRP_SetLanguage(int)：设置平台语言并刷新界面
    using SetLanguageFunc = void (*)(int);
    auto pSetLanguage = reinterpret_cast<SetLanguageFunc>(s_uiRpLib.resolve("UIRP_SetLanguage"));
    if (nullptr != pSetLanguage)
    {
        pSetLanguage(type);
    }
}

void cleanupRpModule()
{
    // Calling close() on the RP widgets triggers MainView::closeEvent(),
    // which calls RPGUIClient::CommunicationExit() to stop ZMQ threads
    // and release other resources held by the RP modules.
    for (auto& pair : s_cachedWidgets)
    {
        if (nullptr != pair.second)
        {
            pair.second->close();
        }
    }
}

} // namespace PlatformModule
