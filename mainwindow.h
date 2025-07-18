#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTextEdit>
#include <QStackedWidget>
#include <QMenu>
#include <QSystemTrayIcon>

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
    void playTrack(int index, bool setIndex = true) noexcept;
    void onPlaylistClicked(const QModelIndex& index) noexcept;
    void updateDurationDisplay() noexcept;
    void toggleView() noexcept;
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason) noexcept;
    void toggleMuted() noexcept;
    void dragEnterEvent(QDragEnterEvent *ev) noexcept;

private:
    void setupPlaylist() noexcept;
    void setupConnections() noexcept;
    void updatePlaybackButtons() noexcept;
    void updatePlayingInfo() noexcept;
    void setupLyricsView() noexcept;
    void updateLyricsDisplay() noexcept;
    QString loadLyrics(const QString &filePath) const noexcept;
    QString formatTime(qint64 milliseconds) const noexcept;
    void setupTray() noexcept;
    void dropEvent(QDropEvent* ev) noexcept;

    Ui::MainWindow* ui;
    QAudioOutput audio;
    QMediaPlayer player;
    PlaylistModel playlistModel;
    int currentTrackIndex;

    QStackedWidget viewStack;
    QTextEdit lyricsDisplay;
    bool isLyricsView;

    bool muted;
    int volume_;

    QSystemTrayIcon trayIcon{this};
    QMenu trayMenu{"播放控制", this};
    QAction actPrev{"上一曲", &trayMenu};
    QAction actPlay{"播放", &trayMenu};
    QAction actNext{"下一曲", &trayMenu};
};

#endif // MAINWINDOW_H
