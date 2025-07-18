#include <QFileDialog>
#include <QMessageBox>
#include <QUrl>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QStackedWidget>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>
#include <QDebug>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDirIterator>

#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent) noexcept :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    audio(this),
    player(this),
    playlistModel(this),
    currentTrackIndex(-1),
    isLyricsView(false),
    muted(false),
    volume_(50)
{
    ui->setupUi(this);
    setCentralWidget(ui->central_widget);
    ui->central_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QWidget::setMinimumSize(400 * devicePixelRatio(), 200 * devicePixelRatio());
    if (auto* root = ui->central_widget->layout()) {
        root->setContentsMargins(15, 10, 15, 15);
        root->setSpacing(10);
        root->setSizeConstraint(QLayout::SetDefaultConstraint);
    }
    ui->menubar->setWindowFlag(Qt::NoDropShadowWindowHint);
    ui->menubar->setAttribute(Qt::WA_TranslucentBackground, false);

    player.setAudioOutput(&audio);
    audio.setVolume(0.5f);

    setupPlaylist();
    setupLyricsView();
    setupConnections();
    setupTray();

    updatePlaybackButtons();
}

void MainWindow::setupPlaylist() noexcept {
    ui->music_list->setModel(&playlistModel);
    ui->music_list->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->music_list->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->music_list->setAlternatingRowColors(true);

    ui->music_list->setColumnWidth(0, 300);
    ui->music_list->setColumnWidth(1, 150);
    ui->music_list->setColumnWidth(2, 150);
    ui->music_list->setColumnWidth(3, 80);
}

void MainWindow::setupLyricsView() noexcept {
    viewStack = new QStackedWidget(this);

    lyricsDisplay = new QTextEdit(this);
    lyricsDisplay->setReadOnly(true);
    lyricsDisplay->setAlignment(Qt::AlignCenter);
    lyricsDisplay->setPlainText("暂无歌词");
    lyricsDisplay->setStyleSheet("QTextEdit { font-size: 14px; line-height: 1.5; padding: 20px; }");

    viewStack->addWidget(ui->music_list);
    viewStack->addWidget(lyricsDisplay);

    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(ui->central_widget->layout());
    if (mainLayout) {
        mainLayout->removeWidget(ui->music_list);
        mainLayout->addWidget(viewStack);
    }

    viewStack->setCurrentWidget(ui->music_list);
}

void MainWindow::setupConnections() noexcept {
    connect(ui->action_add_file, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->action_add_folder, &QAction::triggered, this, &MainWindow::openFolder);
    connect(ui->action_about, &QAction::triggered, this, &MainWindow::showAbout);
    connect(ui->action_exit, &QAction::triggered, this, &QApplication::quit);

    connect(ui->play_pause, &QPushButton::clicked, this, &MainWindow::togglePlayback);
    connect(ui->previous_music, &QPushButton::clicked, this, &MainWindow::previousTrack);
    connect(ui->next_music, &QPushButton::clicked, this, &MainWindow::nextTrack);
    connect(ui->view_toggle, &QPushButton::clicked, this, &MainWindow::toggleView);

    connect(ui->music_list, &QTableView::clicked, this, &MainWindow::onPlaylistClicked);

    connect(&player, &QMediaPlayer::durationChanged, this, [this](qint64 d){
        ui->music_progress->setRange(0, int(d));
        updateDurationDisplay();
    });

    connect(&player, &QMediaPlayer::positionChanged, this, [this](qint64 p){
        ui->music_progress->setValue(int(p));
        ui->current_duration->setText(formatTime(p));
    });

    connect(&player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status){
        if (status == QMediaPlayer::EndOfMedia) nextTrack();
    });

    connect(&player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state){
        updatePlaybackButtons();
    });

    playlistModel.loadPlayList();

    connect(ui->music_progress, &QSlider::sliderMoved, &player, &QMediaPlayer::setPosition);

    connect(ui->music_progress, &QSlider::sliderPressed, this, [this](){
        disconnect(&player, &QMediaPlayer::positionChanged, this, nullptr);
    });

    connect(ui->music_progress, &QSlider::sliderReleased, this, [this](){
        player.setPosition(ui->music_progress->value());
        connect(&player, &QMediaPlayer::positionChanged, this, [this](qint64 p){
            ui->music_progress->setValue(int(p));
            ui->current_duration->setText(formatTime(p));
        });
    });
    connect(ui->music_progress, &QSlider::valueChanged, this, [this](int value){
        ui->current_duration->setText(formatTime(value));
    });

    connect(ui->volume, &QSlider::valueChanged, this, [this](int v){
        if(v > 0 && muted) {
            muted = false;
            ui->mute->setIcon(QIcon(":/assets/material-symbols--volume-up-rounded.png"));
            ui->mute->setToolTip("静音");
        }
        else if(v == 0 && !muted) {
            muted = true;
            ui->mute->setIcon(QIcon(":/assets/material-symbols--volume-off-rounded.png"));
            ui->mute->setToolTip("取消静音");
        }
        audio.setVolume(v / 100.0f);
    });

    connect(ui->mute, &QPushButton::clicked, this, &MainWindow::toggleMuted);

    connect(&playlistModel, &QAbstractItemModel::rowsInserted, this, [this](){
        updatePlaybackButtons();
    });
    connect(&playlistModel, &QAbstractItemModel::rowsRemoved, this, [this](){
        updatePlaybackButtons();
    });
}

void MainWindow::openFile() noexcept {
    const QStringList files = QFileDialog::getOpenFileNames(this, "选择音乐文件", QDir::homePath(), "音乐文件 (*.mp3 *.flac *.aac *.wav *.m4a *.ogg *.wma *.mgg)");
    if (!files.isEmpty()) for(int i = 0; i < files.size(); i++) playlistModel.addMusicFile(files[i]);
}

void MainWindow::openFolder() noexcept {
    const QString folderPath = QFileDialog::getExistingDirectory(this, "选择音乐文件夹", QDir::homePath());
    if (!folderPath.isEmpty()) playlistModel.addMusicFolder(folderPath);
}

void MainWindow::togglePlayback() noexcept {
    QMediaPlayer::PlaybackState state = player.playbackState();
    if (state == QMediaPlayer::PlayingState) player.pause();
    else if (state == QMediaPlayer::PausedState) player.play();
    else {
        if (currentTrackIndex == -1 && playlistModel.getTrackCount() > 0) playTrack(0);
        else if (currentTrackIndex >= 0 && currentTrackIndex < playlistModel.getTrackCount()) playTrack(currentTrackIndex);
        else if (playlistModel.getTrackCount() > 0) playTrack(0);
    }
}

void MainWindow::previousTrack() noexcept {
    if (playlistModel.getTrackCount() == 0) return;
    if (currentTrackIndex > 0) playTrack(currentTrackIndex - 1);
    else playTrack(playlistModel.getTrackCount() - 1);
}

void MainWindow::nextTrack() noexcept {
    if (playlistModel.getTrackCount() == 0) return;
    if (currentTrackIndex < playlistModel.getTrackCount() - 1) playTrack(currentTrackIndex + 1);
    else playTrack(0);
}

void MainWindow::playTrack(int index) noexcept {
    if (index < 0 || index >= playlistModel.getTrackCount()) return;
    const auto* file = playlistModel.getTrack(index);
    if (file != nullptr) {
        currentTrackIndex = index;
        player.setSource(QUrl::fromLocalFile(file->filePath));
        player.play();
        QFileInfo fileInfo(file->filePath);
        setWindowTitle(fileInfo.baseName() + " - Whatever");
        if (QSystemTrayIcon::supportsMessages()) {
            QIcon smallArt;
            if(!file->cover.isNull()) {
                QPixmap temp(QPixmap::fromImage(file->cover));
                smallArt.addPixmap(temp);
            }
            if(smallArt.isNull()) trayIcon.showMessage("正在播放", file->artist + " - " + file->title, QSystemTrayIcon::Information, 2000);
            else trayIcon.showMessage("正在播放", file->artist + " - " + file->title, smallArt, 5000);
        }
        ui->music_list->selectRow(index);
        updatePlayingInfo();
    }
}

void MainWindow::updatePlayingInfo() noexcept {
    if (currentTrackIndex >= 0 && currentTrackIndex < playlistModel.getTrackCount()) {
        const auto* file = playlistModel.getTrack(currentTrackIndex);
        ui->metadata->setText(file->artist + " - " + file->title);
        if (isLyricsView) updateLyricsDisplay();
        if (!file->cover.isNull()) ui->album_cover->setPixmap(QPixmap::fromImage(file->cover));
        else {
            bool found = false;
            const QDir dir{QFileInfo(file->filePath).absolutePath()};
            static const QStringList names = { "cover.jpg", "Cover.jpg", "folder.jpg", "Folder.jpg" };
            for (const QString &n : names) {
                const QString file_ = dir.filePath(n);
                if (QFile::exists(file_)) {
                    QImage img(file_);
                    if (!img.isNull()) {
                        ui->album_cover->setPixmap(QPixmap::fromImage(img));
                        found = true;
                        break;
                    }
                }
            }
            if(!found) ui->album_cover->setPixmap(QPixmap(":/assets/material-symbols-music-cast-rounded.png"));
        }
    }
    else {
        ui->metadata->setText("未在播放");
        ui->album_cover->setPixmap(QPixmap(":/assets/material-symbols-music-cast-rounded.png"));
        if (isLyricsView) lyricsDisplay->setPlainText("暂无歌词");
    }
}

void MainWindow::toggleView() noexcept {
    isLyricsView = !isLyricsView;
    if (isLyricsView) {
        viewStack->setCurrentWidget(lyricsDisplay);
        ui->view_toggle->setToolTip("切换到播放列表视图");
        updateLyricsDisplay();
    }
    else {
        viewStack->setCurrentWidget(ui->music_list);
        ui->view_toggle->setToolTip("切换到歌词视图");
    }
}

void MainWindow::updateLyricsDisplay() noexcept {
    if (currentTrackIndex >= 0 && currentTrackIndex < playlistModel.getTrackCount()) {
        const auto* file = playlistModel.getTrack(currentTrackIndex);
        if(file == nullptr) return;
        QString lyrics = loadLyrics(file->filePath);
        if (lyrics.isEmpty()) lyricsDisplay->setPlainText("暂无歌词");
        else lyricsDisplay->setPlainText(lyrics);
    }
    else lyricsDisplay->setPlainText("暂无歌词");
}

QString MainWindow::loadLyrics(const QString &filePath) const noexcept {
    QFileInfo fileInfo(filePath);
    QString baseName = fileInfo.completeBaseName();
    QString dirPath = fileInfo.absolutePath();
    //qDebug() << "音乐文件路径:" << filePath;
    //qDebug() << "基础名称:" << baseName;
    //qDebug() << "目录路径:" << dirPath;
    QString lrcPath = dirPath + "/" + baseName + ".lrc";
    QFile lrcFile(lrcPath);
    //qDebug() << "尝试加载LRC文件:" << lrcPath;
    //qDebug() << "LRC文件是否存在:" << lrcFile.exists();
    if (lrcFile.exists() && lrcFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&lrcFile);
        QString lyrics = in.readAll();
        lrcFile.close();
        return lyrics;
    }
    QString txtPath = dirPath + "/" + baseName + ".txt";
    QFile txtFile(txtPath);
    //qDebug() << "尝试加载TXT文件:" << txtPath;
    //qDebug() << "TXT文件是否存在:" << txtFile.exists();
    if (txtFile.exists() && txtFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&txtFile);
        QString lyrics = in.readAll();
        txtFile.close();
        return lyrics;
    }
    return QString();
}

void MainWindow::onPlaylistClicked(const QModelIndex &index) noexcept {
    if (index.isValid()) playTrack(index.row());
}

void MainWindow::updateDurationDisplay() noexcept {
    qint64 duration = player.duration();
    ui->total_duration->setText(formatTime(duration));
}

QString MainWindow::formatTime(qint64 milliseconds) const noexcept {
    qint64 seconds = milliseconds / 1000;
    qint64 minutes = seconds / 60;
    seconds = seconds % 60;
    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

void MainWindow::showAbout() noexcept{
    QMessageBox::about(this, "Whatever 播放器",
        "<div style='text-align: center'><h1>做点啥呢？Whatever.</h1><h2>2025 编程实训项目</h2><h2>组员：林峻茗、张峻鸣、易治行</h2></div><div><a href='https://github.com/25programmingpractice/whatever'>https://github.com/25programmingpractice/whatever</a></div>"
    );
}

void MainWindow::updatePlaybackButtons() noexcept {
    QMediaPlayer::PlaybackState state = player.playbackState();
    if (playlistModel.getTrackCount() == 0) {
        ui->play_pause->setEnabled(false);
        ui->play_pause->setIcon(QIcon(":/assets/material-symbols--play-arrow-rounded.png"));
        ui->play_pause->setToolTip("播放");
        actPlay.setText("播放");
        return;
    }
    ui->play_pause->setEnabled(true);
    if (state == QMediaPlayer::PlayingState) {
        ui->play_pause->setIcon(QIcon(":/assets/material-symbols--pause-rounded.png"));
        ui->play_pause->setToolTip("暂停");
        actPlay.setText("暂停");
    }
    else {
        ui->play_pause->setIcon(QIcon(":/assets/material-symbols--play-arrow-rounded.png"));
        ui->play_pause->setToolTip("播放");
        actPlay.setText("播放");
    }
}

void MainWindow::setupTray() noexcept {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) return;

    trayMenu.addAction(&actPrev);
    trayMenu.addAction(&actPlay);
    trayMenu.addAction(&actNext);
    trayIcon.setContextMenu(&trayMenu);

    connect(&actPrev, &QAction::triggered, this, &MainWindow::previousTrack);
    connect(&actPlay, &QAction::triggered, this, &MainWindow::togglePlayback);
    connect(&actNext, &QAction::triggered, this, &MainWindow::nextTrack);

    connect(&trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);

    trayIcon.setIcon(QIcon(":/assets/material-symbols-music-cast-rounded.png"));
    trayIcon.setToolTip("Whatever 播放器");
    trayIcon.show();
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason) noexcept {
    if (reason == QSystemTrayIcon::ActivationReason::Trigger) {
        if (windowState() & Qt::WindowMinimized) {
            setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
            show();
            raise();
            activateWindow();
        }
        else setWindowState(windowState() | Qt::WindowMinimized);
    }
}

void MainWindow::toggleMuted() noexcept {
    if(muted) {
        muted = false;
        ui->mute->setIcon(QIcon(":/assets/material-symbols--volume-up-rounded.png"));
        ui->mute->setToolTip("静音");
        audio.setVolume(volume_ / 100.0f);
        ui->volume->setValue(volume_);
    }
    else {
        muted = true;
        ui->mute->setIcon(QIcon(":/assets/material-symbols--volume-off-rounded.png"));
        ui->mute->setToolTip("取消静音");
        volume_ = audio.volume() * 100;
        audio.setVolume(0.0f);
        ui->volume->setValue(0);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* ev) noexcept {
    if (ev->mimeData()->hasUrls()) ev->acceptProposedAction();
}



void MainWindow::dropEvent(QDropEvent* ev) noexcept {
    const auto urls = ev->mimeData()->urls();
    for (const QUrl &u : urls) {
        if (!u.isLocalFile()) continue;
        const QString path = u.toLocalFile();
        QFileInfo info(path);
        if (info.isDir()) {
            QDirIterator it(path, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                const QString file = it.next();
                playlistModel.addMusicFile(file);
            }
        }
        else playlistModel.addMusicFile(path);
    }
    ev->acceptProposedAction();
}

MainWindow::~MainWindow() {
    delete ui;
}
