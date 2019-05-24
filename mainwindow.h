#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>

namespace Ui {
class MainWindow;
}

class Components;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool parseMakefile(QString path);
    bool parseSourceFile(QString path);
    QString headerFilePath(QString name);


public slots:
    bool onMakefile();
    bool writeProFile();
    void viewHeaders();
    void viewSources();
    void createUserFile();
    void onExit();

private:
    Ui::MainWindow *ui;
    Components* espComponents;
    Components* projComponents;
    Components* components;
    QString target;
    QString pwd;
    QString name;
    QString componentDir;
    QMap<QString, QString> sources;
    QMap<QString, QString> headers;
    QList<QString> includePaths;
    QStringList toolChainPaths;
    QString toolChainPath;
    QString idf_path;
    bool writeUserFile(QString fileName);
    bool _dirty = false;

    void closeEvent(QCloseEvent *event);
    bool checkDirty();
    bool checkUserFile();
};

#endif // MAINWINDOW_H
