#ifndef SDKCONFIGDATAMODEL_H
#define SDKCONFIGDATAMODEL_H
#include <QAbstractTableModel>

class SdkConfigDataModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    SdkConfigDataModel(QMap<QString, QString> *sdkconfig, QObject *parent = nullptr);
    enum COLS
    {
        NAME,
        VALUE
    };
private:
    QMap<QString, QString>* sdkconfig = nullptr;
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
};

#endif // SDKCONFIGDATAMODEL_H
