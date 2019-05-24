#include "tablemodel.h"


TableModel::TableModel(QList<ComponentListEntry*> *c, QObject *parent) : QAbstractTableModel(parent)
{
 this->cList = c;
}

int TableModel::columnCount(const QModelIndex &parent) const {return NUMCOLS;}
int TableModel::rowCount(const QModelIndex &parent) const {return cList->count();}
QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
 if(role == Qt::DisplayRole && orientation == Qt::Horizontal)
 {
  switch (section) {
  case NAME:
   return "Name";
  case PATH:
   return "Path";
  case EXCLUDE:
   return "Excluded";
  default:
   break;
  }
 }
 return QVariant();
}

Qt::ItemFlags TableModel::flags(const QModelIndex &index) const
{
 switch (index.column()) {
 case NAME:
  return Qt::ItemIsEnabled;
 case PATH:
  return Qt::ItemIsEnabled;
 case EXCLUDE:
  return Qt::ItemIsEnabled;
 default:
  break;
 }
 return 0;
}

QVariant TableModel::data(const QModelIndex &index, int role) const
{
 if(role == Qt::CheckStateRole && index.column()==EXCLUDE)
 {
  if(cList->at(index.row())->isExcluded())
   return Qt::Checked;
  else
   return Qt::Unchecked;
 }
 if(role == Qt::DisplayRole)
 {
  switch(index.column())
  {
   case NAME:
    return cList->at(index.row())->name();
   case PATH:
    return cList->at(index.row())->path();
  }
 }
 return QVariant();
}

