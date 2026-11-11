#ifndef RECIPECHOOSEWIDGET_H
#define RECIPECHOOSEWIDGET_H

#include <QDialog>
#include <QVBoxLayout>

class QScrollArea;

class RecipeChooseWidget : public QDialog
{
    Q_OBJECT
public:
    explicit RecipeChooseWidget(QWidget *parent = nullptr);

    inline QString GetChoose();
    void SetLst(const QStringList &lst);

signals:
    void sig_recipeChosen(const QString &name);

private:
    QWidget *m_pWgt;
    QScrollArea *m_pScroll;
    QVBoxLayout *m_pMainLayout;
    QString m_strChoose;
};

inline QString RecipeChooseWidget::GetChoose()
{
    return m_strChoose;
}

#endif // RECIPECHOOSEWIDGET_H
