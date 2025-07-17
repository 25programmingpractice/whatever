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
    void onPlaylistDoubleClicked(const QModelIndex &index);
    void updateDurationDisplay();
    void toggleView() noexcept;  // 新增：切换视图

private:
    void setupPlaylist();
    void setupConnections();
    void updatePlaybackButtons();
    void updatePlayingInfo();
    void setupLyricsView();  // 新增：设置歌词视图
    void updateLyricsDisplay();  // 新增：更新歌词显示
    QString loadLyrics(const QString &filePath) const;  // 新增：加载歌词
    QString formatTime(qint64 milliseconds) const;
    
    Ui::MainWindow* ui;
    QAudioOutput audio;
    QMediaPlayer player;
    PlaylistModel playlistModel;
    int currentTrackIndex;
    
    // 新增：视图切换相关
    QStackedWidget* viewStack;
    QTextEdit* lyricsDisplay;
    bool isLyricsView;  // 当前是否为歌词视图
};

#endif // MAINWINDOW_H
