#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>



namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QTimer* monTimer;
    QTimer* inputTimer;
    QTimer* tmrEmergency;
    QTimer* tmrDirection;

    void putSystemMessage(QString);
    void refreshTime(void);

    unsigned char m_LampValue;
    bool m_EmergencyValue;
    bool m_EmergencyLampValue;
    bool m_LampDirState;
    bool m_LampDirTimerState;
    unsigned char m_LampDirValue;
    unsigned char m_RotaryLedValue;


    int m_CanFd;
    struct sockaddr_can m_CanAddr;
    struct can_frame m_CanSendFrame;
    struct ifreq m_Canifr;

    // I2C R/W
    int mI2cFd;
    int setI2cDevice(int *fd, int addr);
    int setI2cRegister(int *fd, unsigned char reg);
    int setI2cRegister(int *fd, unsigned char hb, unsigned char lb);

    // Temperature
    int getI2cTempRegValue(int *fd, double* value,  size_t size);
    QString getI2cTemperatureValue(void);

    // Illumination
    unsigned char getI2cIlluminRegValue(int *fd);
    QString getI2cIlluminationValue(void);
    
    // Rotary Switch
    int parseCanPacketData(void);
    unsigned char getI2cExpGpioRotaryValue(int *fd);
    void setGpioLedControl(unsigned char led, bool state);
    void setGpioLedInitialization(void);

public slots:

    void slotBtnEmergencyClicked(void);
    void slotBtnLampLowClicked(void);
    void slotBtnLampFrontClicked(void);
    void slotBtnLampTailClicked(void);
    void slotBtnLampOffClicked(void);
    void slotDialLampValueChange(int value);
    void slotBtnDirColorReset(void);

    void slotBtnDirLeftClicked(void);
    void slotBtnDirOffClicked(void);
    void slotBtnDirRightClicked(void);
    void slotDialDirectionValueChange(int value);

    void slotBtnLampColorReset(void);
    void slotTimeDisplay(void);
    void slotBtnMsgClearClicked(void);

    void slotLampDirectionState(void);
    void slotLampEmerState(void);

    int setI2cExpGpioRotaryInit(int *fd);
    int setI2cExpGpioRotaryLed(int *fd);

    int openCanDevice(void);
    void sendCanDataPacket(unsigned char id, unsigned char type, unsigned char func, unsigned char value1, unsigned char value2);

    void slotInputDevState(void);
    void setGpioPushInitialization();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
