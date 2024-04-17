#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDesktopServices>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , readyCaputure(0)
    , device(nullptr)
{
    ui->setupUi(this);
    /* device */
    device = new MLX90640([=](int h, int w, unsigned char* data){
        cv::Mat img(h, w, CV_8UC3, data);
        /* gaussian filter */
        cv::GaussianBlur(img, img, cv::Size(3, 3), 0);
        /* resize */
        cv::resize(img, temperatureImage, cv::Size(640, 480), 0, 0, cv::INTER_CUBIC);
        /* max temperature */
        MLX90640::Temperature &temp = device->maxTemp;
        temp.value /= 100;
        temp.i *= 20;
        temp.j *= 20;
        cv::putText(temperatureImage,
                    std::to_string(temp.value),
                    cv::Point(temp.i, temp.j),
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.5,
                    cv::Scalar(255, 255, 255));
        /* send image */
        emit sendImage(QImage(temperatureImage.data, 640, 480, QImage::Format_RGB888));
    });
    /* connect */
    /* update image */
    connect(this, &MainWindow::sendImage,
            this, &MainWindow::updateImage, Qt::QueuedConnection);
    /* open device */
    connect(ui->startBtn, &QPushButton::clicked, this, [=](){
        if (ui->devComboBox->count() == 0) {
            QMessageBox::warning(this, "ERROR", "NO DEVICE.");
            return;
        }
        QString path = ui->devComboBox->currentText();
        if (!device->openDevice(path.toStdString(), MLX90640::BAUDRATE_460800)) {
            QMessageBox::warning(this, "ERROR", "FAILED TO OPEN DEVICE.");
            return;
        }
        /* parameters */
        device->setEmissivity(0.95);
    });
    /* close device */
    connect(ui->stopBtn, &QPushButton::clicked, this, [=](){
        device->closeDevice();
        return;
    });
    /* capture */
    connect(ui->captureBtn, &QPushButton::clicked, this, [&](){
        if (!readyCaputure.load()) {
            readyCaputure.store(1);
        }
    });
    /* enumerate device */
    std::vector<std::string> devList = YSerialPort::enumerate();
    for (auto &portName : devList) {
        ui->devComboBox->addItem(QString::fromStdString(portName));
    }

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateImage(const QImage &img)
{
    if (readyCaputure.load()) {
        readyCaputure.store(0);
        QString fileName = QString("img_%1.jpg").arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));
        img.save(fileName);
        statusBar()->showMessage(QString("save image:%1").arg(fileName));
        QDesktopServices::openUrl(QUrl(fileName));
    }
    QImage img_ = img.scaled(ui->videoLabel->size());
    ui->videoLabel->setPixmap(QPixmap::fromImage(img_));
    return;
}

void MainWindow::closeEvent(QCloseEvent *ev)
{
    if (device != nullptr) {
        device->closeDevice();
        delete device;
        device = nullptr;
    }
    return QMainWindow::closeEvent(ev);
}

