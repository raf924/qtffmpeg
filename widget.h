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
    explicit Widget(QWidget *parent = 0);
    void setImage(const QImage & image);
    ~Widget();
protected:
    void paintEvent();
private:
    QImage image;
private:
    Ui::Widget *ui;
};

#endif // WIDGET_H
