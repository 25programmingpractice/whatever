#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr) noexcept;
    ~MainWindow();

private slots:
    void openFile() noexcept;
    void togglePlayback() noexcept;
    void showAbout() noexcept;

private:
    Ui::MainWindow* ui;
    QAudioOutput audio;
    QMediaPlayer player;
};

#endif // MAINWINDOW_H
