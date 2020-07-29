#ifndef BURNRESULTDIALOG_H
#define BURNRESULTDIALOG_H

#include <QDialog>

class BurnResult : public QDialog
{
    Q_OBJECT

public:
    explicit BurnResult( int , QString ,QWidget *parent = nullptr);
    ~BurnResult();

    Q_SLOT void exit();

private:
};

#endif // BURNRESULTDIALOG_H

