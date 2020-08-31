#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "components.h"
#include <QFileDialog>
#include <QTextStream>
#include <QDebug>
#include "tablemodel.h"
#include <QtXml>
#include <QUuid>
#include <QMessageBox>
#include <QStringList>
#include <QDialog>
#include <QBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QStringList>
#include <QString>
#include <QProcess>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    componentDirs = QMap<QString, Components*>();


    ui->actionMakefile->setToolTip(tr("Specify the project's Makefile or CMakeLists.txt location"));
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
    connect(ui->actionAdd_define, SIGNAL(triggered(bool)), this, SLOT(onAddDefine()));
    connect(ui->actionAdd_path_to_components_or_SDK, SIGNAL(triggered(bool)), this, SLOT(onAddPath()));
    connect(ui->actionList_components, SIGNAL(triggered(bool)), this, SLOT(onListComponents()));
    ui->textEdit->setHidden(true);
    connect(ui->menuFile, SIGNAL(aboutToShow()), this, SLOT(onFileMenuAboutToShow()));
    connect(ui->menuTools, SIGNAL(aboutToShow()), this, SLOT(onToolsMenuAboutToShow()));


    env = QProcessEnvironment::systemEnvironment();
    toolChainPath = env.value("Home", "")+ QDir::separator() +"esp/xtensa-esp32-elf" +QDir::separator();
    idf_path = env.value("IDF_PATH");
    defs = QProcessEnvironment();
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

void MainWindow::onFileMenuAboutToShow()
{
 ui->actionSave->setEnabled(pwd != nullptr);
 ui->actionCreate_User_file->setEnabled(pwd != nullptr);
}

void MainWindow::onToolsMenuAboutToShow()
{
 ui->actionList_components->setEnabled(pwd != nullptr);
}

bool MainWindow::parseMakefile(QTextStream* stream, QString path, QString fn)
{
 if(fn.endsWith("project.mk"))
  return true;
 currMakefile.push(fn);
 if(stream->atEnd())
  throw MakeException("Unexpected EOF");
 QString line;
 while (stream->readLineInto(&line))
 {
  line = line.trimmed();
  if(line.startsWith("#")) continue;
  if(line.isEmpty()) continue;
  if(line.startsWith("ifeq"))
  {
   try
   {
    if(evaluateIf(line.mid(4).trimmed()))
    {
     parseMakefile(stream, path, fn);
    }
    else
    {
     bypass(stream);
    }
   }
   catch(UnknownVariable& ex)
   {
    qDebug() << ex.reason();
    bypass(stream);
   }
   continue;
  }
  else if(line.startsWith("ifneq"))
  {
   try
   {
    if(!evaluateIf(line.mid(4).trimmed()))
    {
     parseMakefile(stream, path, fn);
    }
    else
    {
     bypass(stream);
    }
   }
   catch(UnknownVariable& ex)
   {
    qDebug() << ex.reason();
    bypass(stream);
   }
  }
  if(line.startsWith("ifdef"))
  {
   if(evaluateDef(line.mid(4).trimmed())!= "")
   {
    parseMakefile(stream, path, fn);
   }
   else
   {
    bypass(stream);
   }
  }
  else if(line.startsWith("ifndef"))
  {
   try
   {
    if(evaluateDef(line.mid(4).trimmed())== "")
    {
     parseMakefile(stream, path, fn);
    }
    else
    {
     bypass(stream);
    }
   }
   catch(UnknownVariable& ex)
   {
    qDebug() << ex.reason();
    bypass(stream);
   }
  }
  else if(line.startsWith("endif"))
   return false;
  else if(line.startsWith("$(error "))
  {
   //QMessageBox::critical(this, tr("Undefined value"), line.mid(8, line.lastIndexOf(")")-8 ));
   throw MakeException(line.mid(8, line.lastIndexOf(")")-8));
  }
  else if(line.startsWith("PROJECT_NAME") && line.contains(":=")  )
  {
   QStringList sl = line.split(":=");
   if(sl.count()==2)
   {
    if(sl.at(0).trimmed() == QString("PROJECT_NAME"))
    {
     name = sl.at(1).trimmed();
    }
   }
  }
  else if(line.startsWith("include"))
  {
   QString iPath = line.remove("include").trimmed();
   processInclude(iPath);
  }
  else if(line.startsWith("COMPONENT_ADD_INCLUDEDIRS"))
  {
   if(line.contains(":="))
   {
    QStringList sl = line.split(":=");
    if(sl.count()>=2)
    {
     if(sl.at(1)== ".")
     {
      componentDirs.insertMulti("", new Components(path));
     }
    }
   }
  }
  else if(line.startsWith("EXTRA_COMPONENT_DIRS"))
  {
   if(line.contains("+="))
    line = line.mid(line.indexOf("+=")+2).trimmed();
   QStringList sl = line.split(" ");
   foreach(QString str, sl)
   {
    // TODO: handle possible continuation char (backslash)
    if(!str.endsWith("\\"))
    {
     path = expandLine(str);
     componentDirs.insertMulti("", new Components(path));
    }
   }
  }
  else if(line.contains(":="))
  {
   QStringList sl = line.split(":=");
   if(sl.count()>=2)
   {
    QString val = expandLine(sl.at(1).trimmed());
    QString key = sl.at(0).trimmed();
    if(!env.contains(key))
    {
     env.insert(key, val);
     qDebug() << "add to env: '" << sl.at(0) << "', " << val;
    }
   }
  }
 }

 QDir dir(path);
 QStringList nameFilters;
 nameFilters << "*.cpp" << "*.c" << "*.cc" << "*.h" << "*.hpp";
 QFileInfoList infolist = dir.entryInfoList(nameFilters, QDir::Files);
 foreach (QFileInfo info, infolist)
 {
  if(info.fileName().endsWith(".h") || info.fileName().endsWith(".hpp") )
  {
   if(!includePaths.contains(info.absolutePath()))
     componentDirs.insert("", new Components(info.absolutePath()));
  }
  parseSourceFile(info.absoluteFilePath());
 }

 currMakefile.pop();
 return false;
}

bool MainWindow::evaluateIf(QString inLine)
{
 QString line;
 if(inLine.startsWith("("))
 {
  line = inLine.mid(1, inLine.lastIndexOf(")")-1);
  QStringList sl = line.split(",");
  if(sl.count() < 2)
   return false;  // error
//  if(sl.at(0).startsWith("$("))
//  {
//   QString val = env.value(sl.at(0).mid(2, sl.at(0).indexOf(")") -2));
//   return (val == sl.at(1));
//  }

   return expandLine(sl.at(0)) == sl.at(1);

 }
 return false;
}

QString MainWindow::expandLine(QString line)
{
 if(!line.contains("$(")) return line;
 if(!line.contains(")")) throw BadPath(tr("missing end parenthesis: %1").arg(line));
 QString var = line.mid(line.indexOf("$("), line.indexOf(")")-line.indexOf("$(")+1);
 QString envKey = line.mid(line.indexOf("$(")+2, line.indexOf(")")-line.indexOf("$(")-2);
 if(!env.contains(envKey))
 {
  qDebug() << "env not found: " << envKey;
  throw UnknownVariable(tr("unknown variable %1").arg(envKey));
 }
 QString val = env.value(envKey);
 line = line.replace(var, val);
 return expandLine(line);
}

QString MainWindow::evaluateDef(QString inLine)
{
 QString line;
 if(inLine != "")
 {
   return defs.value(line);
 }
 return "";
}

void MainWindow::bypass(QTextStream *stream)
{
 QString line = stream->readLine();
 while(true)
 {
  if(line.startsWith("endif")) return;
  if(stream->atEnd())
   return;

  if(line.startsWith("ifeq") || line.startsWith("ifneq") || line.startsWith("ifdef") ||line.startsWith("ifndef"))
  {
   bypass(stream);
  }
  line = stream->readLine();
 }
}


bool MainWindow::onMakefile()
{
 QFileDialog* dialog = new QFileDialog();
 dialog->setFileMode(QFileDialog::ExistingFile);
 dialog->setNameFilter("Makefile CMakeLists.txt");
 QStringList fileNames;
 dialog->setToolTip(tr("Select the Makefile or CMakeLists.txt for the project"));
 if (dialog->exec())
     fileNames = dialog->selectedFiles();
 if(fileNames.isEmpty())
  return false;

 QFileInfo info(fileNames.at(0));
 qDebug() << "process " << fileNames.at(0);


 QDir pwd_dir(info.canonicalPath());
 QStringList pwd_list = pwd_dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
 if(pwd_list.contains("CMakeLists.txt") ||pwd_list.contains("Makefile"))
 {

     //components = new Components(info.path());

     QFile data(fileNames.at(0));
     if(data.open(QFile::ReadOnly))
     {
      QFileInfo info(fileNames.at(0));
      pwd = info.absolutePath();
      env.insert("PROJECT_PATH", pwd);
      componentDirs.insert("PROJECT_PATH", new Components(info.path()));

      QDir pwdDir(pwd); // pwd is Makefile's directory
      target= pwdDir.dirName();  // target will be project' dir name
      componentDir = "../"+pwdDir.dirName();
      QTextStream stream(&data);
      try
      {
       if(info.baseName() == "MakeFile")
        parseMakefile(&stream, info.absolutePath(), fileNames.at(0));
       else{
           componentDirs.insert("IDF_PATH", new Components());
           espComponents = new Components();
       }
      }
      catch(MakeException& ex)
      {
       QMessageBox::critical(this, tr("Exception"), ex.reason());
       return false;
      }
      catch(BadPath& ex)
      {
       QMessageBox::critical(this, tr("Exception"), ex.reason() + " in "+ currMakefile.top());
       return false;
      }
      catch(UnknownVariable& ex)
      {
       // ignore
      }
     }
     onListComponents();
     foreach(QString fileName, pwd_list)
     {
         if(fileName == "CMakeLists.txt" || fileName.toLower().startsWith("readme.")
                 || fileName == "sdkconfig" || fileName.startsWith("sdkconfig.")
                 || fileName == "LICENSE")
             otherFiles.insert(fileName, pwd_dir.path());
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
      if(header.contains(QDir::separator()))
       headerFilePath(header);
      if(!headers.contains(header))
      {
       QString path = headerFilePath(header);
       if(path != "")
        headers.insert(header, path);
        //parseSourceFile(path + QDir::separator() + header);
        bool bFound = false;
        QStringList list = componentDirs.value("")->headers().values();
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
         QMapIterator<QString, Components*> iter(componentDirs);
         while(iter.hasNext())
         {
          iter.next();
          list = iter.value()->headers().values();
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
        QStringList list = componentDirs.value("")->headers().values();
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
         QMapIterator<QString, Components*> iter(componentDirs);
         while(iter.hasNext())
         {
          iter.next();
          list = iter.value()->headers().values();
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
 QMapIterator<QString, Components*> iter(componentDirs);
 while (iter.hasNext())
 {
  iter.next();
  if(iter.value()->headers().contains(name))
  {
   QString path = iter.value()->headers().value(name);
   QString prePath = path.remove(inName);
   //QFileInfo info(path);
   if(!includePaths.contains(prePath) && !(prePath.endsWith(".h")|| prePath.endsWith(".hpp") ))
   {
    includePaths.append(prePath);
    qDebug() << "add " << prePath << " to includePaths";
   }
   return prePath;
  }
 }
 qDebug() << name << " not found";
 return "";
}
#if 0
bool MainWindow::writeProFile()
{

 QString filename = pwd + QDir::separator() + name + ".pro";
 qDebug() << "write pro file: " << filename;
 QFile file(filename);
 if(file.open(QFile::WriteOnly | QFile::Truncate))
 {
  QTextStream out(&file);
  out << "\tTARGET = " << target << "\n";
  out << "\n\n";
  //out << "IDF_PATH =" << idf_path;
  //out << "\n\n";
  QStringList keys = componentDirs.keys();
  //QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  foreach (QString str, keys) {
   if(str != "")
   {
    QString ePath =  env.value(str);
    QString home = QDir::homePath();
    if(ePath.startsWith(home))
     ePath = ePath.replace(home, "$(HOME)");
    out << "\t" << str << " = " << ePath << "\n\n";
   }
  }

  //getIncludePaths();

  out << "\tINCLUDEPATH += ";
  QDir dir(pwd);
  std::sort(includePaths.begin(), includePaths.end() );
  foreach(QString p, includePaths)
  {
   foreach (QString str, keys)
   {
    if(str == "")
     continue;
    if(p.startsWith(env.value(str)))
    {
     out << "\\\n\t\t\t";
     out << "$${" << str << "}" << p.mid(env.value(str).length()) << " ";
     break;
    }
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
   foreach (QString str, keys)
   {
    if(str == "")
    {
     if(p.startsWith(pwd))
     {
      out << "\\\n\t\t\t";
      out << dir.relativeFilePath(p) << " ";
      break;
     }
    }
//    if(p.startsWith(env.value(str)))
//    {
//     out << "\\\n\t\t\t";
//     out << "$${" << str << "}" << p.mid(env.value(str).length()) << " ";
//     break;
//    }
   }
  }
  out << "\n\n";
  out << "\tSOURCES += ";
  QMapIterator<QString, Components*> iter (componentDirs);
  //QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  while(iter.hasNext())
  {
   iter.next();
   QString envKey = iter.key();
   QString envPath = env.value(envKey);
   foreach(QString p, iter.value()->sources().values())
   {
    out << "\\\n\t\t\t";
    if(envKey != "" && p.startsWith(envPath))
    {
     out << "$${" << envKey << "}" << p.mid(envPath.length()) << " ";
    }
    else
     out << dir.relativeFilePath(p) << " ";
   }
  }
  file.close();
  _dirty = false;
 }
 return true;
}
#endif
bool MainWindow::writeProFile()
{

 QString filename = pwd + QDir::separator()+ target + ".pro";
 qDebug() << "write pro file: " << filename;
 QFile file(filename);
 if(file.open(QFile::WriteOnly | QFile::Truncate))
 {
  QTextStream* out = new QTextStream(&file);
  *out << "\tTARGET = " << target << "\n";
  *out << "\n\n";

  QStringList keys = componentDirs.keys();
  //QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  foreach (QString str, keys) {
   if(str != "")
   {
    QString ePath =  env.value(str);
    QString home = QDir::homePath();
    if(ePath.startsWith(home))
     ePath = ePath.replace(home, "$(HOME)");
    *out << "\t" << str << " = " << ePath << "\n\n";
   }
  }


  *out << "\tINCLUDEPATH += ";

  QDir dir(pwd);
  QMapIterator<QString, Components*> iter(componentDirs);
  while(iter.hasNext())
  {
   iter.next();
   if(iter.key() == "" || iter.key() == "PROJECT_PATH")
    continue;
   QString envKey = iter.key();
   QString envPath = env.value(envKey);

   foreach (QString p, iter.value()->includeDirs)
   {
    if(pathHasSources(p))
    {
     *out << "\\\n\t\t\t";
     if( p.startsWith(envPath))
     {
      *out << "$${" << envKey << "}" << p.mid(envPath.length()) << " ";
     }
     else
      *out << dir.relativeFilePath(p) << " ";
    }
   }
  }
  *out << "\n\n";
  *out << "\tXTENSA_TOOLCHAIN += $$(HOME)/esp/xtensa-esp32-elf\n";
  *out << "\tINCLUDEPATH += \\\n\t\t\t$${XTENSA_TOOLCHAIN}/xtensa-esp32-elf/include \\\n";
  *out << "\t\t\t$${XTENSA_TOOLCHAIN}/xtensa-esp32-elf/include/c++/5.2.0 \\\n";
  *out << "\t\t\t$${XTENSA_TOOLCHAIN}/xtensa-esp32-elf/include/c++/5.2.0/xtensa-esp32-elf \\\n";
  *out << "\t\t\t$${XTENSA_TOOLCHAIN}/lib/gcc/xtensa-esp32-elf/5.2.0/include \\\n";
  *out << "\t\t\t$${XTENSA_TOOLCHAIN}/lib/gcc/xtensa-esp32-elf/5.2.0/include-fixed \\\n";
  *out << "\t\t\t$${XTENSA_TOOLCHAIN}/lib/gcc/xtensa-esp32-elf/include/sys \n";
  *out << "\n";

  *out << "\tINCLUDEPATH += ";
  QString envKey = "PROJECT_PATH";
  QString envPath = env.value(envKey);
  foreach(QString p, componentDirs.value(envKey)->includeDirs)
  {
   if(pathHasSources(p))
   {
    *out << "\\\n\t\t\t";
    *out << dir.relativeFilePath(p) << " ";
   }
  }
  *out << "\n\n";
  *out << "\tSOURCES += ";
  iter = QMapIterator<QString, Components*>(componentDirs);
  //QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  while(iter.hasNext())
  {
   iter.next();
   QString envKey = iter.key();
   QString envPath = env.value(envKey);
   foreach(QString p, iter.value()->sources().values())
   {
    *out << "\\\n\t\t\t";
    if(envKey != "" && p.startsWith(envPath))
    {
     *out << "$${" << envKey << "}" << p.mid(envPath.length()) << " ";
    }
    else
     *out << dir.relativeFilePath(p) << " ";
   }
  }
  *out << "\n\n";
  *out << "\tHEADERS += ";
  iter = QMapIterator<QString, Components*>(componentDirs);
  //QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  while(iter.hasNext())
  {
   iter.next();
   QString envKey = iter.key();
   if(envKey == (idf_path))
    continue;
   QString envPath = env.value(envKey);
   foreach(QString p, iter.value()->headers().values())
   {
    *out << "\\\n\t\t\t";
    if(envKey != "" && p.startsWith(envPath))
    {
     *out << "$${" << envKey << "}" << p.mid(envPath.length()) << " ";
    }
    else
     *out << dir.relativeFilePath(p) << " ";
   }
  }

  *out << "\n\n";
  *out << "\tOTHER_FILES += ";
  QMapIterator<QString, QString> iter1 = QMapIterator<QString, QString>(otherFiles);
  //QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  while(iter1.hasNext())
  {
   iter1.next();
   QString envKey = iter1.key();
   *out << "\\\n\t\t\t";
   *out  << envKey << " ";
  }

//  *out << "\n\n";
//  *out << "\tOTHER_FILES += ";
  iter = QMapIterator<QString, Components*>(componentDirs);
  //QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  while(iter.hasNext())
  {
   iter.next();
   QString envKey = iter.key();
   if(envKey == (idf_path))
    continue;
   QString envPath = env.value(envKey);
   foreach(QString p, iter.value()->otherFiles().values())
   {
    *out << "\\\n\t\t\t";
    if(envKey != "" && p.startsWith(envPath))
    {
     *out << "$${" << envKey << "}" << p.mid(envPath.length()) << " ";
    }
    else
     *out << dir.relativeFilePath(p) << " ";
   }
  }
  // If there is an sdkconfig file, process it.
  process_sdkconfig(out);

  file.close();
  _dirty = false;
 }
 return true;
}

void MainWindow::viewHeaders()
{

 QList<ComponentListEntry*>* cList = new QList<ComponentListEntry*>();
 if(componentDirs.contains("PROJECT_PATH"))
 {
  foreach (QString s, componentDirs.value("PROJECT_PATH")->headers().keys())
  {
   ComponentListEntry* entry = new ComponentListEntry(s, componentDirs.value("PROJECT_PATH")->headers().value(s), false);
   cList->append(entry);
  }
  ui->tv1->setModel(new TableModel(cList));
  ui->tv1->resizeColumnsToContents();

 }
}

void MainWindow::viewSources()
{
 QList<ComponentListEntry*>* cList = new QList<ComponentListEntry*>();
 foreach (QString s, componentDirs.value("PROJECT_PATH")->sources().keys()) {
  cList->append(new ComponentListEntry(s, componentDirs.value("")->sources().value(s), false));
 }
 ui->tv1->setModel(new TableModel(cList));
 ui->tv1->resizeColumnsToContents();

}

bool MainWindow::writeUserFile(QString fileName)
{
 qDebug() << "write user file: " << fileName;
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
 QFile ofile(fileName);
 {
  if(ofile.open(QIODevice::WriteOnly | QIODevice::Truncate))
  {
   QTextStream stream(&ofile);
   doc.save(stream, 4);
   ofile.close();
  }
 }
 return true;
}

void MainWindow::createUserFile()
{
 QString filename = pwd + QDir::separator() + name + ".pro.user";
 //QFileInfo user(filename);
 //if(!user.exists())
  writeUserFile(filename);
}

void MainWindow::closeEvent(QCloseEvent *)
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
 if(_dirty)
 {
  if(QMessageBox::warning(this, tr("Project File Chaned"), tr("The project file has changed .\nDo you want to save it?"),
                          QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
  {
   writeProFile();
   return true;
  }
  return false;
 }
 return true;
}
bool MainWindow::checkUserFile()
{

 if(target == "")
  return true;
 QFileInfo info(pwd + QDir::separator() + name + ".pro.user");
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

void MainWindow::processInclude(QString iPath)
{
 QString key;
 QString path = expandLine(iPath);
#if 0
 // need to also decode .mk file
 QFileInfo info( path);
 QString fn;
 if(QFileInfo(info.path()).exists() && info.fileName().endsWith(".mk"))
 {
  fn = info.absoluteFilePath();
  qDebug() << "process " << fn;
  QFile file(fn);
  if(file.open(QIODevice::ReadOnly))
  {
   QTextStream stream(&file);
   parseMakefile(&stream, info.absolutePath(), fn);
  }
  else
  {
   throw BadPath(tr("not found: %1").arg(fn));
  }
 }
#else
 QFileInfo info(path);
 if(info.isDir())
  componentDirs.insertMulti("", new Components(path));
 else
  componentDirs.insertMulti("", new Components(info.absolutePath()));
#endif

}

void MainWindow::onAddPath()
{
 createDialog(tr("Add path to components/SDK"), env);
 dlg->exec();
 componentDirs.insert(dialogKey->text(), new Components(dialogValue->text()));
}

void MainWindow::onAddDefine()
{
 createDialog(tr("Add define"),  defs);
 dlg->exec();
}

void MainWindow::createDialog(QString label, QProcessEnvironment env)
{
 dlg = new QDialog();
 dlgenv = env;
 QVBoxLayout* vLayout;
 dialogOk = new QPushButton("OK");
 dialogOk->setEnabled(false);
 dlg->setLayout(vLayout = new QVBoxLayout());
 vLayout->addWidget(new QLabel(label));
 vLayout->setObjectName("vLayout");
 QHBoxLayout* hLayout1 = new QHBoxLayout();
 hLayout1->setObjectName("vLayout");
 dialogKey = new QLineEdit();
 hLayout1->addWidget(new QLabel(tr("Key:")));
 hLayout1->addWidget(dialogKey);
 hLayout1->addWidget(new QLabel(tr("value:")));
 connect(dialogKey, SIGNAL(textChanged(QString)), this, SLOT(onDialogKeyChanged(QString)));
 QPushButton* browse = new QPushButton(tr("Browse"));
 dialogValue = new QLineEdit();
 dialogValue->setPlaceholderText("enter environment value");
 connect(dialogValue, SIGNAL(textChanged(QString)), this, SLOT(onDialogValueChanged(QString)));
 hLayout1->addWidget(dialogValue);
 hLayout1->addWidget(browse);
 connect(browse, SIGNAL(clicked(bool)), this, SLOT(onBrowse()));
 vLayout->addLayout(hLayout1);
 QHBoxLayout* hLayout2 = new QHBoxLayout();
 hLayout2->setObjectName("hLayout2");
 QPushButton* cancel = new QPushButton("Cancel");
 hLayout2->addWidget(dialogOk);
 hLayout2->addWidget(cancel);
 vLayout->addLayout(hLayout2);
 connect(dialogOk, SIGNAL(clicked(bool)), this, SLOT(onDialogOk()));
 connect(cancel, SIGNAL(clicked(bool)), this, SLOT(onDialogCancel()));
}

void MainWindow::onDialogCancel()
{
 dlg->reject();
}

void MainWindow::onDialogOk()
{
 QString val = dialogValue->text().trimmed();
 if(val.isEmpty())
  return;
 env.insert(dialogKey->text().trimmed(), val);
 dlg->accept();
}

void MainWindow::onBrowse()
{
 QString path = QFileDialog::getExistingDirectory(this, tr("Get Path"));
 dialogValue->setText(path);
}

void MainWindow::onDialogKeyChanged(QString s)
{
 if(s.isEmpty())
 {
  dialogOk->setEnabled(false);
  return;
 }
 dialogValue->setText(env.value(s.trimmed()));
 if( !dialogValue->text().isEmpty())
  dialogOk->setEnabled(true);
 else
  dialogOk->setEnabled(false);
}

void MainWindow::onDialogValueChanged(QString s)
{
 if(s.isEmpty())
 {
  dialogOk->setEnabled(false);
  return;
 }
 if(!dialogValue->text().isEmpty())
  dialogOk->setEnabled(true);
 else
  dialogOk->setEnabled(false);
}

void MainWindow::getIncludePaths()
{
 QMapIterator<QString, Components*> iter(componentDirs);
 while(iter.hasNext())
 {
  iter.next();
  QString key = iter.key();
  if(key != "COMPONENT_DIRS")
   continue;
  Components* comp = iter.value();
  QMapIterator<QString, QString> hdrs(comp->headers());
  while(hdrs.hasNext())
  {
   hdrs.next();
   QFileInfo info(hdrs.value());
   if(!includePaths.contains(info.absolutePath()))
   {
    QDir dir(info.absolutePath());
    includePaths.append(info.absolutePath());
    dir.cdUp();
    if(!includePaths.contains(dir.path()))
       includePaths.append(dir.path());
   }
  }
 }
}

QProcess::ExitStatus MainWindow::listComponents(QString wd)
{
 QProcess::ProcessError error;
 QProcess::ExitStatus exitStatus;
  makeProcess = new QProcess(this);
  connect (makeProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(processStdOutput()));  // connect process signals with your code
  connect (makeProcess, SIGNAL(readyReadStandardError()), this, SLOT(processErrOutput()));  // same here
  makeProcess->setWorkingDirectory(wd);
  makeProcess->setProcessEnvironment(env);
  QString exec = "make";
  QStringList params;
  params << "Makefile" << "list-components";
  makeProcess->start(exec, params);
  makeProcess->waitForStarted();
  makeProcess->waitForFinished(); // sets current thread to sleep and waits for makeProcess end
  error = makeProcess->error();
  exitStatus = makeProcess->exitStatus();
  return exitStatus;
}

void MainWindow::processStdOutput()
{
 ui->textEdit->setTextColor(QColor("black"));
 QString output = makeProcess->readAllStandardOutput();
 ui->textEdit->append(output);
 QStringList sl = output.split("\n");
 foreach (QString line, sl)
 {
  if(line.startsWith("COMPONENT_DIRS"))
   currVariable = "COMPONENT_DIRS";
  if(line.startsWith("TEST_COMPONENTS"))
   currVariable = "TEST_COMPONENTS";
  if(line.startsWith("TEST_EXCLUDE_COMPONENTS"))
   currVariable = "TEST_EXCLUDE_COMPONENTS";
  if(line.startsWith("COMPONENT_PATHS"))
   currVariable = "COMPONENT_PATHS";
  if(line.startsWith("/") && (currVariable == "COMPONENT_DIRS"  || currVariable == "COMPONENT_PATHS"))
  {
   QFileInfo dir(line);
   QString key = "";

   if(dir.isDir())
   {
    foreach (QString envKey, componentDirs.keys())
    {
     if(line.startsWith(env.value(envKey)))
      key = envKey;
      break;
    }
    if(key == "")
    {
     if(line.startsWith(pwd))
      key = "PROJECT_PATH";
     else if (line.startsWith(idf_path))
      key = "IDF_PATH";
     else if(line.startsWith(QDir::homePath()))
      key = "HOME";
     else
      key = "OTHER";
    }
    if(!componentDirs.contains(key))
     componentDirs.insert(key, new Components(line));
    else
     componentDirs.value(key)->update(line);
    QFileInfo info(line + QDir::separator() + "include");
    if(info.exists() && !componentDirs.value(key)->includeDirs.contains(info.absoluteFilePath()))
     componentDirs.value(key)->includeDirs.append(info.absoluteFilePath());
   }
  }
 }
}

void MainWindow::processErrOutput()
{
 ui->textEdit->setTextColor(QColor("red"));
 //qDebug() << makeProcess->readAllStandardError();  // read error channel
 ui->textEdit->append(makeProcess->readAllStandardError());
}

void MainWindow::onListComponents()
{
 currVariable = "";
 componentDirs.remove("COMPONENT_DIRS");
 if(pwd != nullptr)
 {
  ui->textEdit->setVisible(true);
  ui->textEdit->clear();
  listComponents(pwd);
 }
}

bool MainWindow::pathHasSources(QString path)
{
 // return true if path exists and contains source files or headers
 QFileInfo info(path);
 if(!info.exists())
  return false;
 if(info.isFile())
  return false;
 QDir dir(path);
// if(dir.dirName() == "freertos" || dir.dirName() == "lwip")
//  qDebug() << path;
 if(dir.dirName() == "include")
  return true;
 QFileInfoList list = dir.entryInfoList(QStringList() << "*.c" << "*.cpp" << "*.h" << "*.hpp",QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);
 dir.cdUp();
 if(dir.dirName() == "include")
  return true;
 return !(list.count() == 0);
}

void MainWindow::process_sdkconfig(QTextStream* out)
{
    bool bFirst = false;
    QFile sdkconfig(pwd + QDir::separator() + "sdkconfig");
    if(sdkconfig.exists())
    {
        if(sdkconfig.open(QIODevice::ReadOnly | QIODevice::Text))
        {
           while(!sdkconfig.atEnd())
           {
               QString line = sdkconfig.readLine().trimmed();
               if(line.startsWith("#") || line.isEmpty())
                   continue;
               if(!bFirst)
               {
                   *out << "\n\n\tDEFINES+=\t" << line << "\n";
                   bFirst= true;
               }
               else
               {
                   *out << "\t\t\t" << line<< " \\\n";
               }
           }
           if(bFirst)
               *out << "\n";
           sdkconfig.close();
        }
    }
}
