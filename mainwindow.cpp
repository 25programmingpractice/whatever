#include <QFileDialog>
#include <QMessageBox>
#include <QUrl>
#include <QDir>
#include <QStandardPaths>

#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) noexcept :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    audio(QAudioOutput(this)),
    player(QMediaPlayer(this)),
    playlistModel(new PlaylistModel(this)),
    currentTrackIndex(-1)
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
    setupConnections();
    
    // 自动加载音乐文件夹
    playlistModel->addMusicFolder("/Users/yhx/Documents/ZjmTempMusic");
}

void MainWindow::setupPlaylist() {
    ui->music_list->setModel(playlistModel);
    ui->music_list->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->music_list->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->music_list->setAlternatingRowColors(true);
    
    // 设置列宽
    ui->music_list->setColumnWidth(0, 300); // 标题
    ui->music_list->setColumnWidth(1, 150); // 艺术家
    ui->music_list->setColumnWidth(2, 150); // 专辑
    ui->music_list->setColumnWidth(3, 80);  // 时长
}

void MainWindow::setupConnections() {
    // 菜单连接
    connect(ui->action_add_file, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->action_add_folder, &QAction::triggered, this, &MainWindow::openFolder);
    connect(ui->action_about, &QAction::triggered, this, &MainWindow::showAbout);
    connect(ui->action_exit, &QAction::triggered, this, &QApplication::quit);

    // 播放控制连接
    connect(ui->play_pause, &QPushButton::clicked, this, &MainWindow::togglePlayback);
    connect(ui->previous_music, &QPushButton::clicked, this, &MainWindow::previousTrack);
    connect(ui->next_music, &QPushButton::clicked, this, &MainWindow::nextTrack);

    // 播放列表连接
    connect(ui->music_list, &QTableView::doubleClicked, this, &MainWindow::onPlaylistDoubleClicked);

    // 播放器连接
    connect(&player, &QMediaPlayer::durationChanged, this, [this](qint64 d){
        ui->music_progress->setRange(0, int(d));
        updateDurationDisplay();
    });

    connect(&player, &QMediaPlayer::positionChanged, this, [this](qint64 p){
        ui->music_progress->setValue(int(p));
        ui->current_duration->setText(formatTime(p));
    });

    connect(&player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status){
        if (status == QMediaPlayer::EndOfMedia) {
            nextTrack();
        }
    });

    connect(ui->music_progress, &QSlider::sliderMoved, &player, &QMediaPlayer::setPosition);

    connect(ui->volume, &QSlider::valueChanged, this, [this](int v){
        audio.setVolume(v / 100.0f);
    });
}

void MainWindow::openFile() noexcept {
    const QString f = QFileDialog::getOpenFileName(this, tr("Choose audio"), QDir::homePath(), tr("Audio files (*.mp3 *.flac *.aac *.wav *.m4a *.ogg);;All files (*)"));
    if (!f.isEmpty()) {
        playlistModel->addMusicFile(f);
    }
}

void MainWindow::openFolder() noexcept {
    const QString folderPath = QFileDialog::getExistingDirectory(this, tr("选择音乐文件夹"), QDir::homePath());
    if (!folderPath.isEmpty()) {
        playlistModel->addMusicFolder(folderPath);
    }
}

void MainWindow::togglePlayback() noexcept {
    if (player.playbackState() == QMediaPlayer::PlayingState) {
        player.pause();
        ui->play_pause->setText("播放");
    }
    else {
        if (currentTrackIndex == -1 && playlistModel->getTrackCount() > 0) {
            playTrack(0);
        } else {
            player.play();
            ui->play_pause->setText("暂停");
        }
    }
}

void MainWindow::previousTrack() noexcept {
    if (playlistModel->getTrackCount() == 0) return;
    
    if (currentTrackIndex > 0) {
        playTrack(currentTrackIndex - 1);
    } else {
        // 循环到最后一首
        playTrack(playlistModel->getTrackCount() - 1);
    }
}

void MainWindow::nextTrack() noexcept {
    if (playlistModel->getTrackCount() == 0) return;
    
    if (currentTrackIndex < playlistModel->getTrackCount() - 1) {
        playTrack(currentTrackIndex + 1);
    } else {
        // 循环到第一首
        playTrack(0);
    }
}

void MainWindow::playTrack(int index) noexcept {
    if (index < 0 || index >= playlistModel->getTrackCount()) return;
    
    QString filePath = playlistModel->getTrackPath(index);
    if (!filePath.isEmpty()) {
        currentTrackIndex = index;
        player.setSource(QUrl::fromLocalFile(filePath));
        player.play();
        ui->play_pause->setText("暂停");
        
        QFileInfo fileInfo(filePath);
        setWindowTitle(fileInfo.baseName() + " - Whatever");
        setWindowFilePath(filePath);
        
        // 高亮当前播放的行
        ui->music_list->selectRow(index);
    }
}

void MainWindow::onPlaylistDoubleClicked(const QModelIndex &index) {
    if (index.isValid()) {
        playTrack(index.row());
    }
}

void MainWindow::updateDurationDisplay() {
    qint64 duration = player.duration();
    ui->total_duration->setText(formatTime(duration));
}

QString MainWindow::formatTime(qint64 milliseconds) const {
    qint64 seconds = milliseconds / 1000;
    qint64 minutes = seconds / 60;
    seconds = seconds % 60;
    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

void MainWindow::showAbout() noexcept{
    QMessageBox::about(this, tr("Whatever 播放器"),
        tr("<div style='text-align: center'><h1>做点啥呢？Whatever.</h1><h2>2025 编程实训项目</h2><h2>组员：林峻茗、张峻鸣、易治行</h2></div><div><a href='https://github.com/25programmingpractice/whatever'>https://github.com/25programmingpractice/whatever</a></div>")
    );
}

MainWindow::~MainWindow() {
    delete ui;
}
