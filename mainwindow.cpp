#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , readyCaputure(false)
    , device(nullptr)
{
    ui->setupUi(this);
    /* device */
    device = new MLX90640(24, 32, [&](int h, int w, unsigned char* data){
        cv::Mat img(h, w, CV_8UC3, data);
        /* gaussian filter */
        cv::GaussianBlur(img, img, cv::Size(3, 3), 0);
        /* resize */
        cv::resize(img, imgs, cv::Size(640, 480), 0, 0, cv::INTER_CUBIC);
        /* max temperature */
        MLX90640::Temperature &temp = device->maxTemp;
        temp.value /= 100;
        temp.i *= 20;
        temp.j *= 20;
        cv::putText(imgs,
                    std::to_string(temp.value),
                    cv::Point(temp.i, temp.j),
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.5,
                    cv::Scalar(255, 255, 255));
        /* send image */
        emit sendImage(QImage(imgs.data, 640, 480, QImage::Format_RGB888));
    });
    device->setColorScalar(0.8, 0.1, 0.1);
    /* connect */
    /* update image */
    connect(this, &MainWindow::sendImage,
            this, &MainWindow::updateImage, Qt::QueuedConnection);
    /* open device */
    connect(ui->startBtn, &QPushButton::clicked, this, [=](){
        if (ui->devComboBox->count() == 0) {
            return;
        }

        QString portName = ui->devComboBox->currentText();
        device->open(portName.toStdString(), MLX90640::BAUDRATE_460800);

        /* parameters */
        device->setEmissivity(0.95);
    });
    /* close device */
    connect(ui->stopBtn, &QPushButton::clicked, this, [=](){
        device->close();
        return;
    });
    /* capture */
    connect(ui->captureBtn, &QPushButton::clicked, this, [&](){
        if (readyCaputure == false) {
            readyCaputure = true;
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
    if (device != nullptr) {
        delete device;
    }
}

void MainWindow::updateImage(const QImage &img)
{
    if (readyCaputure == true) {
        QString fileName = QString("img_%1.jpg").arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));
        img.save(fileName);
        statusBar()->showMessage(QString("save image:%1").arg(fileName));
        readyCaputure = false;
    }
    QImage img_ = img.scaled(ui->videoLabel->size());
    ui->videoLabel->setPixmap(QPixmap::fromImage(img_));
    return;
}

