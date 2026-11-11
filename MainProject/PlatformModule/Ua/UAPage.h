/***********************************************************************
 * Copyright (C) 2024 ISTAR.
 *
 * File Name:        UAPage.h
 * Description:      Platform module integration - UA (User Admin)
 *                   Provides a single free function to get the UA module widget.
 * Version:          V0.1.0
 * Author:           iStar Platform
 * Date:             2026-09-02
 ***********************************************************************/

#ifndef UAPAGE_H
#define UAPAGE_H

#include <string>

class QWidget;

namespace PlatformModule
{

/**
 * @brief Get the UA (User Admin) module widget
 *
 * Dynamically loads libUIUA.dll and returns the module QWidget.
 * Internally manages the module lifecycle (singleton-style).
 * Returns nullptr if loading fails; caller can use a fallback page.
 *
 * Usage in MainWindow:
 *   #include "PlatformModule/Ua/UAPage.h"
 *   ...
 *   QWidget *w = PlatformModule::getUaModuleWidget("ua");
 */
QWidget *getUaModuleWidget(const std::string &moduleName = "ua");

/**
 * @brief Clean up the UA module before application exit
 *
 * Calls close() on the UA widget, which triggers MainView::closeEvent()
 * and stops ZMQ background threads.
 * Must be called before QApplication::quit() to ensure a clean exit.
 */
void cleanupUaModule();

/**
 * @brief Inject/clear the shared login session into libUIUA module page.
 *        Empty string means logout (module reverts to Visitor read-only).
 */
void setUASession(const char *sessionID);

/**
 * @brief 切换 UA 模块界面语言
 *
 * 调用 libUIUA 导出的 UIUA_SetLanguage：设置平台语言并刷新界面。
 * type: 0=英文, 1=中文。DLL 未加载时静默跳过（模块创建前后调用都安全）。
 */
void setUaLanguage(int type);

} // namespace PlatformModule

#endif // UAPAGE_H
