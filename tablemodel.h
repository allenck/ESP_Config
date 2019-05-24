#ifndef TABLEMODEL_H
#define TABLEMODEL_H
#include <QAbstractTableModel>
#include "components.h"

class Components;
class TableModel : public QAbstractTableModel
{
public:
 TableModel(QList<ComponentListEntry*>* c, QObject *parent = 0);
 enum COLS
 {
  NAME,
  PATH,
  EXCLUDE,
  NUMCOLS
 };
 int columnCount(const QModelIndex &parent) const;
 int rowCount(const QModelIndex &parent) const;
 QVariant headerData(int section, Qt::Orientation orientation, int role) const;
 Qt::ItemFlags flags(const QModelIndex &index) const;
 QVariant data(const QModelIndex &index, int role) const;
private:
 QList<ComponentListEntry*>* cList;
};

#endif // TABLEMODEL_H
