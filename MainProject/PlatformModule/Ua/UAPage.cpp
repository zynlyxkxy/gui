/***********************************************************************
 * Copyright (C) 2024 ISTAR.
 *
 * File Name:        UAPage.cpp
 * Description:      Platform module integration - UA (User Admin)
 *                   Dynamically loads the libUIUA UI library at runtime.
 * Version:          V0.1.0
 * Author:           iStar Platform
 * Date:             2026-09-02
 ***********************************************************************/

#include "UAPage.h"

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
// the DLL stays loaded while UA widgets are in use.
static QLibrary s_uiUaLib;
static std::map<std::string, QWidget*> s_cachedWidgets;

static bool ensureLoaded()
{
    if (!s_uiUaLib.isLoaded())
    {
#ifdef QT_DEBUG
        s_uiUaLib.setFileName(QCoreApplication::applicationDirPath() + "/libUIUAd");
#else
        s_uiUaLib.setFileName(QCoreApplication::applicationDirPath() + "/libUIUA");
#endif
        if (!s_uiUaLib.load())
        {
            return false;
        }
    }
    return true;
}

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

QWidget *getUaModuleWidget(const std::string& moduleName)
{
    auto it = s_cachedWidgets.find(moduleName);
    if (it != s_cachedWidgets.end() && nullptr != it->second)
    {
        return it->second;
    }

    // Minimal dynamic loading of libUIUA via QLibrary (no import .lib needed).
    if (!s_uiUaLib.isLoaded())
    {
    #ifdef GUI_DEBUG
        QString lib = "libUIUAd";
    #else
        QString lib = "libUIUA";
    #endif
        s_uiUaLib.setFileName(lib);
        if (!s_uiUaLib.load())
        {
            QWidget* pFallback = createFallbackPage(QString("UA module load failed: ") + s_uiUaLib.errorString());
            s_cachedWidgets[moduleName] = pFallback;
            return pFallback;
        }
    }

    QWidget* pWidget = nullptr;

    // Multi-instance aware interface: QWidget* UIUA_GetWidgetWithName(const char* moduleName)
    using GetWidgetWithNameFunc = QWidget* (*)(const char*);
    auto pGetWidgetWithName = reinterpret_cast<GetWidgetWithNameFunc>(
        s_uiUaLib.resolve("UIUA_GetWidgetWithName"));
    if (nullptr != pGetWidgetWithName)
    {
        pWidget = pGetWidgetWithName(moduleName.c_str());
    }

    // Fallback: single-instance C export: QWidget* UIUA_GetWidget()
    if (nullptr == pWidget)
    {
        using GetWidgetFunc = QWidget *(*)();
        GetWidgetFunc pGetWidgetFunc = reinterpret_cast<GetWidgetFunc>(
            s_uiUaLib.resolve("UIUA_GetWidget"));
        if (nullptr != pGetWidgetFunc)
        {
            pWidget = pGetWidgetFunc();
        }
    }

    if (nullptr == pWidget)
    {
        pWidget = createFallbackPage("UA module load failed");
    }

    s_cachedWidgets[moduleName] = pWidget;

    return pWidget;
}

void cleanupUaModule()
{
    // Calling close() on the UA widgets triggers MainView::closeEvent(),
    // which stops ZMQ threads and releases other resources held by the UA module.
    for (auto& pair : s_cachedWidgets)
    {
        if (nullptr != pair.second)
        {
            pair.second->close();
        }
    }
}

void setUASession(const char *sessionID)
{
    if (!ensureLoaded())
    {
        return;
    }
    using SetSessionFunc = void (*)(const char *);
    auto pSet = reinterpret_cast<SetSessionFunc>(s_uiUaLib.resolve("UIUA_SetSessionID"));
    if (nullptr != pSet)
    {
        pSet(sessionID);
    }
}

void setUaLanguage(int type)
{
    // 模块未加载时静默跳过（切换入口在模块创建前后调用都安全）
    if (!ensureLoaded())
    {
        return;
    }
    using SetLanguageFunc = void (*)(int);
    auto pSetLanguage = reinterpret_cast<SetLanguageFunc>(s_uiUaLib.resolve("UIUA_SetLanguage"));
    if (nullptr != pSetLanguage)
    {
        pSetLanguage(type);
    }
}


} // namespace PlatformModule
