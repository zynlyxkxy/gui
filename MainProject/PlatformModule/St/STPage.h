/***********************************************************************
 * Copyright (C) 2024 ISTAR.
 *
 * File Name:        STPage.h
 * Description:      Platform module integration - ST (Safety)
 *                   Provides a single free function to get the ST module widget.
 * Version:          V0.1.0
 * Author:           iStar Platform
 * Date:             2026-08-22
 ***********************************************************************/

#ifndef STPAGE_H
#define STPAGE_H

#include <string>

class QWidget;

namespace PlatformModule
{

/**
 * @brief Get the ST (Safety) module widget
 *
 * Dynamically loads libUIST.dll and returns the module QWidget.
 * Internally manages the module lifecycle (singleton-style).
 * Returns nullptr if loading fails; caller can use a fallback page.
 *
 * Usage in MainWindow:
 *   #include "PlatformModule/St/STPage.h"
 *   ...
 *   QWidget *w = PlatformModule::getStModuleWidget("st");
 */
QWidget *getStModuleWidget(const std::string& moduleName = "st");

/**
 * @brief 切换 ST 模块界面语言（type: 0=英文, 1=中文）
 *
 * 调用 libUIST 导出的 UIST_SetLanguage：设置平台语言并刷新已创建的
 * ST 界面。模块库未加载或尚未导出该接口时静默跳过（模块侧升级前为空操作）。
 */
void setStLanguage(int type);

} // namespace PlatformModule

#endif // STPAGE_H
