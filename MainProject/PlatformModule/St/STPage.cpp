/***********************************************************************
 * Copyright (C) 2024 ISTAR.
 *
 * File Name:        STPage.cpp
 * Description:      Platform module integration - ST (Safety)
 *                   Dynamically loads the libUIST UI library at runtime.
 * Version:          V0.1.0
 * Author:           iStar Platform
 * Date:             2026-08-22
 ***********************************************************************/

#include "STPage.h"

#include <QLibrary>
#include <QCoreApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <map>
#include <string>

// libUIST 导出 UILO::createSTModule(QObject*) -> IModule*，经 IModule::moduleWidget() 取 QWidget
#include "IModule.h"

namespace PlatformModule
{

// Keep QLibrary alive for the entire application lifetime so that
// the DLL stays loaded while ST widgets are in use.
static QLibrary s_uiStLib;
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

QWidget *getStModuleWidget(const std::string& moduleName)
{
    auto it = s_cachedWidgets.find(moduleName);
    if (it != s_cachedWidgets.end() && nullptr != it->second)
    {
        return it->second;
    }

    // Minimal dynamic loading of libUIST via QLibrary (no import .lib needed).
    if (!s_uiStLib.isLoaded())
    {
    #ifdef GUI_DEBUG
        QString lib = "libUISTd";
    #else
        QString lib = "libUIST";
    #endif
        s_uiStLib.setFileName(lib);
        if (!s_uiStLib.load())
        {
            QWidget* pFallback = createFallbackPage(QString("ST module load failed: ") + s_uiStLib.errorString());
            s_cachedWidgets[moduleName] = pFallback;
            return pFallback;
        }
    }

    QWidget* pWidget = nullptr;

    // libUIST 工厂接口: UILO::createSTModule(QObject*) -> IModule*
    using CreateSTModuleFunc = IModule* (*)(QObject*);
    auto pCreate = reinterpret_cast<CreateSTModuleFunc>(s_uiStLib.resolve("createSTModule"));
    if (nullptr != pCreate)
    {
        IModule* pModule = pCreate(nullptr);
        if (pModule && pModule->initialize())
        {
            pWidget = pModule->moduleWidget();
        }
    }
    else
    {
        qWarning("ST module: resolve createSTModule failed: %s", qPrintable(s_uiStLib.errorString()));
    }

    if (nullptr == pWidget)
    {
        QWidget* pFallback = createFallbackPage("ST module widget unavailable");
        s_cachedWidgets[moduleName] = pFallback;
        return pFallback;
    }

    s_cachedWidgets[moduleName] = pWidget;
    return pWidget;
}

void setStLanguage(int type)
{
    // 模块未加载时静默跳过（切换入口在模块创建前后调用都安全）
    if (!s_uiStLib.isLoaded())
    {
        return;
    }
    // libUIST 导出 UIST_SetLanguage(int)：设置平台语言并刷新界面
    // （当前平台 libUIST 尚未导出该接口，resolve 失败时静默跳过）
    using SetLanguageFunc = void (*)(int);
    auto pSetLanguage = reinterpret_cast<SetLanguageFunc>(s_uiStLib.resolve("UIST_SetLanguage"));
    if (nullptr != pSetLanguage)
    {
        pSetLanguage(type);
    }
}

} // namespace PlatformModule
