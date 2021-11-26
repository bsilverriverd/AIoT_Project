#include "optiondlg.h"
#include "ui_optiondlg.h"

OptionDlg::OptionDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionDlg)
{
    ui->setupUi(this);
}

OptionDlg::~OptionDlg()
{
    delete ui;
}
