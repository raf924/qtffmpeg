#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(char * filename, QWidget *parent = 0);
    void setImage(uchar * buffer, int len);
    char * filename;

    ~Widget();
protected:
    void paintEvent(QPaintEvent *);
private:
    uchar * buffer;
    int len;
    QImage image;
    Ui::Widget *ui;
};

#endif // WIDGET_H
