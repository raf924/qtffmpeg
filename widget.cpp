#include "widget.h"
#include "ui_widget.h"
#include "videothread.h"

#include <qpainter.h>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    VideoThread * v = new VideoThread(this);
    connect(v,SIGNAL(frameReady()),SLOT(repaint()));
    v->run();
}

void Widget::setImage(const QImage &image)
{
    this->image = image;
}

Widget::~Widget()
{
    delete ui;
}

void Widget::paintEvent()
{
    QPainter p(this);

        //Set the painter to use a smooth scaling algorithm.
    p.setRenderHint(QPainter::SmoothPixmapTransform, 1);

    p.drawImage(this->rect(), image);
}
