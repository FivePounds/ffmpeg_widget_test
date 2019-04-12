#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QThread>
#include <QSignalBlocker>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->resize(1000,800);
    m_thread = new QThread(this);
    m_ff = new Myffmpeg();
//    connect(m_ff,&Myffmpeg::getFrame,this,&MainWindow::showFrame,Qt::QueuedConnection);
    m_timer = new QTimer(this);
    m_timer->setInterval(40);
    connect(m_timer,&QTimer::timeout,this,&MainWindow::showFrame);
}

MainWindow::~MainWindow()
{
    delete ui;
    m_ff->stop();
    m_thread->quit();
    m_thread->wait();
    m_ff->deleteLater();
}

void MainWindow::showFrame()
{
//    QSignalBlocker blocker(this->sender());
    QImage image = m_ff->getLatestImage();
    if(ui->label->width() != image.width())
    {
        ui->label->resize(image.size());
    }
    ui->label->setPixmap(QPixmap::fromImage(image));
}

void MainWindow::on_pushButton_clicked()
{
    m_ff->startPlay("rtsp://192.168.1.88:554/av0_0");
    m_timer->start();
}

void MainWindow::on_pushButton_2_clicked()
{
    m_ff->stop();
    m_timer->stop();
}
