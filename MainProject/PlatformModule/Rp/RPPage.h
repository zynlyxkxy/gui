/***********************************************************************
 * Copyright (C) 2024 ISTAR.
 *
 * File Name:        RPPage.h
 * Description:      Platform module integration - RP (Report)
 *                   Provides a single free function to get the RP module widget.
 * Version:          V0.1.0
 * Author:           iStar Platform
 * Date:             2026-08-31
 ***********************************************************************/

#ifndef RPPAGE_H
#define RPPAGE_H

#include <string>

class QWidget;

namespace PlatformModule
{

/**
 * @brief Get the RP (Report) module widget
 *
 * Dynamically loads libUIRP.dll and returns the module QWidget.
 * Internally manages the module lifecycle (singleton-style).
 * Returns nullptr if loading fails; caller can use a fallback page.
 *
 * Usage in MainWindow:
 *   #include "PlatformModule/Rp/RPPage.h"
 *   ...
 *   QWidget *w = PlatformModule::getRpModuleWidget("rp");
 */
QWidget *getRpModuleWidget(const std::string& moduleName = "rp");

/**
 * @brief 切换 RP 模块界面语言（type: 0=英文, 1=中文）
 *
 * 调用 libUIRP 导出的 UIRP_SetLanguage：设置平台语言并刷新已创建的
 * RP 界面。模块库未加载或尚未导出该接口时静默跳过（模块侧升级前为空操作）。
 */
void setRpLanguage(int type);

/**
 * @brief Clean up the RP module before application exit
 *
 * Calls close() on the RP widget, which triggers MainView::closeEvent()
 * and stops ZMQ background threads via RPGUIClient::CommunicationExit().
 * Must be called before QApplication::quit() to ensure a clean exit.
 */
void cleanupRpModule();

} // namespace PlatformModule

#endif // RPPAGE_H
