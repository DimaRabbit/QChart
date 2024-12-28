#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->pb_clearResult->setCheckable(true);

    connect(this, &MainWindow::dataReadyForPlot, this, &MainWindow::plotData);
}

MainWindow::~MainWindow()
{
    delete ui;
}

QVector<uint32_t> MainWindow::ReadFile(QString path, uint8_t numberChannel)
{
    QFile file(path);
    file.open(QIODevice::ReadOnly);

    if (!file.isOpen()) {
        if (pathToFile.isEmpty()) {
            QMessageBox::critical(this, "Ошибка", "Ошибка открытия файла");
        }
        return {};
    }

    QDataStream dataStream(&file);
    dataStream.setByteOrder(QDataStream::LittleEndian);

    QVector<uint32_t> readData;
    uint32_t currentWord = 0, sizeFrame = 0;

    while (!dataStream.atEnd()) {
        dataStream >> currentWord;
        if (currentWord == 0xFFFFFFFF) {
            dataStream >> currentWord;
            if (currentWord < 0x80000000) {
                dataStream >> sizeFrame;
                if (sizeFrame > 1500) continue;

                for (int i = 0; i < sizeFrame / sizeof(uint32_t); ++i) {
                    dataStream >> currentWord;
                    if ((currentWord >> 24) == numberChannel) {
                        readData.append(currentWord);
                    }
                }
            }
        }
    }

    ui->chB_readSucces->setChecked(true);
    return readData;
}

QVector<double> MainWindow::ProcessFile(const QVector<uint32_t> dataFile)
{
    QVector<double> resultData;

    for (int word : dataFile) {
        word &= 0x00FFFFFF;
        if (word > 0x800000) word -= 0x1000000;

        double res = ((double)word / 6000000) * 10;
        resultData.append(res);
    }

    ui->chB_procFileSucces->setChecked(true);
    return resultData;
}

QVector<double> MainWindow::FindMax(QVector<double> resultData)
{
    double max1 = *std::max_element(resultData.begin(), resultData.end());
    double max2 = 0;
    for (double val : resultData) {
        if (val != max1 && val > max2) max2 = val;
    }

    ui->chB_maxSucess->setChecked(true);
    return {max1, max2};
}

QVector<double> MainWindow::FindMin(QVector<double> resultData)
{
    double min1 = *std::min_element(resultData.begin(), resultData.end());
    double min2 = 0;
    for (double val : resultData) {
        if (val != min1 && val < min2) min2 = val;
    }

    ui->chB_minSucess->setChecked(true);
    return {min1, min2};
}

void MainWindow::DisplayResult(QVector<double> mins, QVector<double> maxs)
{
    ui->te_Result->append("Расчет закончен!");
    ui->te_Result->append("Первый минимум: " + QString::number(mins.first()));
    ui->te_Result->append("Второй минимум: " + QString::number(mins.last()));
    ui->te_Result->append("Первый максимум: " + QString::number(maxs.first()));
    ui->te_Result->append("Второй максимум: " + QString::number(maxs.last()));
}

void MainWindow::plotData(QLineSeries *series)
{
    if (chart) delete chart;

    chart = new QChart();
    chart->addSeries(series);
    chart->createDefaultAxes();
    chart->setTitle("График данных первой секунды");

    if (!chartView) {
        chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        chartView->resize(800, 600);
        chartView->setWindowTitle("График");
    } else {
        chartView->setChart(chart);
    }

    chartView->show();
}

void MainWindow::on_pb_path_clicked()
{
    pathToFile = QFileDialog::getOpenFileName(this, tr("Открыть файл"), "/home/", tr("ADC Files (*.adc)"));
    ui->le_path->setText(pathToFile);
}

void MainWindow::on_pb_start_clicked()
{
    if (pathToFile.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Выберите файл!");
        return;
    }

    ui->chB_maxSucess->setChecked(false);
    ui->chB_procFileSucces->setChecked(false);
    ui->chB_readSucces->setChecked(false);
    ui->chB_minSucess->setChecked(false);

    int selectIndex = ui->cmB_numCh->currentIndex();
    numberSelectChannel = selectIndex == 0 ? 0xEA : selectIndex == 1 ? 0xEF : 0xED;

    auto read = [&]() { return ReadFile(pathToFile, numberSelectChannel); };
    auto process = [&](QVector<uint32_t> res) { return ProcessFile(res); };
    auto findMax = [&](QVector<double> res) {
        maxs = FindMax(res);
        mins = FindMin(res);
        DisplayResult(mins, maxs);

        QLineSeries *series = new QLineSeries();
        for (int i = 0; i < FD && i < res.size(); ++i) {
            series->append(i / FD, res[i]);
        }

        emit dataReadyForPlot(series);
    };

    QtConcurrent::run(read).then(process).then(findMax);
}
