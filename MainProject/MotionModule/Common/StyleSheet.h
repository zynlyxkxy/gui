/***********************************************************************
* Copyright (C) 2024, iStar.
*
* File Name: StyleSheet.h
* Description: GUI style constants matching MotionInterface design system
* Version:          V1.0.0
* Author:           Auto-generated
* Date:             2026-07-26
***********************************************************************/

#ifndef GUI_STYLESHEET_H
#define GUI_STYLESHEET_H

namespace StyleSheet
{

// ── Colors ──────────────────────────────────────────────
constexpr const char *Color_Blue   = "#4C70FA";
constexpr const char *Color_Green  = "#35EF14";
constexpr const char *Color_Red    = "#FD6B6D";
constexpr const char *Color_Yellow = "#EDAD16";

// ── Font styles ─────────────────────────────────────────
constexpr const char *fontStyle_normal_12 =
    "QWidget{font-family:HarmonyOS Sans, Microsoft YaHei;font-weight:normal;font-size:12pt;}";
constexpr const char *fontStyle_normal_14 =
    "QWidget{font-family:HarmonyOS Sans, Microsoft YaHei;font-weight:normal;font-size:14pt;}";
constexpr const char *fontStyle_bold_14 =
    "QWidget{font-family:HarmonyOS Sans, Microsoft YaHei;font-weight:bold;font-size:14pt;}";
constexpr const char *fontStyle_bold_16 =
    "QWidget{font-family:HarmonyOS Sans, Microsoft YaHei;font-weight:bold;font-size:16pt;}";

// ── Header bar ──────────────────────────────────────────
constexpr const char *HeaderBar =
    "background-color:#4C70FA;";
constexpr const char *TitleStyle =
    "font-family:HarmonyOS Sans, Microsoft YaHei;font-weight:bold;font-size:16pt;color:#000000;";

// ── GroupBox ────────────────────────────────────────────
constexpr const char *GroupBoxStyle =
    "QGroupBox{border:none;border-radius:10px;background-color:#FFFFFF}"
    "QGroupBox::title{font-family:HarmonyOS Sans, Microsoft YaHei;font-weight:bold;font-size:16pt;}"
    "QLabel{background-color:transparent;}";

// ── Buttons ─────────────────────────────────────────────
// Main action buttons: Home, Stop, Move Abs, Move Rel
constexpr const char *Button_Border =
    "QPushButton{background-color:rgba(115,143,253,180);color:#FFFFFF;border-radius:5px;font-size:16px;}"
    "QPushButton:hover{border:none; background-color:rgba(115,143,253,120);color:#FFFFFF;}"
    "QPushButton:pressed{border:none; background-color:#4D70FA;color:#FFFFFF;}"
    "QPushButton:checked{border:none; background-color:#4D70FA;color:#FFFFFF;}"
    "QPushButton:disabled{border:none; background-color:#D4D4D4;color:#FFFFFF;}"
    "QLabel{background-color:transparent;}";

// Small / IO buttons: white bg + blue border
constexpr const char *Button_DIO =
    "QPushButton{background-color:#FFFFFF; border:1px solid #4D70FA; border-radius:5px;color:#4C70FA;}"
    "QPushButton:hover{border:none; background-color:#A4B7FF;color:#FFFFFF;}"
    "QPushButton:pressed{border:none; background-color:#4C70FA;color:#FFFFFF;}"
    "QPushButton:checked{border:none; background-color:#4C70FA;color:#FFFFFF;}"
    "QLabel{background-color:transparent;}";

// ── SpinBox ─────────────────────────────────────────────
constexpr const char *DoubleSpinBox_Dark =
    "QDoubleSpinBox{border-radius:2px;background-color:#F3F4FA;}"
    "QDoubleSpinBox:focus{border:1px solid #4C70FA;}"
    "QDoubleSpinBox:hover{border:1px solid #A4B7FF;}"
    "QDoubleSpinBox:disabled{background-color:#C8C8C8;}";

// Combined: DoubleSpinBox_Dark + fontStyle_normal_12
constexpr const char *SpinBox_Dark_Normal =
    "QDoubleSpinBox{border-radius:2px;background-color:#F3F4FA;}"
    "QDoubleSpinBox:focus{border:1px solid #4C70FA;}"
    "QDoubleSpinBox:hover{border:1px solid #A4B7FF;}"
    "QDoubleSpinBox:disabled{background-color:#C8C8C8;}"
    "QWidget{font-family:HarmonyOS Sans, Microsoft YaHei;font-weight:normal;font-size:12pt;}";

constexpr const char *SpinBox_Dark =
    "QSpinBox{border-radius:2px;background-color:#F3F4FA;}"
    "QSpinBox:focus{border:1px solid #4C70FA;}"
    "QSpinBox:hover{border:1px solid #A4B7FF;}"
    "QSpinBox:disabled{background-color:#C8C8C8;}";

// ── ScrollArea ──────────────────────────────────────────
constexpr const char *scrollArea =
    "QScrollArea{border-radius:4px;}"
    "QScrollBar:vertical{width:8px;background:#D8D8D8;margin:0px;padding-top:0px;padding-bottom:0px;}"
    "QScrollBar::handle:vertical{background:#4C7AFA;border-radius:4px;}"
    "QScrollBar:horizontal{height:8px;background:#D8D8D8;margin:0px;}"
    "QScrollBar::handle:horizontal{background:#4C7AFA;border-radius:4px;}"
    "QScrollBar::add-line{background:transparent;}"
    "QScrollBar::sub-line{background:transparent;}"
    "QScrollBar::add-page{background:transparent;}"
    "QScrollBar::sub-page{background:transparent;}"
    "QLabel{background-color:transparent;}";

// ── CheckBox ────────────────────────────────────────────
constexpr const char *CheckBox =
    "QCheckBox{spacing:5px;font-family:HarmonyOS Sans, Microsoft YaHei;font-weight:normal;font-size:12pt;}"
    "QCheckBox::indicator{width:20px;height:20px;}";

} // namespace StyleSheet

#endif // GUI_STYLESHEET_H
