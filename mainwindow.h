#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTextEdit>
#include <QStackedWidget>
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
    void onPlaylistClicked(const QModelIndex& index) noexcept;
    void updateDurationDisplay() noexcept;
    void toggleView() noexcept;

private:
    void setupPlaylist() noexcept;
    void setupConnections() noexcept;
    void updatePlaybackButtons() noexcept;
    void updatePlayingInfo() noexcept;
    void setupLyricsView() noexcept;
    void updateLyricsDisplay() noexcept;
    QString loadLyrics(const QString &filePath) const noexcept;
    QString formatTime(qint64 milliseconds) const noexcept;
    
    Ui::MainWindow* ui;
    QAudioOutput audio;
    QMediaPlayer player;
    PlaylistModel playlistModel;
    int currentTrackIndex;

    QStackedWidget* viewStack;
    QTextEdit* lyricsDisplay;
    bool isLyricsView;
};

#endif // MAINWINDOW_H
