#ifndef VIDEOTHREAD_H
#define VIDEOTHREAD_H

#include "widget.h"

#include <QThread>


class VideoThread : public QThread
{
    Q_OBJECT
public:
    explicit VideoThread(Widget * w,QObject *parent = 0);

private:
    Widget * _w;
protected:
    void run();
signals:
    void frameReady();

public slots:

};

#endif // VIDEOTHREAD_H
