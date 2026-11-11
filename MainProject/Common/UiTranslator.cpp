#include "UiTranslator.h"

#include "istar_language.h"   // Tr()
#include "chart/qcustomplot.h"

#include <QAbstractButton>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QWidget>

namespace {

// 纯 ASCII 才可能是英文原稿（已翻译的中文/运行数据的文本不捕获，防误存 key）
bool asciiOnly(const QString &s)
{
    for (const QChar &ch : s)
    {
        if (ch.unicode() > 127)
        {
            return false;
        }
    }
    return true;
}

QString trOf(const QString &key)
{
    return QString::fromStdString(Tr(key.toStdString()));
}

}  // namespace

void UiTranslator::refresh(QWidget *root)
{
    if (root == nullptr)
    {
        return;
    }

    // 1) tab 标题（key 缓存于 "trTabKeys"，顺序与 tab 下标一一对应）
    for (QTabWidget *tw : root->findChildren<QTabWidget *>())
    {
        QStringList keys = tw->property("trTabKeys").toStringList();
        bool changed = false;
        for (int i = 0; i < tw->count(); ++i)
        {
            QString key = (i < keys.size()) ? keys.at(i) : QString();
            if (key.isEmpty())
            {
                const QString t = tw->tabText(i);
                if (t.isEmpty() || !asciiOnly(t))
                {
                    continue;
                }
                key = t;
                while (keys.size() <= i)
                {
                    keys.append(QString());
                }
                keys[i] = t;
                changed = true;
            }
            tw->setTabText(i, trOf(key));
        }
        if (changed)
        {
            tw->setProperty("trTabKeys", keys);
        }
    }

    // 2) 按钮文本（QAbstractButton 统一覆盖 QPushButton / QCheckBox / EffectButton；
    //    GroupBox 内部标题 lbl_Title 是 QLabel，由下面的标签循环统一处理）
    for (QAbstractButton *b : root->findChildren<QAbstractButton *>())
    {
        QString key = b->property("trKey").toString();
        if (key.isEmpty())
        {
            const QString t = b->text();
            if (t.isEmpty() || !asciiOnly(t))
            {
                continue;
            }
            key = t;
            b->setProperty("trKey", t);
        }
        b->setText(trOf(key));
    }

    // 3) 标签文本（含各 GroupBox 内部 lbl_Title：构造函数里 SetHeaderText("英文原稿")
    //    声明的标题在此捕获 key 并翻译）
    for (QLabel *l : root->findChildren<QLabel *>())
    {
        QString key = l->property("trKey").toString();
        if (key.isEmpty())
        {
            const QString t = l->text();
            if (t.isEmpty() || !asciiOnly(t))
            {
                continue;
            }
            key = t;
            l->setProperty("trKey", t);
        }
        l->setText(trOf(key));
    }

    // 4) 表格：表头列（ASCII 懒惰捕获，缓存于 "trHeaderKeys"）
    //    + 行头（竖表头，同上，缓存于 "trRowHeaderKeys"）
    //    + 单元格（仅 TrKeyRole 存有 key 的才翻译，数据格天然安全）
    for (QTableWidget *tw : root->findChildren<QTableWidget *>())
    {
        QStringList keys = tw->property("trHeaderKeys").toStringList();
        bool changed = false;
        for (int c = 0; c < tw->columnCount(); ++c)
        {
            QTableWidgetItem *h = tw->horizontalHeaderItem(c);
            if (!h)
            {
                continue;
            }
            QString key = (c < keys.size()) ? keys.at(c) : QString();
            if (key.isEmpty())
            {
                const QString t = h->text();
                if (t.isEmpty() || !asciiOnly(t))
                {
                    continue;
                }
                key = t;
                while (keys.size() <= c)
                {
                    keys.append(QString());
                }
                keys[c] = t;
                changed = true;
            }
            h->setText(trOf(key));
        }
        if (changed)
        {
            tw->setProperty("trHeaderKeys", keys);
        }

        // 行头（竖表头）与列头同样处理，缓存于 "trRowHeaderKeys"
        QStringList rowKeys = tw->property("trRowHeaderKeys").toStringList();
        bool rowChanged = false;
        for (int r = 0; r < tw->rowCount(); ++r)
        {
            QTableWidgetItem *h = tw->verticalHeaderItem(r);
            if (!h)
            {
                continue;
            }
            QString key = (r < rowKeys.size()) ? rowKeys.at(r) : QString();
            if (key.isEmpty())
            {
                const QString t = h->text();
                if (t.isEmpty() || !asciiOnly(t))
                {
                    continue;
                }
                key = t;
                while (rowKeys.size() <= r)
                {
                    rowKeys.append(QString());
                }
                rowKeys[r] = t;
                rowChanged = true;
            }
            h->setText(trOf(key));
        }
        if (rowChanged)
        {
            tw->setProperty("trRowHeaderKeys", rowKeys);
        }

        for (int r = 0; r < tw->rowCount(); ++r)
        {
            for (int c = 0; c < tw->columnCount(); ++c)
            {
                QTableWidgetItem *it = tw->item(r, c);
                if (!it)
                {
                    continue;
                }
                const QString key = it->data(TrKeyRole).toString();
                if (!key.isEmpty())
                {
                    it->setText(trOf(key));
                }
            }
        }
    }

    // 5) 输入框占位文本（key 缓存于 "trPlaceholderKey"）
    for (QLineEdit *le : root->findChildren<QLineEdit *>())
    {
        QString key = le->property("trPlaceholderKey").toString();
        if (key.isEmpty())
        {
            const QString t = le->placeholderText();
            if (t.isEmpty() || !asciiOnly(t))
            {
                continue;
            }
            key = t;
            le->setProperty("trPlaceholderKey", t);
        }
        le->setPlaceholderText(trOf(key));
    }

    // 6) 曲线图（QCustomPlot 及其子类：CustomChart / AdvanceCustomChart …）
    //    轴标签形如 "Up Chuck Temperature (℃)"：取「名字」部分当 key、" (℃)" 后缀原样保留，
    //    key 缓存于轴对象（QCPAxis 是 QObject，可挂动态属性），规则与控件文本一致（纯 ASCII 才捕获）。
    //    ⚠ 只改显示用的 label，不动库里的 AxisParameter.name —— name 是曲线 key
    //    （AddAxisData / SetAxisYVisible 按它匹配），翻译它会把数据链路弄断。
    for (QCustomPlot *chart : root->findChildren<QCustomPlot *>())
    {
        bool changed = false;
        for (int r = 0; r < chart->axisRectCount(); ++r)
        {
            for (QCPAxis *ax : chart->axisRect(r)->axes())
            {
                QString key = ax->property("trAxisKey").toString();
                QString tail = ax->property("trAxisTail").toString();

                // 自愈：首次捕获发生在库补单位之前时，缓存的后缀是空的；之后库把 label 写成了
                // "Time(s)" 这种带单位的形态，这里就把后缀补回来（否则中文永远只有"时间"，丢掉 (s)）。
                // 只在英文形态（label 以 key 开头）下命中；翻译成中文后 label 不再以英文 key 开头，不会误判。
                if (!key.isEmpty())
                {
                    const QString lbl = ax->label();
                    if (lbl.startsWith(key) && lbl.size() > key.size() + tail.size())
                    {
                        tail = lbl.mid(key.size());
                        ax->setProperty("trAxisTail", tail);
                    }
                }
                if (key.isEmpty())
                {
                    const QString lbl = ax->label();
                    if (lbl.isEmpty())
                    {
                        continue;
                    }
                    // 拆 "名字(单位)"：库（CustomChart）生成的 label 没有空格，形如
                    // "Up Chuck Temperature(℃)" / "Time(s)"；半角（全角（ 也认，
                    // 括号前的空格一并算进后缀，所以 "名字 (单位)" 这种写法同样能拆对。
                    // 单位不是词典 key，原样接回后缀。
                    int cut = -1;
                    for (int i = lbl.size() - 1; i >= 0; --i)
                    {
                        const QChar ch = lbl.at(i);
                        if (ch == QLatin1Char('(') || ch.unicode() == 0xFF08)
                        {
                            cut = i;
                            break;
                        }
                    }
                    QString name = lbl;
                    // 注意：后缀用外层那个 tail（第 228 行从属性读的），这里不要再声明同名变量，
                    // 否则会遮蔽外层，导致属性里存了单位、拼文本时却用了空的后缀
                    if (cut > 0)
                    {
                        int s = cut;
                        while (s > 0 && lbl.at(s - 1) == QLatin1Char(' '))
                        {
                            --s;
                        }
                        name = lbl.left(s);
                        tail = lbl.mid(s);
                    }
                    if (!asciiOnly(name))
                    {
                        continue;          // 非 ASCII 不当 key（和控件文本同一规则）
                    }
                    key = name;
                    ax->setProperty("trAxisKey", key);
                    ax->setProperty("trAxisTail", tail);
                }

                const QString text = trOf(key) + tail;
                if (ax->label() != text)
                {
                    ax->setLabel(text);
                    changed = true;
                }
            }
        }
        if (changed)
        {
            chart->replot(QCustomPlot::rpQueuedReplot);   // QCustomPlot 改了 label 必须 replot 才刷新
        }
    }

    // 7) 自绘图例（DiagramBase 的 LegendBox）：约定控件挂动态属性 "trLegendKeys"
    //    （QStringList，内容 = 各图例项的英文原稿 key）；这里逐 key 翻译后写回
    //    "trLegendTexts" 并触发重画 —— 图例 paint 只读译文，翻译逻辑全部收在本模组
    //    （与 table 的 TrKeyRole 同一套思路：组件挂 key，模组出译文）
    for (QWidget *w : root->findChildren<QWidget *>())
    {
        const QStringList keys = w->property("trLegendKeys").toStringList();
        if (keys.isEmpty())
        {
            continue;
        }
        QStringList texts;
        for (const QString &k : keys)
        {
            texts << trOf(k);
        }
        w->setProperty("trLegendTexts", texts);
        w->update();
    }

}
