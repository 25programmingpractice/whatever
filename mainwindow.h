#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>

using std::unique_ptr, std::make_unique;

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
    unique_ptr<QAudioOutput> audio;
    unique_ptr<QMediaPlayer> player;
};
#endif // MAINWINDOW_H
