#ifndef OPTIONWIDGET_H
#define OPTIONWIDGET_H

#include <QWidget>
#include <QBoxLayout>
#include <QScrollArea>

class OptionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OptionWidget(QWidget *parent = nullptr);
    OptionWidget(QStringList options, QMap<QString, QString> *sdkConfig, QWidget *parent = nullptr);
    void init(QStringList options, QMap<QString, QString> *sdkConfig);
signals:

public slots:

private:
    QStringList options;
    QMap<QString, QString> *sdkConfig;
    void updateOption(QString,QString);
    bool initDone = false;
    QWidget* panel;
    QScrollArea* scrollArea;
};

#endif // OPTIONWIDGET_H
