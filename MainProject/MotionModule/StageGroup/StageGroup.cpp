#include "StageGroup.h"
#include "ui_StageGroup.h"
#include "RPCModule/MCTRLClientManager.h"
#include "Common/StyleSheet.h"
#include "istar_language.h"

#include <QLabel>
#include <QGridLayout>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace {
std::string fmtVec(const std::vector<double> &v)
{
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < v.size(); i++)
    {
        if (i > 0)
        {
            oss << ", ";
        }
        oss << v[i];
    }
    oss << "]";
    return oss.str();
}
}

StageGroup::StageGroup(const MctrlModule::StageInfo &info,
                       MCTRLClientManager *client,
                       QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::StageGroup)
    , m_client(client)
    , m_stageName(info.stageName)
    , m_stageType(info.stageType)
    , m_axisCount(static_cast<int>(info.axisNames.size()))
{
    ui->setupUi(this);

    btn_Home = new QPushButton();
    btn_Home->setMinimumSize(QSize(0, 45));
    btn_Stop = new QPushButton();
    btn_Stop->setMinimumSize(QSize(0, 45));
    btn_MoveRel = new QPushButton();
    btn_MoveRel->setMinimumSize(QSize(0, 45));
    btn_MoveAbs = new QPushButton();
    btn_MoveAbs->setMinimumSize(QSize(0, 45));

    applyStyles();
    setupAxisRows(info.axisNames);
    setFixedHeight(30 * m_axisCount + 210);

    connect(this, &StageGroup::sig_ObserverReceived,
            this, &StageGroup::onObserverData);
    connectButtons();

    // 初始文案按当前平台语言套用（字典未加载/无词条时保持英文 key）
    refreshTexts();
}

StageGroup::~StageGroup()
{
    delete ui;
}

void StageGroup::applyStyles()
{
    ui->groupBox->setStyleSheet(StyleSheet::GroupBoxStyle);

    ui->lbl_Decorate->setStyleSheet(StyleSheet::HeaderBar);
    ui->lbl_Title->setStyleSheet(StyleSheet::TitleStyle);

    ui->lbl_InPosName->setStyleSheet(StyleSheet::fontStyle_normal_12);
    ui->btnInPos->setStyleSheet(StyleSheet::Button_DIO);
    ui->btnInPos->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui->lbl_MovingName->setStyleSheet(StyleSheet::fontStyle_normal_12);
    ui->btnMoving->setStyleSheet(StyleSheet::Button_DIO);
    ui->btnMoving->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui->lbl_EnableName->setStyleSheet(StyleSheet::fontStyle_normal_12);
    ui->btnEnable->setStyleSheet(StyleSheet::Button_DIO);

    ui->lbl_AxisName->setStyleSheet(StyleSheet::fontStyle_normal_12);
    ui->lbl_CurrentPos->setStyleSheet(StyleSheet::fontStyle_normal_12);
    ui->lbl_CurrentSpeed->setStyleSheet(StyleSheet::fontStyle_normal_12);
    ui->lbl_Target_Abs_Pos->setStyleSheet(StyleSheet::fontStyle_normal_12);
    ui->lbl_Target_Rel_Pos->setStyleSheet(StyleSheet::fontStyle_normal_12);
    ui->Rel_Hold->setStyleSheet(StyleSheet::fontStyle_normal_12);

    btn_MoveAbs->setStyleSheet(StyleSheet::Button_Border);
    btn_MoveRel->setStyleSheet(StyleSheet::Button_Border);
    btn_Home->setStyleSheet(StyleSheet::Button_Border);
    btn_Stop->setStyleSheet(StyleSheet::Button_Border);
}

void StageGroup::setupAxisRows(const std::vector<std::string> &axisNames)
{
    QGridLayout *grid = ui->gridLayout_2;
    int i;
    int n = static_cast<int>(axisNames.size());

    for (i = 0; i < n; i++)
    {
        QLabel *lbl = new QLabel();
        lbl->setMaximumHeight(30);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet(StyleSheet::fontStyle_normal_12);
        lbl->setText(QString::fromStdString(axisNames[i]));
        grid->addWidget(lbl, i + 1, 0, 1, 1);

        NoWheelDoubleSpinBox *spb_CurrentPos = new NoWheelDoubleSpinBox();
        spb_CurrentPos->setEnabled(false);
        spb_CurrentPos->setAlignment(Qt::AlignCenter);
        spb_CurrentPos->setButtonSymbols(NoWheelDoubleSpinBox::NoButtons);
        spb_CurrentPos->setMaximumHeight(30);
        spb_CurrentPos->setStyleSheet(StyleSheet::SpinBox_Dark_Normal);
        spb_CurrentPos->setValue(0.0);
        spb_CurrentPos->setDecimals(6);
        spb_CurrentPos->setRange(-99999, 99999);

        m_dic_CurrentPos.insert(i, spb_CurrentPos);
        grid->addWidget(spb_CurrentPos, i + 1, 1, 1, 1);

        NoWheelDoubleSpinBox *spb_CurrentVel = new NoWheelDoubleSpinBox();
        spb_CurrentVel->setEnabled(false);
        spb_CurrentVel->setAlignment(Qt::AlignCenter);
        spb_CurrentVel->setButtonSymbols(NoWheelDoubleSpinBox::NoButtons);
        spb_CurrentVel->setMinimumHeight(30);
        spb_CurrentVel->setStyleSheet(StyleSheet::SpinBox_Dark_Normal);
        spb_CurrentVel->setValue(0.0);
        spb_CurrentVel->setDecimals(6);
        spb_CurrentVel->setRange(-99999, 99999);

        m_dic_CurrentVel.insert(i, spb_CurrentVel);
        grid->addWidget(spb_CurrentVel, i + 1, 2, 1, 1);

        NoWheelDoubleSpinBox *spb_TargetPos_Abs = new NoWheelDoubleSpinBox();
        spb_TargetPos_Abs->setAlignment(Qt::AlignCenter);
        spb_TargetPos_Abs->setButtonSymbols(NoWheelDoubleSpinBox::NoButtons);
        spb_TargetPos_Abs->setMinimumHeight(30);
        spb_TargetPos_Abs->setStyleSheet(StyleSheet::SpinBox_Dark_Normal);
        spb_TargetPos_Abs->setValue(0.0);
        spb_TargetPos_Abs->setDecimals(6);
        spb_TargetPos_Abs->setRange(-99999, 99999);

        m_lst_TargetAbs.append(spb_TargetPos_Abs);
        grid->addWidget(spb_TargetPos_Abs, i + 1, 3, 1, 1);

        NoWheelDoubleSpinBox *spb_TargetPos_Rel = new NoWheelDoubleSpinBox();
        spb_TargetPos_Rel->setAlignment(Qt::AlignCenter);
        spb_TargetPos_Rel->setButtonSymbols(NoWheelDoubleSpinBox::NoButtons);
        spb_TargetPos_Rel->setMinimumHeight(30);
        spb_TargetPos_Rel->setStyleSheet(StyleSheet::SpinBox_Dark_Normal);
        spb_TargetPos_Rel->setValue(0.0);
        spb_TargetPos_Rel->setDecimals(6);
        spb_TargetPos_Rel->setRange(-99999, 99999);

        m_lst_TargetRel.append(spb_TargetPos_Rel);
        grid->addWidget(spb_TargetPos_Rel, i + 1, 4, 1, 1);

        QPushButton *btn_Rel = new QPushButton();
        btn_Rel->setFixedSize(25, 25);
        btn_Rel->setStyleSheet(StyleSheet::Button_DIO);
        btn_Rel->setCheckable(true);
        grid->addWidget(btn_Rel, i + 1, 5, Qt::AlignCenter);
        grid->setRowStretch(i + 1, 1);
        m_lst_RelHold.append(btn_Rel);
    }

    grid->addWidget(btn_MoveAbs, i + 1, 3, 1, 1);
    grid->addWidget(btn_MoveRel, i + 1, 4, 1, 1);
    grid->setRowStretch(i + 1, 2);
    grid->addWidget(btn_Home, i + 1, 1, 1, 1);
    grid->addWidget(btn_Stop, i + 1, 2, 1, 1);
    grid->setRowStretch(i + 2, 2);

    // 双列布局下卡片会变宽: 让 4 个数值输入列均分多出的宽度
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);
    grid->setColumnStretch(3, 1);
    grid->setColumnStretch(4, 1);
}

void StageGroup::connectButtons()
{
    ui->btnEnable->setChecked(false);
    connect(ui->btnEnable, &QPushButton::toggled, this,
            [this](bool checked)
    {
        if (!m_client) return;
        if (checked)
        {
            std::cout << "[" << m_stageType << ":" << m_stageName << "] Enable" << std::endl;
            auto ret = m_client->getClient()->MotionCmdEnable(m_stageName);
            std::cout << "[" << m_stageType << ":" << m_stageName << "] Enable ret=0x" << std::hex << ret << std::dec << std::endl;
        }
        else
        {
            std::cout << "[" << m_stageType << ":" << m_stageName << "] Disable" << std::endl;
            auto ret = m_client->getClient()->MotionCmdDisable(m_stageName);
            std::cout << "[" << m_stageType << ":" << m_stageName << "] Disable ret=0x" << std::hex << ret << std::dec << std::endl;
        }
    });

    connect(btn_MoveAbs, &QPushButton::clicked, this, [this]()
    {
        if (!m_client) return;
        std::vector<double> positions;
        for (auto *sb : m_lst_TargetAbs)
            positions.push_back(sb->value());
        std::cout << "[" << m_stageType << ":" << m_stageName << "] MoveAbs " << fmtVec(positions) << std::endl;
        MctrlModule::StageVecDouble v;
        v.stageName = m_stageName;
        v.data = positions;
        auto ret = m_client->getClient()->MotionCmdMoveAbs(v);
        std::cout << "[" << m_stageType << ":" << m_stageName << "] MoveAbs ret=0x" << std::hex << ret << std::dec << std::endl;
    });

    connect(btn_MoveRel, &QPushButton::clicked, this, [this]()
    {
        if (!m_client) return;
        std::vector<double> deltas;
        for (auto *sb : m_lst_TargetRel)
            deltas.push_back(sb->value());
        std::cout << "[" << m_stageType << ":" << m_stageName << "] MoveRel " << fmtVec(deltas) << std::endl;
        MctrlModule::StageVecDouble v;
        v.stageName = m_stageName;
        v.data = deltas;
        auto ret = m_client->getClient()->MotionCmdMoveRel(v);
        std::cout << "[" << m_stageType << ":" << m_stageName << "] MoveRel ret=0x" << std::hex << ret << std::dec << std::endl;
        for (int i = 0; i < m_lst_RelHold.size(); i++)
        {
            if (!m_lst_RelHold[i]->isChecked())
                m_lst_TargetRel[i]->setValue(0.0);
        }
    });

    connect(btn_Home, &QPushButton::clicked, this, [this]()
    {
        if (!m_client) return;
        std::cout << "[" << m_stageType << ":" << m_stageName << "] Home" << std::endl;
        auto ret = m_client->getClient()->MotionCmdHome(m_stageName);
        std::cout << "[" << m_stageType << ":" << m_stageName << "] Home ret=0x" << std::hex << ret << std::dec << std::endl;
    });

    connect(btn_Stop, &QPushButton::clicked, this, [this]()
    {
        if (!m_client) return;
        std::cout << "[" << m_stageType << ":" << m_stageName << "] Stop" << std::endl;
        auto ret = m_client->getClient()->MotionCmdStop(m_stageName);
        std::cout << "[" << m_stageType << ":" << m_stageName << "] Stop ret=0x" << std::hex << ret << std::dec << std::endl;
    });
}

void StageGroup::onObserverData(const MctrlModule::StageObserverData &data)
{
    ui->btnEnable->blockSignals(true);
    ui->btnEnable->setChecked(data.enable);
    ui->btnEnable->blockSignals(false);

    for (int i = 0; i < static_cast<int>(data.pos.size()) && i < m_axisCount; i++)
    {
        if (m_dic_CurrentPos.contains(i))
            m_dic_CurrentPos[i]->setValue(data.pos[i]);
    }

    for (int i = 0; i < static_cast<int>(data.vel.size()) && i < m_axisCount; i++)
    {
        if (m_dic_CurrentVel.contains(i))
            m_dic_CurrentVel[i]->setValue(data.vel[i]);
    }

    ui->btnInPos->setChecked(data.inPos);
    ui->btnMoving->setChecked(data.moving);
}

void StageGroup::refreshTexts()
{
    // 按钮（key 固定英文，词典无词条时 Tr 原样返回英文）
    btn_Home->setText(TR_STR("Home"));
    btn_Stop->setText(TR_STR("Stop"));
    btn_MoveRel->setText(TR_STR("Move Rel"));
    btn_MoveAbs->setText(TR_STR("Move Abs"));

    // .ui 静态文案
    ui->lbl_InPosName->setText(TR_STR("InPos"));
    ui->lbl_MovingName->setText(TR_STR("Moving"));
    ui->lbl_EnableName->setText(TR_STR("Enable"));
    ui->lbl_AxisName->setText(TR_STR("AxisName"));
    ui->lbl_CurrentPos->setText(TR_STR("CurPos"));
    ui->lbl_CurrentSpeed->setText(TR_STR("Speed"));
    ui->lbl_Target_Abs_Pos->setText(TR_STR("TargetPos(Abs)"));
    ui->lbl_Target_Rel_Pos->setText(TR_STR("TargetPos(Rel)"));
    ui->Rel_Hold->setText(TR_STR("RelHold"));

    // 卡片标题：stageName 词典有词条（如 AlignStage→对准台）则翻译，否则保持原名
    ui->lbl_Title->setText(TR_STR(m_stageName));
}

