#ifndef OPTIONS_H
#define OPTIONS_H

#include <QObject>

class Options : public QObject
{
    Q_OBJECT
public:
    explicit Options(QString name, QString value, QObject *parent = nullptr);
    QString name();
    QString value();
    void setValue(QString sval);
signals:

public slots:
private:
    QString _name;
    QString _value;
};

#endif // OPTIONS_H
