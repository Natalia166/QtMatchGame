#include "sec.h"
#include "ui_sec.h"
#include <QDebug>
#include <QSound>
#include <QAction>
#include <QMessageBox>
#include <QPainter>
#include <QLine>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QMessageBox>
// --------- 全局变量 --------- //
const int kIconSize = 72;//图标大小
const int kTopMargin = 70;
const int kLeftMargin = 50;

const QString kIconReleasedStyle = "";
const QString kIconClickedStyle = "background-color: rgba(255, 255, 12, 161)";
const QString kIconHintStyle = "background-color: rgba(255, 0, 0, 255)";


const int kGameTimeTotal = 6*60*1000; // 总时间 毫秒
const int kGameTimerInterval = 300;
const int kLinkTimerDelay = 700;
// -------------------------- //
sec::sec(QWidget *parent) ://构造函数
    QMainWindow(parent),
    ui(new Ui::sec),
    preIcon(NULL),
    curIcon(NULL)
{
    ui->setupUi(this);

    //设置中央部件的背景颜色为白色
    ui->centralWidget->setStyleSheet("background-color:rgb(255,255,255);");

    // 重载eventfilter安装到当前window（其实如果不适用ui文件的话直接可以在window的paintevent里面画）
    ui->centralWidget->installEventFilter(this);

//    setFixedSize(kLeftMargin * 2 + (kIconMargin + kIconSize) * MAX_COL, kTopMargin + (kIconMargin + kIconSize) * MAX_ROW);
    // 选关信号槽
    connect(ui->actionBasic, SIGNAL(triggered(bool)), this, SLOT(createGameWithLevel()));
    connect(ui->actionMedium, SIGNAL(triggered(bool)), this, SLOT(createGameWithLevel()));
    connect(ui->actionHard, SIGNAL(triggered(bool)), this, SLOT(createGameWithLevel()));

    // 初始化游戏
    initGame(BASIC);
}
sec::~sec()
{
    if (game)
        delete game;

    delete ui;
}

void sec::initGame(GameLevel level)
{
    // 启动游戏

    game = new GameModel;
    game->startGame(level);

    // 添加button
    for(int i = 0; i < MAX_ROW * MAX_COL; i++)
    {
        imageButton[i] = new IconButton(this);
        imageButton[i]->setGeometry(kLeftMargin + (i % MAX_COL) * kIconSize, kTopMargin
                                    + (i / MAX_COL) * kIconSize, kIconSize, kIconSize);//图片大小设置
        // 设置索引
        imageButton[i]->xID = i % MAX_COL;
        imageButton[i]->yID = i / MAX_COL;

        imageButton[i]->show();

        if (game->getGameMap()[i])
        {
            // 有方块就设置图片
            QPixmap iconPix;
            QString fileString;
            fileString.sprintf(":/new/prefix1/image/%d.png", game->getGameMap()[i]);
            iconPix.load(fileString);
            QIcon icon(iconPix);
            imageButton[i]->setIcon(icon);
            imageButton[i]->setIconSize(QSize(kIconSize, kIconSize));
            //imageButton[i]->setStyleSheet("background-color:white;");//设置背景为白色

            // 添加按下的信号槽
            connect(imageButton[i], SIGNAL(pressed()), this, SLOT(onIconButtonPressed()));
        }
        else
            imageButton[i]->hide();
    }

    // 进度条
    ui->timeBar->setMaximum(kGameTimeTotal);
    ui->timeBar->setMinimum(0);
    ui->timeBar->setValue(kGameTimeTotal);

    // 游戏计时器
    gameTimer = new QTimer(this);
    connect(gameTimer, SIGNAL(timeout()), this, SLOT(gameTimerEvent()));
    gameTimer->start(kGameTimerInterval);

    // 连接状态值
    isLinking = false;

    // 播放背景音乐(QMediaPlayer只能播放绝对路径文件),确保res文件在程序执行文件目录里而不是开发目录
    audioPlayer = new QMediaPlayer(this);
    QString curDir = QCoreApplication::applicationDirPath(); // 这个api获取路径在不同系统下不一样,mac 下需要截取路径
    QStringList sections = curDir.split(QRegExp("[/]"));
    QString musicPath;

    for (int i = 0; i < sections.size() - 3; i++)
        musicPath += sections[i] + "/";

    audioPlayer->setMedia(QUrl::fromLocalFile(musicPath + ":/sound/backgrand.mp3"));
    audioPlayer->play();
}

void sec::onIconButtonPressed()
{
    // 如果当前有方块在连接，不能点击方块
    // 因为涉及到多线，可能还要维护队列，有点复杂，就先这么简单处理一下
    if (isLinking)
    {
        // 播放音效
        QSound::play(":/new/prefix1/sound/release.wav");
        return;
    }


    // 记录当前点击的icon
    curIcon = dynamic_cast<IconButton *>(sender());

    if(!preIcon)
    {
        // 播放音效
        QSound::play(":/new/prefix1/sound/select.wav");

        // 如果单击一个icon
        curIcon->setStyleSheet(kIconClickedStyle);
        preIcon = curIcon;
    }
    else
    {
        if(curIcon != preIcon)
        {
            // 如果不是同一个button就都标记,尝试连接
            curIcon->setStyleSheet(kIconClickedStyle);
            if(game->linkTwoTiles(preIcon->xID, preIcon->yID, curIcon->xID, curIcon->yID))
            {
                // 锁住当前状态
                isLinking = true;

                // 播放音效
                QSound::play(":/new/prefix1/sound/pair.wav");

                // 重绘
                update();

                // 延迟后实现连接效果
                QTimer::singleShot(kLinkTimerDelay, this, SLOT(handleLinkEffect()));

                // 检查是否胜利
                if (game->isWin()){
                    QMessageBox::information(this, "great", "you win");
                    return;//结束操作
                }

                // 每次检查一下是否僵局
                if (game->isFrozen())
                {
                    QMessageBox::information(this, "oops", "dead game");
                    return ;//over
                }

                int *hints = game->getHint();
            }
            else
            {
                // 播放音效
                QSound::play(":/new/prefix1/sound/release.wav");

                // 消除失败，恢复
                preIcon->setStyleSheet(kIconReleasedStyle);
                curIcon->setStyleSheet(kIconReleasedStyle);

                // 指针置空，用于下次点击判断
                preIcon = NULL;
                curIcon = NULL;
            }
        }
        else if(curIcon == preIcon)
        {
            // 播放音效
            QSound::play(":/new/prefix1/sound/release.wav");

            preIcon->setStyleSheet(kIconReleasedStyle);
            curIcon->setStyleSheet(kIconReleasedStyle);
            preIcon = NULL;
            curIcon = NULL;
        }
    }
}

void sec::handleLinkEffect()
{
    // 消除成功，隐藏掉，并析构
    game->paintPoints.clear();
    preIcon->hide();
    curIcon->hide();

    preIcon = NULL;
    curIcon = NULL;

    // 重绘
    update();

    // 恢复状态
    isLinking = false;
}

bool sec::eventFilter(QObject *watched, QEvent *event)
{
    // 重绘时会调用，可以手动调用
    if (event->type() == QEvent::Paint)
    {
        QPainter painter(ui->centralWidget);
        QPen pen;
        QColor color(rand() % 256, rand() % 256, rand() % 256);
        pen.setColor(color);
        pen.setWidth(5);
        painter.setPen(pen);

        QString str;
        for (int i = 0; i < game->paintPoints.size(); i++)
        {
            PaintPoint p = game->paintPoints[i];
            str += "x:" + QString::number(p.x) + "y:" + QString::number(p.y) + "->";
        }
//        qDebug() << str;

        // 连接各点画线（注，qt中用标砖vector的size好像有点问题，需要类型转换，否则溢出）
        for (int i = 0; i < int(game->paintPoints.size()) - 1; i++)
        {
            PaintPoint p1 = game->paintPoints[i];
            PaintPoint p2 = game->paintPoints[i + 1];

            // 拿到各button的坐标,注意边缘点坐标
            QPoint btn_pos1;
            QPoint btn_pos2;

            // p1
            if (p1.x == -1)
            {
                btn_pos1 = imageButton[p1.y * MAX_COL + 0]->pos();
                btn_pos1 = QPoint(btn_pos1.x() - kIconSize, btn_pos1.y());
            }
            else if (p1.x == MAX_COL)
            {
                btn_pos1 = imageButton[p1.y * MAX_COL + MAX_COL - 1]->pos();
                btn_pos1 = QPoint(btn_pos1.x() + kIconSize, btn_pos1.y());
            }
            else if (p1.y == -1)
            {
                btn_pos1 = imageButton[0 + p1.x]->pos();
                btn_pos1 = QPoint(btn_pos1.x(), btn_pos1.y() - kIconSize);
            }
            else if (p1.y == MAX_ROW)
            {
                btn_pos1 = imageButton[(MAX_ROW - 1) * MAX_COL + p1.x]->pos();
                btn_pos1 = QPoint(btn_pos1.x(), btn_pos1.y() + kIconSize);
            }
            else
                btn_pos1 = imageButton[p1.y * MAX_COL + p1.x]->pos();

            // p2
            if (p2.x == -1)
            {
                btn_pos2 = imageButton[p2.y * MAX_COL + 0]->pos();
                btn_pos2 = QPoint(btn_pos2.x() - kIconSize, btn_pos2.y());
            }
            else if (p2.x == MAX_COL)
            {
                btn_pos2 = imageButton[p2.y * MAX_COL + MAX_COL - 1]->pos();
                btn_pos2 = QPoint(btn_pos2.x() + kIconSize, btn_pos2.y());
            }
            else if (p2.y == -1)
            {
                btn_pos2 = imageButton[0 + p2.x]->pos();
                btn_pos2 = QPoint(btn_pos2.x(), btn_pos2.y() - kIconSize);
            }
            else if (p2.y == MAX_ROW)
            {
                btn_pos2 = imageButton[(MAX_ROW - 1) * MAX_COL + p2.x]->pos();
                btn_pos2 = QPoint(btn_pos2.x(), btn_pos2.y() + kIconSize);
            }
            else
                btn_pos2 = imageButton[p2.y * MAX_COL + p2.x]->pos();



            // 中心点
            QPoint pos1(btn_pos1.x() + kIconSize / 2, btn_pos1.y() + kIconSize / 2);
            QPoint pos2(btn_pos2.x() + kIconSize / 2, btn_pos2.y() + kIconSize / 2);

            painter.drawLine(pos1, pos2);
        }

        return true;
    }
    else
        return QMainWindow::eventFilter(watched, event);
}

void sec::gameTimerEvent()
{
    // 进度条计时效果
    if(ui->timeBar->value() == 0)
    {
        gameTimer->stop();

        // 弹出消息框，点击 "play again>_<" 时退出程序
        QMessageBox msgBox;
        msgBox.setText("game over");
        msgBox.setInformativeText(">_<");
        QPushButton *playAgainButton = msgBox.addButton("over game", QMessageBox::AcceptRole);
        msgBox.exec();

        // 如果点击了按钮，则关闭所有窗口并退出
        if (msgBox.clickedButton() == playAgainButton)
        {
            QApplication::quit();  // 退出应用程序，关闭所有窗口
        }
        return;
    }
    else
    {
        ui->timeBar->setValue(ui->timeBar->value() - kGameTimerInterval);
    }
}


// 提示
void sec::on_hintBtn_clicked()
{
    this->close();
//    // 初始时不能获得提示
//    for (int i = 0; i < 4;i++)
//        if (game->getHint()[i] == -1)
//            return;

//    int srcX = game->getHint()[0];
//    int srcY = game->getHint()[1];
//    int dstX = game->getHint()[2];
//    int dstY = game->getHint()[3];

//    IconButton *srcIcon = imageButton[srcY * MAX_COL + srcX];
//    IconButton *dstIcon = imageButton[dstY * MAX_COL + dstX];
//    srcIcon->setStyleSheet(kIconHintStyle);
//    dstIcon->setStyleSheet(kIconHintStyle);

}

void sec::on_robot_btn_clicked()
{
    // 初始时不能自动玩
    for (int i = 0; i < 4;i++)
        if (game->getHint()[i] == -1)
            return;

    while (game->gameStatus == PLAYING)
    {
        // 连接生成提示

        int srcX = game->getHint()[0];
        int srcY = game->getHint()[1];
        int dstX = game->getHint()[2];
        int dstY = game->getHint()[3];

        if(game->linkTwoTiles(srcX, srcY, dstX, dstY))
        {
            // 播放音效
//            QSound::play(":/new/prefix1/sound/pair.wav");

            // 消除成功，隐藏掉
            IconButton *icon1 = imageButton[srcY * MAX_COL + srcX];
            IconButton *icon2 = imageButton[dstY * MAX_COL + dstX];

            icon1->hide();
            icon2->hide();

            game->paintPoints.clear();

            // 重绘
            update();

            // 检查是否胜利
            if (game->isWin())
                QMessageBox::information(this, "great", "you win");
            else return;//未赢则退出 玩家继续游戏

            // 每次检查一下是否僵局
            if (game->isFrozen() && game->gameStatus == PLAYING)
                QMessageBox::information(this, "oops", "dead game");



            int *hints = game->getHint();
        }
    }
}

void sec::createGameWithLevel()
{
    // 先析构之前的
    if (game)
    {
        delete game;
        for (int i = 0;i < MAX_ROW * MAX_COL; i++)
        {
            if (imageButton[i])
               delete imageButton[i];
        }
    }

    // 停止音乐
    audioPlayer->stop();

    // 重绘
    update();

    QAction *actionSender = (QAction *)dynamic_cast<QAction *>(sender());
    if (actionSender == ui->actionBasic)
    {
        initGame(BASIC);
    }
    else if (actionSender == ui->actionMedium)
    {
        initGame(MEDIUM);
    }
    else if (actionSender == ui->actionHard)
    {
        initGame(HARD);
    }

}
