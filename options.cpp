#include "options.h"

Options::Options(QString name, QString value, QObject *parent) : QObject(parent)
{
 _name = name;
 _value = value;
}

QString Options::name() {return _name;}
QString Options::value() {return _value;}
void Options::setValue(QString sval)
{
    _value = sval;
}
