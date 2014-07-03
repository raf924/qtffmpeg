#include "widget.h"
#include "ui_widget.h"
#include "videothread.h"

#include <qpainter.h>
#include <QDebug>

Widget::Widget(char * filename, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    filename(filename)
{
    ui->setupUi(this);
}

void Widget::setImage(uchar *buffer, int len)
{
    this->buffer = buffer;
    this->len = len;
}

Widget::~Widget()
{
    delete ui;
}

void Widget::paintEvent(QPaintEvent *)
{
    if(buffer){
        qDebug()<<"Painting frame";
        QPainter p(this);

        image = QImage::fromData(buffer,len,"PPM");

            //Set the painter to use a smooth scaling algorithm.
        //p.setRenderHint(QPainter::SmoothPixmapTransform, 1);

        p.drawImage(QPoint(0,0), image);
    }
}
