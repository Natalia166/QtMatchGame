#include "widget.h"
#include "ui_widget.h"
#include <sec.h>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_pushButton_clicked()
{
    this->close();
    sec *configWindow = new sec;
    configWindow->show();
}

void Widget::on_pushButton_2_clicked()
{
    this->close();
}
