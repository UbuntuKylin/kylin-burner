#ifndef TITLE_BAR_H
#define TITLE_BAR_H

#include "option/k3boptiondialog.h"
#include "misc/k3bmediaformattingdialog.h"

#include <QToolButton>
#include <QLabel>

namespace K3b{
    
    namespace Device {
        class DeviceManager;
        class Device;
    }   

    class AppDeviceManager;

class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = 0);
    ~TitleBar();
    
    void formatMedium( K3b::Device::Device* );

protected:

    // 双击标题栏进行界面的最大化/还原
    virtual void mouseDoubleClickEvent(QMouseEvent *event);

    // 进行鼠界面的拖动
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

    // 进行最小化、最大化/还原、关闭操作
public Q_SLOT:    
    void onClicked();
    void popup();
    void clean();
    void md5();
    void filter();
    void help();
    void about();

private:
    QPoint mLastMousePosition;
    bool mMoving;
    // 最大化/还原
    void updateMaximize();

private:
    QLabel *m_pIconLabel;
    QLabel *m_pTitleLabel;
    QToolButton *m_pMenubutton;
    QPushButton *m_pMinimizeButton;
    QPushButton *m_pMaximizeButton;
    QPushButton *m_pCloseButton;
};
}
#endif // TITLE_BAR_H

