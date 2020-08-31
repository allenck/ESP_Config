#include "components.h"
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include "exceptions.h"
#include <QDebug>

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
  if(!dir.exists())
   throw BadPath(tr("path not found: %1").arg(path));

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

 infoList = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::NoSymLinks);
 if(infoList.isEmpty())
  return;
 foreach (QFileInfo info, infoList) {
  if(info.isDir())
  {
   getFiles(info.filePath());
   if(info.fileName() == "include")
   {
    QString p = info.absoluteFilePath();
    if(!includeDirs.contains( p))
     includeDirs.append(p);
   }
  }
  else
  {
   if(!includeDirs.contains( info.absolutePath()))
    includeDirs.append(info.absolutePath());
   if(info.fileName().endsWith(".cpp") || info.fileName().endsWith(".c"))
   {
    _sources.insert(info.fileName(), info.filePath());
    if(info.fileName() == "font_render.c")
        qDebug() << "found";
    continue;
   }
   if(info.fileName().endsWith(".hpp") || info.fileName().endsWith(".h"))
   {
    _headers.insert(info.fileName(), info.filePath());
    if(!includeDirs.contains( info.absolutePath()))
       includeDirs.append(info.absolutePath());
    continue;
   }
   if(info.fileName() == "CMakeLists.txt" || info.fileName().toLower().startsWith("readme")
           || info.fileName() == "sdkconfig" || info.baseName() == "sdkconfig"
           || info.fileName() == "LICENSE")
       _otherFiles.insert(info.fileName(), info.filePath());
  }
 }
}

QMap<QString, QString> Components::sources() {return _sources;}
QMap<QString, QString> Components::headers() {return _headers;}
QMap<QString, QString> Components::otherFiles() {return _otherFiles;}

void Components::update(QString path)
{
 getFiles(path);
}


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

