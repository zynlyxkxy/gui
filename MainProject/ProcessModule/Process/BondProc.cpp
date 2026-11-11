#include "BondProc.h"

#include "ui_ProcessPage.h"

#include "Common/BondCurveRecorder.h"
#include "ProcessModule/Protocol/ProcessManagerProtocol.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QGridLayout>
#include <QHeaderView>

namespace {

// 参数列数下限 = 7（键合 11 条命令里参数最多的是 ThermostaticPosCtl），含 Command 共 8 列。
// 与处方编辑器（module/re BondingTab 的 kBondingDefaultParamCount）的列方案保持一致
constexpr int kBondingDefaultParamCount = 7;

}  // namespace

BondProc::BondProc(int id, QWidget *tabPage, Ui::ProcessPage *ui, QWidget *dialogParent)
    : ProcessBase(id, tabPage, ui, dialogParent)
    , m_table(ui->tw_bondingProcess)
{
    // 分组框标题：英文原稿声明（语言切换由 UiTranslator 处理）
    ui->gb_bondingProcess->SetHeaderText(QStringLiteral("Bond Process"));
    ui->gb_operateStepDesc_9->SetHeaderText(QStringLiteral("Operation Steps"));
    ui->gb_result_9->SetHeaderText(QStringLiteral("Result"));

    // 工序表初始就是下限列数（含 Command 共 8 列）+ 列宽按内容自适应；换处方后由
    // bonding_loadUi() 按处方重算列数并再设一次，保证每次重建列后该模式仍然生效
    ui->tw_bondingProcess->setColumnCount(qMax(ui->tw_bondingProcess->columnCount(),
                                               kBondingDefaultParamCount + 1));
    ui->tw_bondingProcess->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tw_bondingProcess->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->dsb_maxTargetForce->setReadOnly(true);
    ui->dsb_maxTargetTemp->setReadOnly(true);
    ui->dsb_runElapsedTime_bond->setReadOnly(true);

    ensureChart();   // 空数据也有坐标轴

    // 曲线采集器：父对象用本 tab 页（ProcessPage 注入，随 tab 销毁）
    m_curve = new BondCurveRecorder(p_Chart, m_tabPage);

    setConnect();
}

void BondProc::setConnect()
{
    // 曲线纵轴显隐：勾选框 → 采集器（它记住状态，重设量程时会把隐藏的轴补回去）
    QObject::connect(ui->ckb_upChuckTemperature, &QCheckBox::toggled, m_dialogParent, [this](bool b)
    {
        m_curve->setCurveVisible(QString::fromLatin1(BondCurve::kAxisUpTemp), b);
    });
    QObject::connect(ui->ckb_downChuckTemperature, &QCheckBox::toggled, m_dialogParent, [this](bool b)
    {
        m_curve->setCurveVisible(QString::fromLatin1(BondCurve::kAxisDownTemp), b);
    });
    QObject::connect(ui->ckb_bondForce, &QCheckBox::toggled, m_dialogParent, [this](bool b)
    {
        m_curve->setCurveVisible(QString::fromLatin1(BondCurve::kAxisForce), b);
    });
}

void BondProc::clearData()
{
    // 只清曲线（表格是处方数据的展示，不清）。
    // 时机：ProcessPage 在发 Start 之前调用本函数 = "新一轮开始"，所以顺带开一段
    // 新的键合曲线采集（采集器内部先清图再开窗，清图行为不变）。
    m_table.clearHighlight();   // 高亮是本轮的运行态：新一轮从"哪条都不亮"开始
    if (m_curve != nullptr)
    {
        m_curve->startSession();
    }
    else if (p_Chart != nullptr)
    {
        p_Chart->ClearView();
        p_Chart->replot();
    }
}

void BondProc::updateRecipe()
{
    bonding_loadUi(m_rcpChip.bond);
}

void BondProc::ReceiveMctrlCommand(const ProcessMessage &data)
{
    switch (data.command)
    {
        // 本流程完成 → 键合曲线收尾（数据定格，不清图）
        case ProcessManagerCMD_SingleProcFinished:
            if (m_curve != nullptr)
            {
                m_curve->finishSession();
            }
            break;

        // 即将执行第 N 条工艺 → 该工序的两行高亮
        case BondCMD_StepBegin:
        {
            const S_BondCMDStepBeginData step =
                    Serializer::from_rawJson<S_BondCMDStepBeginData>(data.json);
            highlightStep(step.stepNo);
        }
        break;

        default:
            break;
    }
}

void BondProc::highlightStep(int stepNo)
{
    // 控制层给的是 0 基的工序序号（= 处方 lst_BondingSequence 下标），而表里一条工序占
    // 两行（命令/参数名一行 + 值一行，见 bonding_loadUi），所以第 N 条 = 第 2N / 2N+1 行。
    m_table.clearHighlight();   // 一条工序一段：先撤掉上一条的，只留当前这条
    if (stepNo < 0)
    {
        return;
    }
    const int row = stepNo * 2;
    if (row + 1 >= m_table.rows())
    {
        return;   // 序号对不上当前处方（换过处方 / 越界）：表格一个格子都不动
    }
    m_table.highlightRow(row);
    m_table.highlightRow(row + 1);
}

void BondProc::bonding_loadUi(const S_RecipeChipBond &bond)
{
    // 每条工序两行：表头行（命令名 + 参数名），数据行（空 + 参数值）。
    // 有啥填啥：值均为处方原文（check=Checked/Unchecked、combo=选项名/序号、spin=数字文本），
    // 不做 Yes/No 等加工；表头行的命令/参数名挂翻译 key，切语言时自动重翻。
    m_table.removeAllRows();

    // 列数按本处方重算，参数列数下限 = kBondingDefaultParamCount（7）→ 含 Command 共 8 列：
    // 步骤参数不足 7 个（如只有 5 个）时表格照样显示 8 列，参数多于 7 个的命令则照常扩列。
    // 行已清空，缩小列不会丢数据；列数/表头在此重建，下面把列宽模式一并重设
    int maxParamCount = 0;
    for (const S_BondStep &step : bond.lst_BondingSequence)
    {
        maxParamCount = qMax(maxParamCount, int(step.params.size()));
    }
    ui->tw_bondingProcess->setColumnCount(qMax(maxParamCount, kBondingDefaultParamCount) + 1);

    // 顶部最大目标压力/温度取自处方标量（只读展示）：无处方数据时结构体默认为 0，避免残留旧值
    ui->dsb_maxTargetForce->setValue(bond.maxTargetForce);
    ui->dsb_maxTargetTemp->setValue(bond.maxTargetTemp);

    for (const S_BondStep &step : bond.lst_BondingSequence)
    {
        const int base = m_table.rows();
        m_table.setCellText(base, 0, step.command);
        m_table.setCellTrKey(base, 0, step.command);
        m_table.setCellText(base + 1, 0, std::string());
        for (int p = 0; p < int(step.params.size()); ++p)
        {
            m_table.setCellText(base, p + 1, step.params[p].key);
            m_table.setCellTrKey(base, p + 1, step.params[p].key);
            m_table.setCellText(base + 1, p + 1, step.params[p].value);
            m_table.setCellTrKey(base + 1, p + 1, step.params[p].value);   // 值也有英文/中文混存，同样挂 key
        }
    }

    // 列宽按内容自适应：改列数会重建表头项，故每次重建列/写完内容后重新应用，
    // 保证列数变化（本函数每次调用都会重算列数）后该模式依然生效（需 <QHeaderView>）
    ui->tw_bondingProcess->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void BondProc::ensureChart()
{
    if (p_Chart != nullptr)
    {
        return;
    }
    p_Chart = new ISTAR::GUI::CustomChart(ui->frm_Plot);
    p_Chart->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables | QCP::iSelectAxes);
    QGridLayout *gly = qobject_cast<QGridLayout *>(ui->frm_Plot->layout());
    if (gly == nullptr)
    {
        gly = new QGridLayout(ui->frm_Plot);
    }
    gly->addWidget(p_Chart);
    configureAxes();
    p_Chart->replot();
}

void BondProc::configureAxes()
{
    // 量程策略与轴定义都在 BondCurve::applyAxisRange() 里（mode=0 固定基线；
    // 数据超过基线时由 BondCurveRecorder 再调一次把量程扩出去）。
    BondCurve::applyAxisRange(p_Chart, BondCurve::kTempAxisBaselineMax, BondCurve::kForceAxisBaselineMax);
}
