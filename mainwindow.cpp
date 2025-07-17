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

#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent) noexcept :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    audio(this),
    player(this),
    playlistModel(this),
    currentTrackIndex(-1),
    isLyricsView(false)
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
    connect(ui->view_toggle, &QPushButton::clicked, this, &MainWindow::toggleView);  // 新增：视图切换

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
        audio.setVolume(v / 100.0f);
    });

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
    QString filePath = playlistModel.getTrackPath(index);
    if (!filePath.isEmpty()) {
        currentTrackIndex = index;
        player.setSource(QUrl::fromLocalFile(filePath));
        player.play();
        QFileInfo fileInfo(filePath);
        setWindowTitle(fileInfo.baseName() + " - Whatever");
        setWindowFilePath(filePath);
        ui->music_list->selectRow(index);
        updatePlayingInfo();
    }
}

void MainWindow::updatePlayingInfo() noexcept {
    if (currentTrackIndex >= 0 && currentTrackIndex < playlistModel.getTrackCount()) {
        QString filePath = playlistModel.getTrackPath(currentTrackIndex);
        QFileInfo fileInfo(filePath);
        ui->metadata->setText(QString("%1").arg(fileInfo.baseName()));
        if (isLyricsView) updateLyricsDisplay();
    }
    else {
        ui->metadata->setText("未在播放");
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
        QString filePath = playlistModel.getTrackPath(currentTrackIndex);
        QString lyrics = loadLyrics(filePath);
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
        return;
    }
    ui->play_pause->setEnabled(true);
    if (state == QMediaPlayer::PlayingState) {
        ui->play_pause->setIcon(QIcon(":/assets/material-symbols--pause-rounded.png"));
        ui->play_pause->setToolTip("暂停");
    }
    else {
        ui->play_pause->setIcon(QIcon(":/assets/material-symbols--play-arrow-rounded.png"));
        ui->play_pause->setToolTip("播放");
    }
}

MainWindow::~MainWindow() {
    delete ui;
}
