#include "sdkconfigdatamodel.h"

SdkConfigDataModel::SdkConfigDataModel(QMap<QString, QString> *sdkconfig, QObject* parent)
{
    this->sdkconfig = sdkconfig;
}

int SdkConfigDataModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}

int SdkConfigDataModel::rowCount(const QModelIndex &parent) const
{
    return sdkconfig->count();
}

QVariant SdkConfigDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section) {
        case NAME:
            return tr("Option");
        case VALUE:
            return tr("Value");
        default:
            break;
        break;
        }
    }
    return QVariant();
}

Qt::ItemFlags SdkConfigDataModel::flags(const QModelIndex &index) const
{
    switch (index.column()) {
    case NAME:
    {
        QString key = sdkconfig->keys().at(index.row());
        if(!sdkconfig->value(key).isEmpty())
            return Qt::ItemIsEnabled;
        break;
    }
    case VALUE:
        return Qt::ItemIsEnabled | Qt::ItemIsEditable;
        break;
    default:
        break;
    }
}

QVariant SdkConfigDataModel::data(const QModelIndex &index, int role) const
{

    if(role == Qt::DisplayRole)
    {
        switch (index.column()) {
        case NAME:
            return sdkconfig->keys().at(index.row());
        case VALUE:
            return sdkconfig->values().at(index.row());
        default:
            break;
        }
    }
    else if(role == Qt::ToolTipRole && index.column() == 1)
        return tr("enter: 'y', number or quoted string; space to disable.");
    return QVariant();
}

bool SdkConfigDataModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(role== Qt::EditRole)
    {
      QString str = value.toString().trimmed();
      sdkconfig->insert(sdkconfig->keys().at(index.row()), value.toString());

    }
    return false;
}
