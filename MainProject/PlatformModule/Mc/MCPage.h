/***********************************************************************
 * Copyright (C) 2024 ISTAR.
 *
 * File Name:        MCPage.h
 * Description:      Platform module integration - MC (Machine Constant)
 *                   Provides a single free function to get the MC module widget.
 * Version:          V0.4.0
 * Author:           iStar Platform
 * Date:             2026-08-07
 ***********************************************************************/

#ifndef MCPAGE_H
#define MCPAGE_H

#include <string>

class QWidget;

namespace PlatformModule
{

/**
 * @brief Get the MC (Machine Constant) module widget
 *
 * Dynamically loads libUIMC.dll and returns the module QWidget.
 * Internally manages the module lifecycle (singleton-style).
 * Returns nullptr if loading fails; caller can use a fallback page.
 *
 * Usage in MainWindow:
 *   #include "PlatformModule/Mc/MCPage.h"
 *   ...
 *   QWidget *w = PlatformModule::getMcModuleWidget("mc");
 */
QWidget *getMcModuleWidget(const std::string& moduleName = "mc");

/**
 * @brief 切换 MC 模块界面语言（type: 0=英文, 1=中文）
 *
 * 调用 libUIMC 导出的 UIMC_SetLanguage：设置平台语言 + Name 列词典语言并刷新
 * 已创建的 MC 界面。模块库未加载或尚未导出该接口时静默跳过（模块侧升级前为空操作）。
 */
void setMcLanguage(int type);

/**
 * @brief Clean up the MC module before application exit
 *
 * Calls close() on the MC widget, which triggers MainView::closeEvent()
 * and stops ZMQ background threads via MCGUIClient::CommunicationExit().
 * Must be called before QApplication::quit() to ensure a clean exit.
 */
void cleanupMcModule();

} // namespace PlatformModule

#endif // MCPAGE_H
