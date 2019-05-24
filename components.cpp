#include "components.h"
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>

/* Get list of components, sources and header files */

Components::Components(QObject *parent) : QObject(parent)
{
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

  this->path = env.value("IDF_PATH", "")+ QDir::separator() + "components";
  init();
}

Components::Components(QString path, QObject *parent) : QObject(parent)
{
 this->path = path;
 init();
}

void Components::init()
{
  QDir dir = QDir(path);
  if(!dir.exists()) return;

  QFileInfoList comps = dir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot);
  foreach(QFileInfo info, comps)
  {
   ComponentListEntry* entry = new ComponentListEntry(info.baseName(), info.filePath());
   components.insert(info.baseName(), entry);
   getFiles(info.filePath());
  }
}

void Components::getFiles(QString path)
{
 QDir dir(path);
 QFileInfoList infoList;

 infoList = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
 if(infoList.isEmpty())
  return;
 foreach (QFileInfo info, infoList) {
  if(info.isDir())
  {
   getFiles(info.filePath());
  }
  else
  {
   if(info.fileName().contains(".cpp") || info.fileName().contains(".c"))
    _sources.insert(info.fileName(), info.filePath());
   if(info.fileName().contains(".hpp") || info.fileName().contains(".h"))
    _headers.insert(info.fileName(), info.filePath());
  }
 }
}
QMap<QString, QString> Components::sources() {return _sources;}
QMap<QString, QString> Components::headers() {return _headers;}


ComponentListEntry::ComponentListEntry(QString name, QString path, bool excluded)
{
 this->_name = name;
 this->_path = path;
 this->_excluded = excluded;
}

QString ComponentListEntry::name() {return _name;}
QString ComponentListEntry::path() {return _path;}
bool ComponentListEntry::isExcluded() {return _excluded;}
void ComponentListEntry::setExcluded(bool b) {this->_excluded = b;}
