#include "optionwidget.h"
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>

OptionWidget::OptionWidget( QWidget *parent) : QWidget(parent)
{

}

OptionWidget::OptionWidget(QStringList options, QMap<QString, QString>* sdkConfig, QWidget *parent) : QWidget(parent)
{
    this->options = options;
    this->sdkConfig = sdkConfig;
    init(options, sdkConfig) ;
}

void OptionWidget::init(QStringList options, QMap<QString, QString> *sdkConfig)
{
    this->options = options;
    this->sdkConfig = sdkConfig;

    if(!initDone)
    {
        setMinimumHeight(100);
        QScrollArea* scrollArea = new QScrollArea();
        setLayout(new QVBoxLayout());
        panel = new QWidget();
        panel->setLayout(new QVBoxLayout());
        scrollArea->setWidget(panel);
        scrollArea->setWidgetResizable(true);
        layout()->addWidget(scrollArea);
        initDone = true;
    }
    else
    {
        while ( QWidget* w = panel->findChild<QWidget*>() )
            delete w;
    }

    foreach (QString line, options)
    {
        if(line.isEmpty())
        {
            QLabel* label = new QLabel(line);
            panel->layout()->addWidget(label);
            continue;
        }

        if(line.startsWith("#"))
        {
            if(!line.endsWith(" is not set"))
            {
                QLabel* label = new QLabel(line);
                panel->layout()->addWidget(label);
                continue;
            }
            else
            {
                QStringList sl = line.split(" ");
                if(sl.count() >= 3)
                {
                   QWidget* panel1 = new QWidget();
                   QHBoxLayout* pane11Layout = new QHBoxLayout(panel1);
                   QLabel* label = new QLabel(sl.at(1));
                   pane11Layout->addWidget(label, 1, Qt::AlignLeft);
                   QLineEdit* le = new QLineEdit();
                   le->setPlaceholderText("not active");
                   le->setToolTip(tr("enter 'y', number or text"));
                   pane11Layout->addWidget(le, 0, Qt::AlignRight);
                   connect(le, &QLineEdit::textEdited, [=]{
                       updateOption(sl.at(1), le->text());
                   });
                   panel->layout()->addWidget(panel1);
                   continue;
                }
            }
        }
        else
        {
            QStringList sl = line.split("=");
            if(sl.count() >= 2)
            {
               QWidget* panel1 = new QWidget();
               QHBoxLayout* pane11Layout = new QHBoxLayout(panel1);
               QLabel* label = new QLabel(sl.at(0));
               pane11Layout->addWidget(label, 1, Qt::AlignLeft);
               QLineEdit* le = new QLineEdit();
               le->setText(sl.at(1));
               le->setToolTip(tr("enter 'y', number or text or blank to deactivate"));
               pane11Layout->addWidget(le, 0, Qt::AlignRight);
               connect(le, &QLineEdit::textEdited, [=]{
                   updateOption(sl.at(0), le->text());
               });
               panel->layout()->addWidget(panel1);
               continue;
            }
        }
    }
}

void OptionWidget::updateOption(QString name, QString value)
{
   if(value.isEmpty() || value == "y" || value == "n")
       sdkConfig->insert(name, value);
   else
   {
       if(value.startsWith("\""))
       {
           if(value.endsWith("\""))
               sdkConfig->insert(name, value);
           else
           {
               QMessageBox::critical(this, tr("Error"), tr("missing terminating \""));
               return;
           }
       }
       else
       {
           bool ok;
           int i;
           float d;
           value.toInt(&ok);
           if(ok)
           {
               sdkConfig->insert(name, value);
               return;
           }
           value.toInt(&ok, 16);
           if(ok)
           {
               sdkConfig->insert(name, value);
               return;
           }
           d= value.toFloat(&ok);
           if(ok)
           {
               sdkConfig->insert(name, value);
               return;
           }
           // must be a string
           sdkConfig->insert(name, "\""+value+"\"");
       }
   }
}
