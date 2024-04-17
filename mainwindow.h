#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QDateTime>
#include <QTimer>
#include <QCloseEvent>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <atomic>
#include "mlx90640.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
signals:
    void sendImage(const QImage &img);
public slots:
    void updateImage(const QImage &img);
protected:
    void closeEvent(QCloseEvent *ev) override;
private:
    Ui::MainWindow *ui;
    cv::Mat temperatureImage;
    std::atomic_bool readyCaputure;
    MLX90640 *device;
};
#endif // MAINWINDOW_H
