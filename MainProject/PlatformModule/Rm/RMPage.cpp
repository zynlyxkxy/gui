/***********************************************************************
 * Copyright (C) 2024 ISTAR.
 *
 * File Name:        RMPage.cpp
 * Description:      Platform module integration - RM (Recipe Management)
 *                   Dynamically loads the libUIRM UI library at runtime and
 *                   passes the external editor (communication) library name
 *                   (e.g. libUIRE) into it, so different RM instances can use
 *                   different editor libraries.
 * Version:          V0.5.0
 * Author:           iStar Platform
 * Date:             2026-08-10
 ***********************************************************************/

#include "RMPage.h"
#include "IREModule.h"
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
// the DLL stays loaded while RM widgets are in use.
// libUIRm is the platform UI library (name fixed); the external editor
// (communication) library it loads inside is dynamic, see libname below.
static QLibrary s_uiRmLib;
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

QWidget *getRmModuleWidget(const std::string& moduleName, const std::string &libname)
{
    // A widget instance is identified by module name + external editor
    // (communication) library name: the same moduleName with different
    // editor libraries yields independent widgets.
    const std::string cacheKey = moduleName + "|" + libname;

    auto it = s_cachedWidgets.find(cacheKey);
    if (it != s_cachedWidgets.end() && nullptr != it->second)
    {
        return it->second;
    }

    #ifdef GUI_DEBUG
        QString lib = "libUIRmd" ;
    #else
        QString lib = "libUIRm";
    #endif

    // Minimal dynamic loading of libUIRM via QLibrary (no import .lib needed).
    // libUIRm is the platform UI library and is always loaded by this name.
    if (!s_uiRmLib.isLoaded())
    {
        s_uiRmLib.setFileName(lib);
        if (!s_uiRmLib.load())
        {
            QWidget* pFallback = createFallbackPage(QString("RM module load failed: ") + s_uiRmLib.errorString());
            s_cachedWidgets[cacheKey] = pFallback;
            return pFallback;
        }
        // 词典目录：libUIRm 默认按进程 cwd 找 Languages/RecipeName.csv，
        // 这里显式指到配置根，保证 cwd 不再是配置根也照样能翻译处方名。
        using SetDictPathFunc = void (*)(const char*);
        SetDictPathFunc pSetDictPath = reinterpret_cast<SetDictPathFunc>(
            s_uiRmLib.resolve("UIRm_SetMcDictPath"));
        if (nullptr != pSetDictPath)
        {
            static const QByteArray dictPath =
                ConfigRoot::Under(QStringLiteral("gui_config/Languages/RecipeName.csv")).toUtf8();
            pSetDictPath(dictPath.constData());
        }
    }

    QWidget* pWidget = nullptr;

    // Preferred multi-instance interface with custom external editor
    // (communication) library:
    //   QWidget* UIRm_GetWidgetWithLib(const char* moduleName, const char* extLibName)
    using GetWidgetWithLibFunc = QWidget* (*)(const char*, const char*);
    auto pGetWidgetWithLib = reinterpret_cast<GetWidgetWithLibFunc>(
        s_uiRmLib.resolve("UIRm_GetWidgetWithLib"));
    if (nullptr != pGetWidgetWithLib)
    {
        pWidget = pGetWidgetWithLib(moduleName.c_str(), libname.c_str());
    }

    // Fallback 1: multi-instance interface using the default external editor library:
    //   QWidget* UIRm_GetWidgetWithName(const char* moduleName)
    if (nullptr == pWidget)
    {
        using GetWidgetWithNameFunc = QWidget* (*)(const char*);
        auto pGetWidgetWithName = reinterpret_cast<GetWidgetWithNameFunc>(
            s_uiRmLib.resolve("UIRm_GetWidgetWithName"));
        if (nullptr != pGetWidgetWithName)
        {
            pWidget = pGetWidgetWithName(moduleName.c_str());
        }
    }

    // Fallback 2: Preferred contract from IREModule.h: "CreateREModule" factory (single-instance legacy)
    if (nullptr == pWidget)
    {
        CreateREModuleFunc pCreateFunc = reinterpret_cast<CreateREModuleFunc>(
            s_uiRmLib.resolve(CREATE_RE_MODULE_FUNC));
        if (nullptr != pCreateFunc)
        {
            IREModule *pModule = pCreateFunc();
            if (nullptr != pModule && nullptr != pModule->getWidget())
            {
                pWidget = pModule->getWidget();
            }
        }
    }

    // Fallback 3: Legacy actual C export: QWidget* UIRm_GetWidget()
    if (nullptr == pWidget)
    {
        using UIRmGetWidgetFunc = QWidget *(*)();
        UIRmGetWidgetFunc pGetWidgetFunc = reinterpret_cast<UIRmGetWidgetFunc>(
            s_uiRmLib.resolve("UIRm_GetWidget"));
        if (nullptr != pGetWidgetFunc)
        {
            pWidget = pGetWidgetFunc();
        }
    }

    if (nullptr == pWidget)
    {
        pWidget = createFallbackPage("RM module load failed");
    }

    s_cachedWidgets[cacheKey] = pWidget;
    return pWidget;
}

void setRmLanguage(int type)
{
    // 模块未加载时静默跳过（切换入口在模块创建前后调用都安全）
    if (!s_uiRmLib.isLoaded())
    {
        return;
    }
    // libUIRm 导出 UIRm_SetLanguage(int)：设置平台语言 + Name 列词典语言并刷新界面
    using SetLanguageFunc = void (*)(int);
    auto pSetLanguage = reinterpret_cast<SetLanguageFunc>(s_uiRmLib.resolve("UIRm_SetLanguage"));
    if (nullptr != pSetLanguage)
    {
        pSetLanguage(type);
    }
}

void cleanupRmModule()
{
    // Calling close() on the RM widgets triggers View::closeEvent(),
    // which calls RM_GUIClient::Communication_Exit() to stop ZMQ threads
    // and release other resources held by the RM modules.
    for (auto& pair : s_cachedWidgets)
    {
        if (nullptr != pair.second)
        {
            pair.second->close();
        }
    }
}

} // namespace PlatformModule
