#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "components.h"
#include <QFileDialog>
#include <QTextStream>
#include <QDebug>
#include <QProcessEnvironment>
#include "tablemodel.h"
#include <QtXml>
#include <QUuid>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    espComponents = new Components();

    ui->actionMakefile->setToolTip(tr("Specify the project's Makefile location"));
    connect(ui->actionMakefile, SIGNAL(triggered(bool)), this, SLOT(onMakefile()));
    ui->actionSave->setToolTip(tr("Save Pro file"));
    connect(ui->actionSave, SIGNAL(triggered(bool)), this, SLOT(writeProFile()));
    ui->actionHeaders->setToolTip("View headers");
    connect(ui->actionHeaders, SIGNAL(triggered(bool)), this, SLOT(viewHeaders()));
    ui->actionSources->setToolTip("View source files");
    connect(ui->actionSources, SIGNAL(triggered(bool)), this, SLOT(viewSources()));
    ui->actionCreate_User_file->setToolTip("Create user file");
    connect(ui->actionCreate_User_file, SIGNAL(triggered(bool)), this, SLOT(createUserFile()));
    connect(ui->actionExit, SIGNAL(triggered(bool)), this, SLOT(onExit()));

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    toolChainPath = env.value("Home", "")+ QDir::separator() +"esp/xtensa-esp32-elf" +QDir::separator();
    idf_path = env.value("IDF_PATH");
    toolChainPaths = QStringList();
    toolChainPaths << toolChainPath + "xtensa-esp32-elf/include"
                   << toolChainPath + "toolChainPath + xtensa-esp32-elf/include/c++/5.2.0"
                   << toolChainPath + "xtensa-esp32-elf/include/c++/5.2.0/xtensa-esp32-elf"
                   << toolChainPath + "lib/gcc/xtensa-esp32-elf/5.2.0/include"
                   << toolChainPath + "lib/gcc/xtensa-esp32-elf/5.2.0/include-fixed";
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::parseMakefile(QString path)
{
 QFileInfo info(path);
 QDir dir(info.absolutePath());
 while(!dir.path().isEmpty() )
 {
  QStringList list = dir.entryList();

  if(list.contains("components"))
  {
   projComponents = new Components(dir.absolutePath());
   return true;
  }
  dir.cdUp();
 }
 return false;
}

bool MainWindow::onMakefile()
{
 QFileDialog* dialog = new QFileDialog();
 dialog->setFileMode(QFileDialog::ExistingFile);
 dialog->setNameFilter("Makefile");
 QStringList fileNames;
 dialog->setToolTip(tr("Select the Makefile for the project"));
 if (dialog->exec())
     fileNames = dialog->selectedFiles();
 if(fileNames.isEmpty())
  return false;

 QFileInfo info(fileNames.at(0));

 components = new Components(info.path());

 QFile data(fileNames.at(0));
 parseMakefile(fileNames.at(0));
 if(data.open(QFile::ReadOnly))
 {
  QFileInfo info(fileNames.at(0));
  QTextStream stream(&data);
  QString line;
  while (stream.readLineInto(&line))
  {
   if(line.contains(":="))
   {
    QStringList sl = line.split(":=");
    if(sl.count()==2)
    {
     if(sl.at(0).trimmed() == QString("PROJECT_NAME"))
     {
      name = sl.at(1);
     }
     else if(sl.at(0).trimmed() == QString("a") && sl.at(1) == QString("$(shell pwd)"))
     {
      pwd = info.absolutePath();
      QDir pwdDir(pwd);
      target= pwdDir.dirName();
     }
     else if((sl.at(0).trimmed() == QString("b")) && (sl.at(1) == QString("$(dir $(patsubst %/,%,$(dir $(a))))")))
     {
      QDir dir(pwd);
      componentDir = "../"+dir.dirName();

     }
     else if(sl.at(0).trimmed() == "EXCLUDE_COMPONENTS")
     {

     }
    }
   }
  }
  QMapIterator<QString, QString> iter(components->sources());
  while(iter.hasNext())
  {
   iter.next();
   QString key = iter.key();
   if(key.endsWith(".c") || key.endsWith(".cpp"))
   {
    QString path = iter.value();
    parseSourceFile(path);
   }
  }
 }
 viewHeaders();
 _dirty = true;
 return true;
}

bool MainWindow::parseSourceFile(QString path)
{
 QFileInfo info(path);
 qDebug() << "parse path: " << path;
 QFile data(path);
 if(data.open(QFile::ReadOnly))
 {
  QTextStream stream(&data);
  QString line;
  QString header;
  while (stream.readLineInto(&line))
  {
   if(line.contains("#include"))
   {
    if(line.contains("<"))
    {
     if(line.indexOf(">") > line.indexOf("<"))
     {
      header = line.mid(line.indexOf("<")+1, line.indexOf(">") - line.indexOf("<")-1);
      qDebug() << "source file: " << info.fileName() << " includes: " << header;
      if(!headers.contains(header))
      {
       QString path = headerFilePath(header);
       if(path != "")
        headers.insert(header, path);
        //parseSourceFile(path + QDir::separator() + header);
        bool bFound = false;
        QStringList list = components->headers().values();
        foreach(QString p, list)
        {
         if(p.endsWith(header))
         {
          parseSourceFile(p);
          bFound = true;
         }
        }
        if(!bFound)
        {
         list = projComponents->headers().values();
         foreach(QString p, list)
         {
          if(p.endsWith(header))
          {
           parseSourceFile(p);
           bFound = true;
          }
         }
        }
        if(!bFound)
        {
         list = espComponents->headers().values();
         foreach(QString p, list)
         {
          if(p.endsWith(header))
          {
           parseSourceFile(p);
           bFound = true;
          }
         }
        }
      }
     }
    }
    if(line.contains("\""))
    {
     if(line.lastIndexOf("\"") > line.indexOf("\""))
     {
      header = line.mid(line.indexOf("\"")+1, line.lastIndexOf("\"") - line.indexOf("\"")-1);
      qDebug() << "source file: " << info.fileName() << " includes: " << header;
      if(!headers.contains(header))
      {
       QString path = headerFilePath(header);
       if(path != "")
        headers.insert(header, path);
        //parseSourceFile(path + QDir::separator() + header);
        bool bFound = false;
        QStringList list = components->headers().values();
        foreach(QString p, list)
        {
         if(p.endsWith(header))
         {
          parseSourceFile(p);
          bFound = true;
         }
        }
        if(!bFound)
        {
         list = projComponents->headers().values();
         foreach(QString p, list)
         {
          if(p.endsWith(header))
          {
           parseSourceFile(p);
           bFound = true;
          }
         }
        }
        if(!bFound)
        {
         list = espComponents->headers().values();
         foreach(QString p, list)
         {
          if(p.endsWith(header))
          {
           parseSourceFile(p);
           bFound = true;
          }
         }
        }
      }
     }
    }
   }
  }
  data.close();
 }
 else
 {
  qDebug() << path << " file not found";
 }
 return true;
}

QString MainWindow::headerFilePath(QString inName)
{
 if(inName == "freertos/FreeRTOS.h")
  qDebug() << inName;
 QString name = inName;
 int ix=name.lastIndexOf(QDir::separator());
 if(ix != -1)
  name = name.mid(ix+1);
 if(components->headers().contains(name))
 {
  QString path = components->headers().value(name);
  QFileInfo info(path);
  if(!includePaths.contains(info.absolutePath()))
  {
   includePaths.append(info.absolutePath());
   qDebug() << "add " << info.absolutePath() << " to includePaths";
  }
  return info.absolutePath();
 }
 if(projComponents->headers().contains(name))
 {
  QString path = projComponents->headers().value(name);
  QFileInfo info(path);
  if(!includePaths.contains(info.absolutePath()))
  {
   includePaths.append(info.absolutePath());
   qDebug() << "add " << info.absolutePath() << " to includePaths";
  }
  return info.absolutePath();
 }
 if(espComponents->headers().contains(name))
 {
  QString path = espComponents->headers().value(name);
  if(ix == -1)
  {
   QFileInfo info(path);
   if(!includePaths.contains(info.absolutePath()))
   {
    includePaths.append(info.absolutePath());
    qDebug() << "add " << info.absolutePath() << " to includePaths";
   }
   return info.absolutePath();
  }
  else // relative path specified
  {
   if(path.contains(inName))
   {
    QString absPath = path.replace(QDir::separator()+inName, "");
    if(!includePaths.contains(absPath))
    {
     includePaths.append(absPath);
     qDebug() << "add " << absPath << " to includePaths";
    }
    return absPath;
   }
  }
 }
 qDebug() << name << " not found";
 return "";
}

bool MainWindow::writeProFile()
{
 QString filename = pwd + QDir::separator() + target + ".pro";
 QFile file(filename);
 if(file.open(QFile::WriteOnly | QFile::Truncate))
 {
  QTextStream out(&file);
  out << "\tTARGET = " << target << "\n";
  out << "\n\n";
  out << "IDF_PATH =" << idf_path;
  out << "\n\n";
  out << "\tINCLUDEPATH += ";
  QDir dir(pwd);
  qSort(includePaths.begin(), includePaths.end() );
  foreach(QString p, includePaths)
  {
   if(p.startsWith(idf_path))
   {
    out << "\\\n\t\t\t";
    out << "$${IDF_PATH}" << p.mid(idf_path.length()) << " ";
   }
  }
  out << "\n\n";
  out << "\tXTENSA_TOOLCHAIN += $$(HOME)/esp/xtensa-esp32-elf\n";
  out << "\tINCLUDEPATH += \\\n\t\t\t$${XTENSA_TOOLCHAIN}/xtensa-esp32-elf/include \\\n";
  out << "\t\t\t$${XTENSA_TOOLCHAIN}/xtensa-esp32-elf/include/c++/5.2.0 \\\n";
  out << "\t\t\t$${XTENSA_TOOLCHAIN}/xtensa-esp32-elf/include/c++/5.2.0/xtensa-esp32-elf \\\n";
  out << "\t\t\t$${XTENSA_TOOLCHAIN}/lib/gcc/xtensa-esp32-elf/5.2.0/include \\\n";
  out << "\t\t\t$${XTENSA_TOOLCHAIN}/lib/gcc/xtensa-esp32-elf/5.2.0/include-fixed \\\n";
  out << "\t\t\t$${XTENSA_TOOLCHAIN}/lib/gcc/xtensa-esp32-elf/include/sys \n";
  out << "\n";

  out << "\tINCLUDEPATH += ";
  foreach(QString p, includePaths)
  {
   if(!p.startsWith(idf_path))
   {
    out << "\\\n\t\t\t";
    out << dir.relativeFilePath(p) << " ";
   }
  }
  out << "\n\n";
  out << "\tSOURCES += ";
  foreach(QString src, components->sources().values())
  {
   out << "\\\n\t\t\t";
   out << dir.relativeFilePath(src) << " ";
  }
  foreach(QString src, projComponents->sources().values())
  {
   out << "\\\n\t\t\t";
   out << dir.relativeFilePath(src) << " ";
  }
  out << "\n\n";
  file.close();
  _dirty = false;
 }
}

void MainWindow::viewHeaders()
{
 QList<ComponentListEntry*>* cList = new QList<ComponentListEntry*>();
 foreach (QString s, components->headers().keys())
 {
  ComponentListEntry* entry = new ComponentListEntry(s, components->headers().value(s), false);
  cList->append(entry);
 }
 ui->tv1->setModel(new TableModel(cList));
 ui->tv1->resizeColumnsToContents();
}

void MainWindow::viewSources()
{
 QList<ComponentListEntry*>* cList = new QList<ComponentListEntry*>();
 foreach (QString s, components->sources().keys()) {
  cList->append(new ComponentListEntry(s, components->sources().value(s), false));
 }
 ui->tv1->setModel(new TableModel(cList));
 ui->tv1->resizeColumnsToContents();

}

bool MainWindow::writeUserFile(QString fileName)
{
 QDomDocument doc;
 QFile file(":/resources/user.pro");
 if(file.open(QIODevice::ReadOnly))
 {
  QTextStream stream(&file);
  QString content = stream.readAll();
  if(content.contains("*REPLACE_BUILD_DIR*"))
  {
   content = content.replace("*REPLACE_BUILD_DIR*", pwd);
   qDebug() << "replace *REPLACE_BUILD_DIR* with " << pwd;
  }
  if(content.contains("*REPLACE_IDF_PATH*"))
  {
   content = content.replace("*REPLACE_IDF_PATH*", idf_path);
   qDebug() << "replace *REPLACE_IDF_PATH* with " << idf_path;
  }
  if(content.contains("*REPLACE_PRO_PATH*"))
  {
   content = content.replace("*REPLACE_PRO_PATH*", pwd + QDir::separator() + (target + ".pro"));
   qDebug() << "replace *REPLACE_PRO_PATH* with " << (pwd + QDir::separator() + (target + ".pro"));
  }
  if(content.contains("*REPLACE_PRO_FN*"))
  {
   content = content.replace("*REPLACE_PRO_FN*", (target + ".pro"));
   qDebug() << "replace *REPLACE_PRO_FN* with " << (target + ".pro");
  }
  if(content.contains("*REPLACE_WORKING_DIR*"))
  {
   content = content.replace("*REPLACE_WORKING_DIR*", pwd);
   qDebug() << "replace *REPLACE_WORKING_DIR* with " << pwd;
  }
  doc.setContent(content);
  QDomElement root = doc.documentElement();
  qDebug() << "root = " << root.tagName();
  QDomNodeList nl = root.childNodes();
  qDebug() << "root has " << nl.count() << " nodes";
  for(int i=0; i < nl.count(); i++)
  {
   QDomElement elem = nl.at(i).toElement();
   qDebug() << i << " " << elem.tagName();
   QDomElement e =elem.firstChildElement("variable");
   if(!e.isNull() && e.text() == "EnvironmentId")
   {
    QDomElement e = elem.firstChildElement("value");
    if(e.attribute("type") == "QByteArray")
    {
     qDebug() << e.text();
    }
//    QUuid id = QUuid::createUuid();
//    qDebug() << "new id = " << id.toString();

//    QDomElement newId = doc.createElement("value");
//    newId.setAttribute("type", "QByteArray");
//    newId.appendChild(doc.createTextNode(id.toString()));
//    elem.replaceChild(newId, e);
   }
  }
 }
 file.close();
 QFile ofile(pwd + QDir::separator()+ target + ".pro.user");
 {
  if(ofile.open(QIODevice::WriteOnly | QIODevice::Truncate))
  {
   QTextStream stream(&ofile);
   doc.save(stream, 4);
   ofile.close();
  }
 }
}

void MainWindow::createUserFile()
{
 QString filename = pwd + QDir::separator() + target + ".pro.user";
 //QFileInfo user(filename);
 //if(!user.exists())
  writeUserFile(filename);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
 onExit();
}

void MainWindow::onExit()
{
 checkDirty();
 checkUserFile();
}
bool MainWindow::checkDirty()
{
 if(QMessageBox::warning(this, tr("Project File Chaned"), tr("The project file has changed .\nDo you want to save it?"),
                         QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
 {
  writeProFile();
  return true;
 }
 return false;
}
bool MainWindow::checkUserFile()
{
 if(target == "")
  return true;
 QFileInfo info(pwd + QDir::separator() + target + ".pro.user");
 if(!info.exists())
 {
  if(QMessageBox::warning(this, tr("No User file"), tr("A user file has not yet been created.\nDo you want to create one?"),
                          QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
  {
   createUserFile();
   return true;
  }
 }
 return false;
}

