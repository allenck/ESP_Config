#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QProcessEnvironment>
#include <QTextStream>
#include "exceptions.h"
#include <QStack>
#include <QActionGroup>
#include <QHash>

namespace Ui {
class MainWindow;
}

class Options;
class QPushButton;
class QLineEdit;
class Components;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool parseMakefile(QTextStream *stream, QString path, QString fn);
    bool parseSourceFile(QString path);
    QString headerFilePath(QString name);


public slots:
    bool onMakefile();
    bool writeProFile();
    void viewHeaders();
    void viewSources();
    void viewOptions();
    void createUserFile();
    void onExit();
    void onAddDefine();
    void onAddPath();
    void onListComponents();
    void onAddDirToIgnore();
    void onSaveSdkconfig();

private:
    Ui::MainWindow *ui;
    Components* espComponents = nullptr;
    //Components* projComponents = nullptr;
    //Components* components = nullptr;
    QMap<QString, Components*> componentDirs;
    QMap<QString, bool> dir_ignore;
    QString target;
    QString pwd;
    QString name;
    QString componentDir;
    QMap<QString, QString> sources;
    QMap<QString, QString> headers;
    QMap<QString, QString> otherFiles;
    QList<QString> includePaths;
    QStringList toolChainPaths;
    QString toolChainPath;
    QString idf_path;
    bool writeUserFile(QString fileName);
    bool _dirty = false;
    QProcessEnvironment env;
    QProcessEnvironment defs;
    bool evaluateIf(QString);
    QString evaluateDef(QString);
    void bypass(QTextStream* stream);
    QActionGroup* dirIgnoreActGrp = nullptr;
    QMap<QString, QString>* sdkconfig = new QMap<QString, QString>();
    QList<QString> optionList;
    void closeEvent(QCloseEvent *event);
    bool checkDirty();
    bool checkUserFile();
    void processInclude(QString iPath);
    QString expandLine(QString line);
    QStack<QString> currMakefile;
    void getIncludePaths();
    QProcess::ExitStatus listComponents(QString wd);
    QProcess* makeProcess;
    QString currVariable;
    bool pathHasSources(QString);
    void procesSdkconfig();
    void outputOptions(QTextStream* out);

private slots:
    void onDialogOk();
    void onDialogCancel();
    void onBrowse();
    void onDialogKeyChanged(QString);
    void onDialogValueChanged(QString);
    void onFileMenuAboutToShow();
    void onToolsMenuAboutToShow();
    void processStdOutput();
    void processErrOutput();
    void on_dirIgnoreAction(QAction*);

protected:
    void createDialog(QString label, QProcessEnvironment env);
    QDialog* dlg = nullptr;
    QLineEdit* dialogValue = nullptr;
    QLineEdit* dialogKey = nullptr;
    QPushButton* dialogOk = nullptr;
    QProcessEnvironment dlgenv;
};

#endif // MAINWINDOW_H
