#ifndef UPDATEPAGE_H
#define UPDATEPAGE_H

#include "Document.h"
#include "istar_framework_widget.h"

#include <QString>
#include <QWebEngineView>

using namespace ISTAR::Framework;

class UpdatePage : public FrameworkWidget
{
    Q_OBJECT

public:
    explicit UpdatePage(QString name, QWidget *parent = nullptr);
    ~UpdatePage();

    QWebEngineView * addUpdateContent(QString updateContentFilePath, Document& m_content);

private:
    Document m_content;
    QString m_filePath;

};

#endif // UPDATEPAGE_H
