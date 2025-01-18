#include "widget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;

    //可以通过设置窗口标题或大小来定制窗口
    w.setWindowTitle("Match Game");
    w.resize(1550,1250);
    w.show();

    return a.exec();
}
