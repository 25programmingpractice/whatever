#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include "playlistmodel.h"

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
    void openFolder() noexcept;
    void togglePlayback() noexcept;
    void showAbout() noexcept;
    void previousTrack() noexcept;
    void nextTrack() noexcept;
    void playTrack(int index) noexcept;
    void onPlaylistDoubleClicked(const QModelIndex &index);
    void updateDurationDisplay();

private:
    void setupPlaylist();
    void setupConnections();
    void updatePlaybackButtons();
    QString formatTime(qint64 milliseconds) const;
    
    Ui::MainWindow* ui;
    QAudioOutput audio;
    QMediaPlayer player;
    PlaylistModel playlistModel;
    int currentTrackIndex;
};

#endif // MAINWINDOW_H
