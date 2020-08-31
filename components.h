#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <QObject>
#include <QMap>

class ComponentListEntry;
class Components : public QObject
{
    Q_OBJECT
public:
    explicit Components(QObject *parent = nullptr);
    Components(QString path, QObject *parent = nullptr);
    QMap<QString, QString> sources();
    QMap<QString, QString> headers();
    QMap<QString, QString> otherFiles();
    void update(QString path);
    QStringList includeDirs;

signals:

public slots:

private:
    QString path;
    QMap<QString, QString> _sources;
    QMap<QString, QString> _headers;
    QMap<QString, QString> _otherFiles;
    QMap<QString, ComponentListEntry*> components;

    void init();
    void getFiles(QString path);
};

class ComponentListEntry : public QObject
{
 Q_OBJECT
public:
 ComponentListEntry(QString name, QString path, bool excluded = true);
 QString name();
 QString path();
 bool isExcluded();
 void setExcluded(bool);
private:
 QString _name;
 QString _path;
 bool _excluded;
};
#endif // COMPONENTS_H
