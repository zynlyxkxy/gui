/***********************************************************************
 * Copyright (C) 2024 ISTAR.
 *
 * File Name:        RMPage.h
 * Description:      Platform module integration - RM (Recipe Management)
 *                   Provides a single free function to get the RM module widget.
 * Version:          V0.5.0
 * Author:           iStar Platform
 * Date:             2026-08-10
 ***********************************************************************/

#ifndef RMPAGE_H
#define RMPAGE_H

#include <string>

class QWidget;

namespace PlatformModule
{

/**
 * @brief Get the RM (Recipe Management) module widget
 *
 * Dynamically loads libUIRm.dll (platform UI library, name fixed) and returns
 * the module QWidget. Supports multi-instance (moduleName: rm/rm2/rm3) and a
 * custom external editor (communication) library name — the library loaded
 * internally by libUIRm (libUIRE by default), so different instances can use
 * different editor libraries.
 * Internally manages the module lifecycle (singleton-style).
 * Returns a fallback page if loading fails.
 *
 * Usage in MainWindow:
 *   #include "PlatformModule/Rm/RMPage.h"
 *   ...
 *   QWidget *w = PlatformModule::getRmModuleWidget("rm");                  // 默认通信库 libUIRE
 *   QWidget *w2 = PlatformModule::getRmModuleWidget("rm2", "libUIREtest"); // 换通信(外部编辑器)库
 *
 * @param moduleName  RM module/instance name, e.g. "rm"/"rm2"/"rm3"
 * @param libname     External editor (communication) library name loaded by libUIRm, e.g. "libUIRE"
 */
QWidget *getRmModuleWidget(const std::string& moduleName = "rm",
                           const std::string &libname = "libUIRE");

/**
 * @brief 切换 RM 模块界面语言（type: 0=英文, 1=中文）
 *
 * 调用 libUIRm 导出的 UIRm_SetLanguage：设置平台语言 + Name 列词典语言并刷新
 * 已创建的 RM 界面（同一库下 rm/rm2 等多实例一并刷新）。模块库未加载或尚未
 * 导出该接口时静默跳过（模块侧升级前为空操作）。
 */
void setRmLanguage(int type);

/**
 * @brief Clean up the RM module before application exit
 *
 * Calls close() on the RM widget, which triggers View::closeEvent()
 * and stops ZMQ background threads via Communication_Exit().
 * Must be called before QApplication::quit() to ensure a clean exit.
 */
void cleanupRmModule();

} // namespace PlatformModule

#endif // RMPAGE_H
