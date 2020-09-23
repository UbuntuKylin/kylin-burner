#ifndef K3BFILEFILTER_H
#define K3BFILEFILTER_H

#include <QDialog>
#include <QCheckBox>

class FileFilter : public QDialog
{
    Q_OBJECT
signals:
    void setHidden(bool);
    void setBroken(bool);
    void setReplace(bool);

public:
    explicit FileFilter(QWidget *parent = nullptr);
    ~FileFilter();

    QCheckBox *discard_hidden_file;
    QCheckBox *discard_broken_link;
    QCheckBox *follow_link;

    Q_SLOT void filter_exit();

private slots:
    void hiddenChanged(int);
    void brokenChanged(int);
    void replaceChanged(int);

public:
    void setIsHidden(bool);
    void setIsBroken(bool);
    void setIsReplace(bool);

private:

    QDialog *filter_dialog;
};

#endif // BURNFILTER_H
