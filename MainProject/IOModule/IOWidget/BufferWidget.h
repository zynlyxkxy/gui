#ifndef BUFFERWIDGET_H
#define BUFFERWIDGET_H

#include <QWidget>

namespace Ui { class BufferWidget; }

class BufferWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BufferWidget(int varId, const QString &name, QWidget *parent = nullptr);
    ~BufferWidget();

    int varId() const { return m_varId; }
    void display(bool running);

signals:
    void startBuffer(int varId);
    void stopBuffer(int varId);

private:
    Ui::BufferWidget *ui;
    int m_varId;
};

#endif // BUFFERWIDGET_H
