/***********************************************************************
 * Copyright (C) 2024 ISTAR.
 *
 * File Name:        MTPage.h
 * Description:      Platform module integration - MT (Metrology)
 *                   Provides free functions to get the MT second-level
 *                   navigation page widgets (Single / Queue / History).
 * Version:          V0.1.0
 * Author:           iStar Platform
 * Date:             2026-10-16
 ***********************************************************************/

#ifndef MTPAGE_H
#define MTPAGE_H

#include <string>

class QWidget;

namespace PlatformModule
{

/**
 * @brief 获取 MT（测校）整机二级导航"单页"widget
 *
 * 每个导航页对应界面库里的一个独立页面（无模块内左导航），由 libUIMT
 * 导出的 UIMT_GetPageWidget(page) 按页名创建并缓存。页名支持别名：
 * Single/SingleScript、Queue/Metrology、History/Report。
 *
 * 整机"单页"模式下**不**调用 UIMT_GetWidgetWithName 创建一体式 MainView ——
 * 库内 PageCoordinator 已承担其职责，MainView 存活会让 UIMT_PageCleanup
 * 跳过共享通信退出，导致后台线程残留。加载失败返回兜底页。
 *
 * Usage in MainWindow:
 *   #include "PlatformModule/Mt/MTPage.h"
 *   ...
 *   QWidget *w = PlatformModule::getMtPageWidget("Single");
 */
QWidget *getMtPageWidget(const std::string& page);

/**
 * @brief 切换 MT 模块界面语言（type: 0=英文, 1=中文）
 *
 * 调用 libUIMT 导出的 UIMT_SetLanguage：设置平台语言并刷新已创建的
 * MT 界面。模块库未加载或尚未导出该接口时静默跳过。
 */
void setMtLanguage(int type);

/**
 * @brief Clean up the MT module before application exit
 *
 * 关闭已创建的 MT 页面并调用 libUIMT 导出的 UIMT_PageCleanup()：停止
 * Python 脚本进程、退出共享通信（ZMQ 线程）。必须在 QApplication::quit()
 * 之前调用，否则后台线程会吊住进程退出。
 */
void cleanupMtModule();

} // namespace PlatformModule

#endif // MTPAGE_H
