#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "devices.h"
#include "cmd_type.h"

#include "gpios.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <QDebug>





struct tm * g_SysTimeinfo;
time_t g_SysRawTime;;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    int retv;

    monTimer = new QTimer(this);
    monTimer->start(1000);
    inputTimer = new QTimer(this); // Read from Dev
    inputTimer->start(500);
    tmrEmergency = new QTimer(this);
    tmrDirection = new QTimer(this);

    m_LampDirTimerState = false;

    putSystemMessage("[System] GUI Initialization - OK.");


    mI2cFd = open(I2C_DEVICE, O_RDWR);
    if(mI2cFd<0) {
        putSystemMessage("[System] I2C Line Opened.");
    }

    retv = openCanDevice();
    if(retv<0)
        putSystemMessage("[System] CAN Device Failed!");


    // Init
    slotBtnLampOffClicked();
    slotBtnDirOffClicked();
    setGpioLedInitialization();
    setGpioPushInitialization();
    setGpioLedControl(GP_LEDA, B_OFF);

    setI2cExpGpioRotaryInit(&mI2cFd);

 


    connect(monTimer, SIGNAL(timeout()), SLOT(slotTimeDisplay()));
    connect(inputTimer, SIGNAL(timeout()), SLOT(slotInputDevState()));
    connect(tmrEmergency, SIGNAL(timeout()), SLOT(slotLampEmerState()));
    connect(tmrDirection, SIGNAL(timeout()), SLOT(slotLampDirectionState()));

    connect(ui->btnMsgClear, SIGNAL(clicked()), SLOT(slotBtnMsgClearClicked(void)));
    connect(ui->btnLampEmergency, SIGNAL(clicked()), SLOT(slotBtnEmergencyClicked(void)));

    connect(ui->btnLampLow, SIGNAL(clicked()), SLOT(slotBtnLampLowClicked()));
    connect(ui->btnLampFront, SIGNAL(clicked()), SLOT(slotBtnLampFrontClicked()));
    connect(ui->btnLampTail, SIGNAL(clicked()), SLOT(slotBtnLampTailClicked()));
    connect(ui->btnLampOff, SIGNAL(clicked()), SLOT(slotBtnLampOffClicked()));
    connect(ui->diaLamp, SIGNAL(valueChanged(int)), SLOT(slotDialLampValueChange(int)));

    connect(ui->btnLampDirLeft, SIGNAL(clicked()), SLOT(slotBtnDirLeftClicked()));
    connect(ui->btnLampDirOff, SIGNAL(clicked()), SLOT(slotBtnDirOffClicked()));
    connect(ui->btnLampDirRight, SIGNAL(clicked()), SLOT(slotBtnDirRightClicked()));
    connect(ui->diaDirection, SIGNAL(valueChanged(int)), SLOT(slotDialDirectionValueChange(int)));



}

void MainWindow::slotInputDevState(void)
{
    // Push Button


    // Rotary Button


    // Push Button Switch
}

int MainWindow::openCanDevice(void)
{
    if ((m_CanFd = socket(PF_CAN, SOCK_RAW, CAN_RAW)) == -1)
    {
        putSystemMessage("CAN Device Open Failed");
        return -1;
    } else {
        putSystemMessage("CAN Device Opened!");
    }

    strcpy(m_Canifr.ifr_name, CAN_DEVICE);
    ioctl(m_CanFd, SIOCGIFINDEX, &m_Canifr);

    m_CanAddr.can_family  = AF_CAN;
    m_CanAddr.can_ifindex = m_Canifr.ifr_ifindex;


    if (bind(m_CanFd, (struct sockaddr *)&m_CanAddr, sizeof(m_CanAddr)) == -1) {
        putSystemMessage("CAN Binding Error");
        return -1;
    }

    return 0;
}


void MainWindow::sendCanDataPacket(unsigned char id, unsigned char type, unsigned char func, unsigned char value1, unsigned char value2)
{
    int nbytes;

    m_CanSendFrame.can_id  = id;
    m_CanSendFrame.can_dlc = CAN_DEF_PACKET_SIZE;
    memset(m_CanSendFrame.data,0,sizeof(m_CanSendFrame.data));
    m_CanSendFrame.data[TICK] = 0; // Not used
    m_CanSendFrame.data[TYPE] = type;
    m_CanSendFrame.data[FUNC] = func;
    m_CanSendFrame.data[VAL1] = value1;
    m_CanSendFrame.data[VAL2] = value2;

    nbytes = write(m_CanFd, &m_CanSendFrame, sizeof(struct can_frame));
    if(nbytes<1)
    {
       putSystemMessage("Can data send failed!");
    }
}


void MainWindow::setGpioLedInitialization(void)
{
    system("echo 6 > /sys/class/gpio/export");
    usleep(1000);
    system("echo 16 > /sys/class/gpio/export");
    usleep(1000);
    system("echo 17 > /sys/class/gpio/export");
    usleep(1000);
    system("echo 18 > /sys/class/gpio/export");
    usleep(1000);

    system("echo out > /sys/class/gpio/gpio6/direction");
    usleep(1000);
    system("echo out > /sys/class/gpio/gpio16/direction");
    usleep(1000);
    system("echo out > /sys/class/gpio/gpio17/direction");
    usleep(1000);
    system("echo out > /sys/class/gpio/gpio18/direction");
}

void MainWindow::setGpioPushInitialization(void)
{
    system("echo 0 > /sys/class/gpio/export");
    usleep(1000);
    system("echo 5 > /sys/class/gpio/export");
    usleep(1000);

    system("echo in > /sys/class/gpio/gpio0/direction");
    usleep(1000);
    system("echo in > /sys/class/gpio/gpio5/direction");
    usleep(1000);
}



void MainWindow::setGpioLedControl(unsigned char led, bool state)
{
    if(state) // Off
    {
        switch(led)
        {
        case 0x00:
            system("echo 1 > /sys/class/gpio/gpio6/value");

            break;
        case 0x01:
            system("echo 1 > /sys/class/gpio/gpio16/value");

            break;
        case 0x02:
            system("echo 1 > /sys/class/gpio/gpio17/value");

            break;
        case 0x03:
            system("echo 1 > /sys/class/gpio/gpio18/value");

            break;
        case 0x04:
            system("echo 1 > /sys/class/gpio/gpio6/value");
            system("echo 1 > /sys/class/gpio/gpio16/value");
            system("echo 1 > /sys/class/gpio/gpio17/value");
            system("echo 1 > /sys/class/gpio/gpio18/value");

            break;
        default :
            break;
        }

    } else { // On
        switch(led)
        {
        case 0x00:
            system("echo 0 > /sys/class/gpio/gpio6/value");
            break;
        case 0x01:
            system("echo 0 > /sys/class/gpio/gpio16/value");
            break;
        case 0x02:
            system("echo 0 > /sys/class/gpio/gpio17/value");
            break;
        case 0x03:
            system("echo 0 > /sys/class/gpio/gpio18/value");
            break;
        case 0x04:
            system("echo 0 > /sys/class/gpio/gpio6/value");
            system("echo 0 > /sys/class/gpio/gpio16/value");
            system("echo 0 > /sys/class/gpio/gpio17/value");
            system("echo 0 > /sys/class/gpio/gpio18/value");
            break;
        default :
            break;
        }

    }
}

void MainWindow::slotLampEmerState(void)
{
    if(m_EmergencyLampValue) {
        ui->lbStateEmergency->setStyleSheet("QLabel { background-color : orange; }");
        setGpioLedControl(GP_LED1, B_ON);
        setGpioLedControl(GP_LED2, B_ON);
        m_EmergencyLampValue=false;
    } else {
        ui->lbStateEmergency->setStyleSheet("QLabel { background-color : rgb(240,240,240); }");
        setGpioLedControl(GP_LED1, B_OFF);
        setGpioLedControl(GP_LED2, B_OFF);
        m_EmergencyLampValue=true;
    }
}

void MainWindow::slotLampDirectionState(void)
{
    // Direction LED
    if(m_LampDirState)
        m_LampDirState=false;
    else
        m_LampDirState=true;

    switch(m_LampDirValue)
    {
        case 0:
           if(m_LampDirState) {
               setGpioLedControl(GP_LED0, B_ON);
               setGpioLedControl(GP_LED3, B_OFF);
           } else {
               setGpioLedControl(GP_LED0, B_OFF);
               setGpioLedControl(GP_LED3, B_OFF);
           }
           break;
        case 1:
           setGpioLedControl(GP_LED0, B_OFF);
           setGpioLedControl(GP_LED3, B_OFF);
           break;
        case 2:
           if(m_LampDirState) {
               setGpioLedControl(GP_LED0, B_OFF);
               setGpioLedControl(GP_LED3, B_ON);             
           } else {
               setGpioLedControl(GP_LED0, B_OFF);
               setGpioLedControl(GP_LED3, B_OFF);
           }
           break;
        default:
           break;
    }
}


void MainWindow::slotBtnEmergencyClicked(void)
{

    if(m_EmergencyValue) {
        m_EmergencyValue = false;
        ui->btnLampEmergency->setStyleSheet("QPushButton { background-color : red; }");
        // void sendCanDataPacket(unsigned char id, unsigned char devid, unsigned char func, unsigned char value);
        sendCanDataPacket(0x20, 0x01, 0x30, 0x01,0x01);
        tmrEmergency->start(1000);

    } else {
        m_EmergencyValue = true;
        ui->btnLampEmergency->setStyleSheet("QPushButton { background-color : rgb(240,240,240); }");
        ui->lbStateEmergency->setStyleSheet("QLabel { background-color : rgb(240,240,240); }");
        sendCanDataPacket(0x20, 0x01, 0x30, 0x00,0x01);
        setGpioLedControl(GP_LED1, B_OFF);
        setGpioLedControl(GP_LED2, B_OFF);
        tmrEmergency->stop();
    }
}

void MainWindow::slotBtnMsgClearClicked(void)
{
    ui->txtSystemMessage->clear();
}


void MainWindow::slotBtnDirOffClicked()
{
    slotBtnDirColorReset();
    ui->btnLampDirOff->setStyleSheet("QPushButton { background-color : red; }");
    ui->diaDirection->setValue(1);
    m_LampDirValue = 0x01;
}

void MainWindow::slotBtnDirLeftClicked()
{
    slotBtnDirColorReset();
    ui->btnLampDirLeft->setStyleSheet("QPushButton { background-color : red; }");
    ui->diaDirection->setValue(0);
    m_LampDirValue = 0x00;
}

void MainWindow::slotBtnDirRightClicked()
{
    slotBtnDirColorReset();
    ui->btnLampDirRight->setStyleSheet("QPushButton { background-color : red; }");
    ui->diaDirection->setValue(2);
    m_LampDirValue = 0x02;
}

void MainWindow::slotDialDirectionValueChange(int value)
{
    int setBtnValue;
    value = ui->diaDirection->value();
    setBtnValue = value % 3;

    if(!m_LampDirTimerState)
    {
        m_LampDirTimerState=true;
        tmrDirection->start(500);
    }


    switch(setBtnValue)
    {
    case 0:
        slotBtnDirLeftClicked();
        m_LampDirValue = 0x00;
        break;
    case 1:
        slotBtnDirOffClicked();
        m_LampDirValue = 0x01;
        m_LampDirTimerState=false;
        setGpioLedControl(GP_LED0, B_OFF);
        setGpioLedControl(GP_LED3, B_OFF);
        tmrDirection->stop();
        break;
    case 2:
        slotBtnDirRightClicked();
        m_LampDirValue = 0x02;
        break;
    default:
        break;
    }

    sendCanDataPacket(0x20,0x01,0x20,(char)setBtnValue,0x01);

}

void MainWindow::slotDialLampValueChange(int value)
{
    int setBtnValue;
    value = ui->diaLamp->value();
    setBtnValue = value % 4;

    switch(setBtnValue)
    {
    case 0:
        slotBtnLampOffClicked();
        break;
    case 1:
        slotBtnLampTailClicked();
        break;
    case 2:
        slotBtnLampFrontClicked();
        break;
    case 3:
        slotBtnLampLowClicked();
        break;
    default:
        break;
    }

}


int MainWindow::setI2cDevice(int *fd, int addr)
{
    return ioctl(*fd, I2C_SLAVE, addr);
}

int MainWindow::getI2cTempRegValue(int *fd, double* value,  size_t size)
{
    int retv;
    char regv[2]={0,0};
    retv = read(*fd, regv, size);

    *value = (((regv[0]<<8) | regv[1])>>4) * 0.0625;

    return retv;
}

QString MainWindow::getI2cTemperatureValue(void)
{
    QString strValue;
    double tempValue;
    unsigned char regs[2]={0x00,0x00};
    // Get temperature data
    setI2cDevice(&mI2cFd, TEMP_ADDR);
    setI2cRegister(&mI2cFd,regs[0],regs[1]);
    getI2cTempRegValue(&mI2cFd, &tempValue, 2);
    return strValue.sprintf("%2.1f C",tempValue);
}

// ADC로부터 조도 값을 가져온다.
unsigned char MainWindow::getI2cIlluminRegValue(int *fd)
{
    double temp;

    unsigned char regv;
    read(*fd, &regv, sizeof(regv));

    temp = ((255-regv)/255.0)*100;  // 백분율 계산

    return (unsigned char)temp;
}

// 조도값을 화면에 출력한다
QString MainWindow::getI2cIlluminationValue(void)
{
    int retv;
    QString strValue;

    unsigned char reg = 0;  // Chnnel selected - 0

    setI2cDevice(&mI2cFd, ADC_CDS);  // 0x4C
    retv=setI2cRegister(&mI2cFd, reg);
    if(retv!=1)
        putSystemMessage("[System] ADC error!.");
    retv=getI2cIlluminRegValue(&mI2cFd);

    return strValue.sprintf("%2d %%",retv);
}

int MainWindow::setI2cRegister(int *fd, unsigned char reg)
{
    return write(*fd, &reg,sizeof(reg));
}

int MainWindow::setI2cRegister(int *fd, unsigned char hb, unsigned char lb)
{
    unsigned char regs[2]={0,};
    regs[0] = hb;
    regs[1] = lb;
    write(*fd, regs,sizeof(regs));
}

int MainWindow::setI2cExpGpioRotaryInit(int *fd)
{

    int retv;
    unsigned char regs[2]={0,};

    retv = ioctl(*fd, I2C_SLAVE, MOD_ROTARY);
    usleep(1000);

    regs[0]=0x07;
    regs[1]=0x00; // I2C Ch1 - Output
    write(*fd, regs, sizeof(regs));
    usleep(1000);

    regs[0]=0x03; // Output Register for Ch1
    regs[1]=0x00; // 8Bit(LED0~LED8);
    write(*fd, regs, sizeof(regs));

}


int MainWindow::setI2cExpGpioRotaryLed(int *fd)
{

    int retv;
    unsigned char regs[2]={0,};

    retv = ioctl(*fd, I2C_SLAVE, MOD_ROTARY);
    usleep(1000);

    regs[0]=0x03; // Output Register for Ch1
    regs[1]=m_RotaryLedValue; // 8Bit(LED0~LED8);
    write(*fd, regs, sizeof(regs));

}


unsigned char MainWindow::getI2cExpGpioRotaryValue(int *fd)
{

    int retv;
    unsigned char reg;

    retv = ioctl(*fd, I2C_SLAVE, MOD_ROTARY);
    usleep(1000);

    reg=0x00; // Output Register for Ch1

    read(*fd, &reg, sizeof(reg));

    return reg;

}





void MainWindow::putSystemMessage(QString msg)
{
    ui->txtSystemMessage->append(msg);
}

void MainWindow::slotTimeDisplay(void)
{


    ui->lbDispLabelTop->setText(getI2cTemperatureValue());
    ui->lbDispLabelMiddle->setText(getI2cIlluminationValue());



    switch(m_LampValue)
    {
        case 0: // OFF               1111 1111
            m_RotaryLedValue = 0xFF;
        break;
        case 1: // Tail Lamp         1110 0111
            m_RotaryLedValue = 0xe7;
        break;
        case 2: // Front Lamp        1010 0101
            m_RotaryLedValue = 0xa5;
        break;
        case 3: // Low Lamp          0110 0110
            m_RotaryLedValue = 0x66;
        break;
        default:
        break;
    }
    setI2cExpGpioRotaryLed(&mI2cFd);

    refreshTime();
}

void MainWindow::refreshTime()
{
    char strTempArray[30];

    time(&g_SysRawTime);
    g_SysTimeinfo = localtime(&g_SysRawTime);
    sprintf(strTempArray,"%04d/%02d/%02d", g_SysTimeinfo->tm_year+1900,g_SysTimeinfo->tm_mon,g_SysTimeinfo->tm_mday);
    ui->lbDispDate->setText(strTempArray);
    sprintf(strTempArray,"%02d:%02d:%02d",g_SysTimeinfo->tm_hour,g_SysTimeinfo->tm_min,g_SysTimeinfo->tm_sec);
    ui->lbDispTime->setText(strTempArray);
}




void MainWindow::slotBtnLampLowClicked(void)
{
    slotBtnLampColorReset();
    ui->btnLampLow->setStyleSheet("QPushButton { background-color : red; }");
    ui->diaLamp->setValue(3);
    m_LampValue = 0x03;
}

void MainWindow::slotBtnLampFrontClicked(void)
{
    slotBtnLampColorReset();
    ui->btnLampFront->setStyleSheet("QPushButton { background-color : red; }");
    ui->diaLamp->setValue(2);
    m_LampValue = 0x02;
}

void MainWindow::slotBtnLampTailClicked(void)
{
    slotBtnLampColorReset();
    ui->btnLampTail->setStyleSheet("QPushButton { background-color : red; }");
    ui->diaLamp->setValue(1);
    m_LampValue = 0x01;
}

void MainWindow::slotBtnLampOffClicked(void)
{
    slotBtnLampColorReset();
    ui->btnLampOff->setStyleSheet("QPushButton { background-color : red; }");
    ui->diaLamp->setValue(0);
    m_LampValue = 0x00;
}

void MainWindow::slotBtnLampColorReset(void)
{
    ui->btnLampLow->setStyleSheet("QPushButton { background-color : rgb(240,240,240); }");
    ui->btnLampFront->setStyleSheet("QPushButton { background-color : rgb(240,240,240); }");
    ui->btnLampTail->setStyleSheet("QPushButton { background-color : rgb(240,240,240); }");
    ui->btnLampOff->setStyleSheet("QPushButton { background-color : rgb(240,240,240); }");
}

void MainWindow::slotBtnDirColorReset(void)
{
    ui->btnLampDirOff->setStyleSheet("QPushButton { background-color : rgb(240,240,240); }");
    ui->btnLampDirLeft->setStyleSheet("QPushButton { background-color : rgb(240,240,240); }");
    ui->btnLampDirRight->setStyleSheet("QPushButton { background-color : rgb(240,240,240); }");
}

MainWindow::~MainWindow()
{
    /* Implement a destructor. */

    // GPIO LED OFF
    system("echo 1 > /sys/class/gpio/gpio6/value");
    system("echo 1 > /sys/class/gpio/gpio16/value");
    system("echo 1 > /sys/class/gpio/gpio17/value");
    system("echo 1 > /sys/class/gpio/gpio18/value");

    // Device close
    system("echo 6 > /sys/class/gpio/unexport");
    system("echo 16 > /sys/class/gpio/unexport");
    system("echo 17 > /sys/class/gpio/unexport");
    system("echo 18 > /sys/class/gpio/unexport");

    ::close(mI2cFd);

    delete ui;
}


