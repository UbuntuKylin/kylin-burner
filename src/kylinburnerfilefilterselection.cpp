#include "kylinburnerfilefilterselection.h"
#include "ui_kylinburnerfilefilterselection.h"

#include <QMouseEvent>

KylinBurnerFileFilterSelection::KylinBurnerFileFilterSelection(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KylinBurnerFileFilterSelection)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    this->move(25, parent->height() - 55 - this->height());
    this->hide();
    ui->labelTitle->setText(i18n("Kylin-Burner"));
    ui->labelName->setText(i18n("FilterSetting"));


    ui->labelClose->setAttribute(Qt::WA_Hover, true);
    ui->labelClose->installEventFilter(this);

    connect(this, SIGNAL(changeSetting(int, bool)), parent, SLOT(slotDoChangeSetting(int, bool)));
}

KylinBurnerFileFilterSelection::~KylinBurnerFileFilterSelection()
{
    delete ui;
}

bool KylinBurnerFileFilterSelection::eventFilter(QObject *obj, QEvent *event)
{
    QMouseEvent *mouseEvent;

    switch (event->type())
    {
    case QEvent::MouseButtonPress:
        mouseEvent = static_cast<QMouseEvent *>(event);
        if (ui->labelClose == obj && (Qt::LeftButton == mouseEvent->button())) this->hide();
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

void KylinBurnerFileFilterSelection::labelCloseStyle(bool in)
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

void KylinBurnerFileFilterSelection::on_checkBoxHidden_stateChanged(int arg1)
{
    bool check = false;
    if (arg1 == Qt::Checked)  check = true;
    emit changeSetting(0, check);
}

void KylinBurnerFileFilterSelection::on_checkBoxWrongSymbol_stateChanged(int arg1)
{
    bool check = false;
    if (arg1 == Qt::Checked)  check = true;
    emit changeSetting(1, check);
}

void KylinBurnerFileFilterSelection::on_checkBoxReplaceSymbol_stateChanged(int arg1)
{
    bool check = false;
    if (arg1 == Qt::Checked)  check = true;
    emit changeSetting(2, check);
}

void KylinBurnerFileFilterSelection::setOption(bool hidden, bool broken, bool replace)
{
    if (hidden) ui->checkBoxHidden->setCheckState(Qt::Checked);
    else ui->checkBoxHidden->setCheckState(Qt::Unchecked);
    if (broken) ui->checkBoxWrongSymbol->setCheckState(Qt::Checked);
    else ui->checkBoxWrongSymbol->setCheckState(Qt::Unchecked);
    if (replace) ui->checkBoxReplaceSymbol->setCheckState(Qt::Checked);
    else ui->checkBoxReplaceSymbol->setCheckState(Qt::Unchecked);
}
