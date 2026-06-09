/********************************************************************************
** Form generated from reading UI file 'licenseplatedialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LICENSEPLATEDIALOG_H
#define UI_LICENSEPLATEDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_licensePlateDialog
{
public:
QStackedWidget *stackedWidget;
QWidget *cameraPage;
QScrollArea *m_camArea;
QWidget *scrollAreaWidgetContents;
QWidget *layoutWidget;
QHBoxLayout *horizontalLayout;
QSpacerItem *horizontalSpacer;
QPushButton *m_btnStart;
QPushButton *m_btnStop;
QPushButton *m_btnCapture;
QSpacerItem *horizontalSpacer_2;
QWidget *payPage;
QPushButton *m_btnBack;
QPushButton *m_btnClear;
QTextEdit *m_txtResult;
QPushButton *m_btnOK;
// Create a list of QPushButtons
QList<QPushButton*> m_buttons;

private:
QStringList m_buttonNames;

public:
void setupUi(QDialog *licensePlateDialog)
{
  if (licensePlateDialog->objectName().isEmpty())
    licensePlateDialog->setObjectName(QString::fromUtf8("licensePlateDialog"));
  licensePlateDialog->resize(810, 600);
  stackedWidget = new QStackedWidget(licensePlateDialog);
  stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
  stackedWidget->setGeometry(QRect(30, 10, 771, 581));
  cameraPage = new QWidget();
  cameraPage->setObjectName(QString::fromUtf8("cameraPage"));
  m_camArea = new QScrollArea(cameraPage);
  m_camArea->setObjectName(QString::fromUtf8("m_camArea"));
  m_camArea->setGeometry(QRect(120, 20, 551, 291));
  m_camArea->setWidgetResizable(true);
  scrollAreaWidgetContents = new QWidget();
  scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
  scrollAreaWidgetContents->setGeometry(QRect(0, 0, 549, 289));
  m_camArea->setWidget(scrollAreaWidgetContents);
  layoutWidget = new QWidget(cameraPage);
  layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
  layoutWidget->setGeometry(QRect(200, 410, 351, 71));
  horizontalLayout = new QHBoxLayout(layoutWidget);
  horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
  horizontalLayout->setContentsMargins(0, 0, 0, 0);
  horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

  horizontalLayout->addItem(horizontalSpacer);

  m_btnStart = new QPushButton(layoutWidget);
  m_btnStart->setObjectName(QString::fromUtf8("m_btnStart"));

  horizontalLayout->addWidget(m_btnStart);

  m_btnStop = new QPushButton(layoutWidget);
  m_btnStop->setObjectName(QString::fromUtf8("m_btnStop"));

  horizontalLayout->addWidget(m_btnStop);

  m_btnCapture = new QPushButton(layoutWidget);
  m_btnCapture->setObjectName(QString::fromUtf8("m_btnCapture"));

  horizontalLayout->addWidget(m_btnCapture);

  horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

  horizontalLayout->addItem(horizontalSpacer_2);

  stackedWidget->addWidget(cameraPage);

  payPage = new QWidget();
  payPage->setObjectName(QString::fromUtf8("payPage"));

  m_buttonNames = QStringList({ "m_btn0", "m_btn1", "m_btn2", "m_btn3", "m_btn4",
                                "m_btn5", "m_btn6", "m_btn7", "m_btn8", "m_btn9",
                                "m_btnA", "m_btnB", "m_btnC", "m_btnD", "m_btnE",
                                "m_btnF", "m_btnG", "m_btnH", "m_btnI", "m_btnJ",
                                "m_btnK", "m_btnL", "m_btnM", "m_btnN", "m_btnO",
                                "m_btnP", "m_btnQ", "m_btnR", "m_btnS", "m_btnT",
                                "m_btnU", "m_btnV", "m_btnW", "m_btnX", "m_btnY",
                                "m_btnZ" });

  const int numButtons = m_buttonNames.size();
  const int buttonWidth = 71;
  const int buttonHeight = 51;
  const int numButtonsPerRow = 9;

  for (int i = 0; i < numButtons; ++i)
    {
      // Create a new QPushButton and set its object name and geometry
      QPushButton* button = new QPushButton(payPage);
      button->setObjectName(m_buttonNames[i]);

      button->setGeometry(QRect((i % numButtonsPerRow) * buttonWidth, (i / numButtonsPerRow) * buttonHeight + 160, buttonWidth, buttonHeight));

      // Add the button to the list of buttons
      m_buttons.append(button);
    }

  m_btnBack = new QPushButton(payPage);
  m_btnBack->setObjectName(QString::fromUtf8("m_btnBack"));
  m_btnBack->setGeometry(QRect(590, 470, 71, 51));
  QFont font;
  font.setPointSize(13);
  m_btnBack->setFont(font);
  m_btnBack->setIconSize(QSize(16, 16));

  m_btnClear = new QPushButton(payPage);
  m_btnClear->setObjectName(QString::fromUtf8("m_btnClear"));
  m_btnClear->setGeometry(QRect(680, 470, 71, 51));
  m_btnClear->setFont(font);
  m_btnClear->setIconSize(QSize(16, 16));

  m_txtResult = new QTextEdit(payPage);
  m_txtResult->setObjectName(QString::fromUtf8("m_txtResult"));
  m_txtResult->setGeometry(QRect(10, 30, 391, 41));

  m_btnOK = new QPushButton(payPage);
  m_btnOK->setObjectName(QString::fromUtf8("m_btnOK"));
  m_btnOK->setGeometry(QRect(440, 30, 71, 51));
  m_btnOK->setFont(font);
  m_btnOK->setIconSize(QSize(16, 16));

  stackedWidget->addWidget(payPage);

  retranslateUi(licensePlateDialog);

  QMetaObject::connectSlotsByName(licensePlateDialog);
}     // setupUi

void retranslateUi(QDialog *licensePlateDialog)
{
  licensePlateDialog->setWindowTitle(QCoreApplication::translate("licensePlateDialog", "licensePlateDialog", nullptr));
  m_btnStart->setText(QCoreApplication::translate("licensePlateDialog", "Camera Start", nullptr));
  m_btnStop->setText(QCoreApplication::translate("licensePlateDialog", "Camera Stop", nullptr));
  m_btnCapture->setText(QCoreApplication::translate("licensePlateDialog", "Capture", nullptr));
  m_btnBack->setText(QCoreApplication::translate("licensePlateDialog", "<-", nullptr));
  m_btnClear->setText(QCoreApplication::translate("licensePlateDialog", "clear", nullptr));
  m_btnOK->setText(QCoreApplication::translate("licensePlateDialog", "OK", nullptr));
  for (int i = 0; i < m_buttonNames.size(); ++i)
    {
      QStringList bottom_parts = m_buttonNames[i].split('_');
      QString lastPart = bottom_parts.last();
      QChar c = lastPart[lastPart.size() - 1];
      m_buttons[i]->setText(c);
    }
}     // retranslateUi
};

namespace Ui {
class licensePlateDialog : public Ui_licensePlateDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LICENSEPLATEDIALOG_H
