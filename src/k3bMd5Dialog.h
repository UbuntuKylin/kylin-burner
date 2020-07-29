#ifndef K3BMD5DIALOG_H
#define K3BMD5DIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>

using namespace::std;
namespace K3b{

namespace Device {
    class DeviceManager;
    class Device;
}

//class AppDeviceManager;

class Md5Check : public QDialog
{
    Q_OBJECT

public:
    explicit Md5Check(QWidget *parent = nullptr);
    ~Md5Check();
    bool checkMd5(const char* cmd);

public Q_SLOTS:
    void exit();
    void slotMediaChange( K3b::Device::Device* );
    void slotDeviceChange( K3b::Device::DeviceManager* );
    void md5_start();
    void openfile();
    void checkChange(int state);

private:

    QComboBox* combo;
    QCheckBox* check;
    QLineEdit* lineedit;
    QStringList mount_index;
    QPushButton* button_open;
};
}
#endif // MD5DIALOG_H

