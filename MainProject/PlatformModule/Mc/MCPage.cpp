/***********************************************************************
 * Copyright (C) 2024 ISTAR.
 *
 * File Name:        MCPage.cpp
 * Description:      Platform module integration - MC (Machine Constant)
 *                   Dynamically loads the libUIMC UI library at runtime.
 * Version:          V0.4.0
 * Author:           iStar Platform
 * Date:             2026-08-07
 ***********************************************************************/

#include "MCPage.h"
#include "Common/ConfigRoot.h"

#include <QLibrary>
#include <QCoreApplication>
#include <QByteArray>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <map>
#include <string>

namespace PlatformModule
{

// Keep QLibrary alive for the entire application lifetime so that
// the DLL stays loaded while MC widgets are in use.
static QLibrary s_uiMcLib;
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

QWidget *getMcModuleWidget(const std::string& moduleName)
{
    auto it = s_cachedWidgets.find(moduleName);
    if (it != s_cachedWidgets.end() && nullptr != it->second)
    {
        return it->second;
    }

    // Minimal dynamic loading of libUIMC via QLibrary (no import .lib needed).
    if (!s_uiMcLib.isLoaded())
    {
    #ifdef GUI_DEBUG
        QString lib = "libUIMCd";
    #else
        QString lib = "libUIMC";
    #endif
        s_uiMcLib.setFileName(lib);
        if (!s_uiMcLib.load())
        {
            QWidget* pFallback = createFallbackPage(QString("MC module load failed: ") + s_uiMcLib.errorString());
            s_cachedWidgets[moduleName] = pFallback;
            return pFallback;
        }
        // 词典目录：libUIMC 默认按进程 cwd 找 Languages/MachineConstantName.csv，
        // 这里显式指到配置根，保证 cwd 不再是配置根也照样能翻译常数名。
        using SetDictPathFunc = void (*)(const char*);
        SetDictPathFunc pSetDictPath = reinterpret_cast<SetDictPathFunc>(
            s_uiMcLib.resolve("UIMC_SetMcDictPath"));
        if (nullptr != pSetDictPath)
        {
            static const QByteArray dictPath =
                ConfigRoot::Under(QStringLiteral("gui_config/Languages/MachineConstantName.csv")).toUtf8();
            pSetDictPath(dictPath.constData());
        }
    }

    QWidget* pWidget = nullptr;

    // Multi-instance aware interface: QWidget* UIMC_GetWidgetWithName(const char* moduleName)
    using GetWidgetWithNameFunc = QWidget* (*)(const char*);
    auto pGetWidgetWithName = reinterpret_cast<GetWidgetWithNameFunc>(
        s_uiMcLib.resolve("UIMC_GetWidgetWithName"));
    if (nullptr != pGetWidgetWithName)
    {
        pWidget = pGetWidgetWithName(moduleName.c_str());
    }

    // Fallback: single-instance C export: QWidget* UIMC_GetWidget()
    if (nullptr == pWidget)
    {
        using GetWidgetFunc = QWidget *(*)();
        GetWidgetFunc pGetWidgetFunc = reinterpret_cast<GetWidgetFunc>(
            s_uiMcLib.resolve("UIMC_GetWidget"));
        if (nullptr != pGetWidgetFunc)
        {
            pWidget = pGetWidgetFunc();
        }
    }

    if (nullptr == pWidget)
    {
        pWidget = createFallbackPage("MC module load failed");
    }

    s_cachedWidgets[moduleName] = pWidget;

    return pWidget;
}

void setMcLanguage(int type)
{
    // 模块未加载时静默跳过（切换入口在模块创建前后调用都安全）
    if (!s_uiMcLib.isLoaded())
    {
        return;
    }
    // libUIMC 导出 UIMC_SetLanguage(int)：设置平台语言 + Name 列词典语言并刷新界面
    using SetLanguageFunc = void (*)(int);
    auto pSetLanguage = reinterpret_cast<SetLanguageFunc>(s_uiMcLib.resolve("UIMC_SetLanguage"));
    if (nullptr != pSetLanguage)
    {
        pSetLanguage(type);
    }
}

void cleanupMcModule()
{
    // Calling close() on the MC widgets triggers MainView::closeEvent(),
    // which calls MCGUIClient::CommunicationExit() to stop ZMQ threads
    // and release other resources held by the MC modules.
    for (auto& pair : s_cachedWidgets)
    {
        if (nullptr != pair.second)
        {
            pair.second->close();
        }
    }
}

} // namespace PlatformModule
