#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QDataStream>
#include <QMessageBox>
#include <QtConcurrent>
#include <QtCharts>
#include <QLineSeries>
#include <QChartView>
#include <QChart>

#define FD 1000.0 // частота дискретизации

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QVector<uint32_t> ReadFile(QString path, uint8_t numberChannel);
    QVector<double> ProcessFile(QVector<uint32_t> dataFile);
    QVector<double> FindMax(QVector<double> resultData);
    QVector<double> FindMin(QVector<double> resultData);
    void DisplayResult(QVector<double> mins, QVector<double> maxs);

signals:
    void dataReadyForPlot(QLineSeries *series);

private slots:
    void on_pb_path_clicked();
    void on_pb_start_clicked();
    void plotData(QLineSeries *series);

private:
    Ui::MainWindow *ui;
    QString pathToFile = "";
    uint8_t numberSelectChannel = 0xEA;

    QVector<uint32_t> readData;
    QVector<double> procesData;
    QVector<double> mins, maxs;

    QChart *chart = nullptr;
    QChartView *chartView = nullptr;
};
#endif // MAINWINDOW_H
