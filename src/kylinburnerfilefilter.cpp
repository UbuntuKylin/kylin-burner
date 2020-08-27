#include "kylinburnerfilefilter.h"
#include "ui_kylinburnerfilefilter.h"

#include <QDebug>
#include <QMouseEvent>

KylinBurnerFileFilter::KylinBurnerFileFilter(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KylinBurnerFileFilter)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    this->move(parent->width() / 2 - width() / 2, parent->height() / 2 - height() / 2);
    this->hide();
    ui->labelTitle->setText(i18n("Kylin-Burner"));
    ui->labelName->setText(i18n("FileFilter"));
    ui->btnSetting->setText(i18n("FileFilterSetting"));
    ui->btnRecovery->setText(i18n("Recovery"));
    ui->labelClose->setAttribute(Qt::WA_Hover, true);
    ui->labelClose->installEventFilter(this);
}

KylinBurnerFileFilter::~KylinBurnerFileFilter()
{
    delete ui;
}

bool KylinBurnerFileFilter::eventFilter(QObject *obj, QEvent *event)
{
    QMouseEvent *mouseEvent;
    switch (event->type())
    {
    case QEvent::MouseButtonPress:
        mouseEvent = static_cast<QMouseEvent *>(event);
        if (ui->labelClose == obj && (Qt::LeftButton == mouseEvent->button())) this->close();
        break;
    case QEvent::HoverEnter:
        if (ui->labelClose == obj) labelCloseStyle(true);
        break;
    case QEvent::HoverLeave:
        if (ui->labelClose == obj) labelCloseStyle(false);
        break;
    default:
        return QWidget::eventFilter(obj, event);
    }

    return QWidget::eventFilter(obj, event);
}

void KylinBurnerFileFilter::labelCloseStyle(bool in)
{
    if (in)
    {
        ui->labelClose->setStyleSheet("background-color:rgba(247,99,87,1);"
                                      "image: url(:/icon/icon/icon-关闭-悬停点击.png); ");
    }
    else
    {
        ui->labelClose->setStyleSheet("background-color:transparent;"
                                      "image: url(:/icon/icon/icon-关闭-默认.png); ");
    }
}
