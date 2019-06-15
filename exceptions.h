#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H
#include <QException>

class MakeException : public QException
{
public:
 void raise() const override { throw *this; }
    MakeException *clone() const override { return new MakeException(*this); }
    MakeException(QString reason = "") {this->_reason = reason;}
    QString reason() {return _reason;}
private:
    QString _reason;
};

class BadPath : public QException
{
public:
 BadPath *clone() const override { return new BadPath(*this); }
 BadPath(QString reason = "") {this->_reason = reason;}
 QString reason() {return _reason;}
private:
 QString _reason;

};

class UnknownVariable : public QException
{
public:
 UnknownVariable *clone() const override { return new UnknownVariable(*this); }
 UnknownVariable(QString reason = "") {this->_reason = reason;}
 QString reason() {return _reason;}
private:
 QString _reason;

};
#endif // EXCEPTIONS_H
