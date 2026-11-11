/***********************************************************************
 * Copyright (C) 2024 ISTAR.
 *
 * File Name:        EHPage.h
 * Description:      Platform module integration - EH (Equipment Handler / Alarm)
 *                   Provides a single free function to get the EH module widget.
 * Version:          V0.4.0
 * Author:           iStar Platform
 * Date:             2026-08-07
 ***********************************************************************/

#ifndef EHPAGE_H
#define EHPAGE_H

#include <string>

class QWidget;

namespace PlatformModule
{

/**
 * @brief Get the EH (Equipment Handler / Alarm) module widget
 *
 * Dynamically loads libUIEH.dll and returns the module QWidget.
 * Internally manages the module lifecycle (singleton-style).
 * Returns nullptr if loading fails; caller can use a fallback page.
 *
 * Usage in MainWindow:
 *   #include "PlatformModule/Eh/EHPage.h"
 *   ...
 *   QWidget *w = PlatformModule::getEhModuleWidget("eh");
 */
QWidget *getEhModuleWidget(const std::string& moduleName = "eh");

/**
 * @brief 获取 EH 整机二级导航“单页”widget（Report/Edit/History）
 *
 * 每个导航页对应界面库里的一个独立页面（无模块内左导航），
 * 由 libUIEH 导出的 UIEH_GetPageWidget(page) 按页名创建并缓存。
 */
QWidget *getEhPageWidget(const std::string& page);

/**
 * @brief 切换 EH 模块界面语言（type: 0=英文, 1=中文）
 *
 * 调用 libUIEH 导出的 UIEH_SetLanguage：设置平台语言并刷新已创建的
 * EH 界面。模块库未加载或尚未导出该接口时静默跳过（模块侧升级前为空操作）。
 */
void setEhLanguage(int type);

/**
 * @brief Clean up the EH module before application exit
 *
 * Calls close() on the EH widget, which triggers MainView::closeEvent()
 * and stops ZMQ background threads via EHGUIClient::CommunicationExit().
 * Must be called before QApplication::quit() to ensure a clean exit.
 */
void cleanupEhModule();

} // namespace PlatformModule

#endif // EHPAGE_H
