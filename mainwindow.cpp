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

MainWindow::MainWindow(QWidget *parent) noexcept :
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
    setupLyricsView();  // 新增：设置歌词视图
    setupConnections();
    
    // 初始化播放按钮图标
    updatePlaybackButtons();
}

void MainWindow::setupPlaylist() {
    ui->music_list->setModel(&playlistModel);
    ui->music_list->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->music_list->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->music_list->setAlternatingRowColors(true);

    ui->music_list->setColumnWidth(0, 300);
    ui->music_list->setColumnWidth(1, 150);
    ui->music_list->setColumnWidth(2, 150);
    ui->music_list->setColumnWidth(3, 80);
}

void MainWindow::setupLyricsView() {
    // 创建堆叠窗口来管理播放列表和歌词视图
    viewStack = new QStackedWidget(this);
    
    // 创建歌词显示控件
    lyricsDisplay = new QTextEdit(this);
    lyricsDisplay->setReadOnly(true);
    lyricsDisplay->setAlignment(Qt::AlignCenter);
    lyricsDisplay->setPlainText("暂无歌词");
    lyricsDisplay->setStyleSheet("QTextEdit { font-size: 14px; line-height: 1.5; padding: 20px; }");
    
    // 将播放列表和歌词显示添加到堆叠窗口
    viewStack->addWidget(ui->music_list);
    viewStack->addWidget(lyricsDisplay);
    
    // 替换原来的music_list位置
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(ui->central_widget->layout());
    if (mainLayout) {
        mainLayout->removeWidget(ui->music_list);
        mainLayout->addWidget(viewStack);
    }
    
    // 默认显示播放列表
    viewStack->setCurrentWidget(ui->music_list);
}

void MainWindow::setupConnections() {
    connect(ui->action_add_file, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->action_add_folder, &QAction::triggered, this, &MainWindow::openFolder);
    connect(ui->action_about, &QAction::triggered, this, &MainWindow::showAbout);
    connect(ui->action_exit, &QAction::triggered, this, &QApplication::quit);

    connect(ui->play_pause, &QPushButton::clicked, this, &MainWindow::togglePlayback);
    connect(ui->previous_music, &QPushButton::clicked, this, &MainWindow::previousTrack);
    connect(ui->next_music, &QPushButton::clicked, this, &MainWindow::nextTrack);
    connect(ui->view_toggle, &QPushButton::clicked, this, &MainWindow::toggleView);  // 新增：视图切换

    connect(ui->music_list, &QTableView::doubleClicked, this, &MainWindow::onPlaylistDoubleClicked);

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

    // 监听播放状态变化，自动更新按钮图标
    connect(&player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state){
        updatePlaybackButtons();
    });

    connect(ui->music_progress, &QSlider::sliderMoved, &player, &QMediaPlayer::setPosition);
    connect(ui->music_progress, &QSlider::sliderPressed, this, [this](){
        // 用户开始拖拽或点击进度条时，暂停位置更新
        disconnect(&player, &QMediaPlayer::positionChanged, this, nullptr);
    });
    connect(ui->music_progress, &QSlider::sliderReleased, this, [this](){
        // 用户释放进度条时，设置新位置并恢复位置更新
        player.setPosition(ui->music_progress->value());
        connect(&player, &QMediaPlayer::positionChanged, this, [this](qint64 p){
            ui->music_progress->setValue(int(p));
            ui->current_duration->setText(formatTime(p));
        });
    });
    connect(ui->music_progress, &QSlider::valueChanged, this, [this](int value){
        // 实时更新时间显示
        ui->current_duration->setText(formatTime(value));
    });

    connect(ui->volume, &QSlider::valueChanged, this, [this](int v){
        audio.setVolume(v / 100.0f);
    });

    // 监听播放列表模型变化，更新播放按钮状态
    connect(&playlistModel, &QAbstractItemModel::rowsInserted, this, [this](){
        updatePlaybackButtons();
    });
    connect(&playlistModel, &QAbstractItemModel::rowsRemoved, this, [this](){
        updatePlaybackButtons();
    });
}

void MainWindow::openFile() noexcept {
    const QStringList files = QFileDialog::getOpenFileNames(this, tr("选择音乐文件"), QDir::homePath(), tr("音乐文件 (*.mp3 *.flac *.aac *.wav *.m4a *.ogg *.wma *.mgg)"));
    if (!files.isEmpty()) for(int i = 0; i < files.size(); i++) playlistModel.addMusicFile(files[i]);
}

void MainWindow::openFolder() noexcept {
    const QString folderPath = QFileDialog::getExistingDirectory(this, tr("选择音乐文件夹"), QDir::homePath());
    if (!folderPath.isEmpty()) playlistModel.addMusicFolder(folderPath);
}

void MainWindow::togglePlayback() noexcept {
    QMediaPlayer::PlaybackState state = player.playbackState();
    
    if (state == QMediaPlayer::PlayingState) {
        // 当前正在播放，暂停播放
        player.pause();
    }
    else if (state == QMediaPlayer::PausedState) {
        // 当前已暂停，继续播放
        player.play();
    }
    else {
        // 当前停止状态，开始播放
        if (currentTrackIndex == -1 && playlistModel.getTrackCount() > 0) {
            playTrack(0);
        } else if (currentTrackIndex >= 0 && currentTrackIndex < playlistModel.getTrackCount()) {
            playTrack(currentTrackIndex);
        } else if (playlistModel.getTrackCount() > 0) {
            playTrack(0);
        }
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
        
        // 更新播放信息显示
        updatePlayingInfo();
    }
}

void MainWindow::updatePlayingInfo() {
    if (currentTrackIndex >= 0 && currentTrackIndex < playlistModel.getTrackCount()) {
        QString filePath = playlistModel.getTrackPath(currentTrackIndex);
        QFileInfo fileInfo(filePath);
        
        // 这里可以添加更多的元数据显示
        ui->metadata->setText(QString("正在播放: %1").arg(fileInfo.baseName()));
        
        // 如果当前是歌词视图，更新歌词显示
        if (isLyricsView) {
            updateLyricsDisplay();
        }
    } else {
        ui->metadata->setText("未在播放");
        if (isLyricsView) {
            lyricsDisplay->setPlainText("暂无歌词");
        }
    }
}

void MainWindow::toggleView() noexcept {
    isLyricsView = !isLyricsView;
    
    if (isLyricsView) {
        // 切换到歌词视图
        viewStack->setCurrentWidget(lyricsDisplay);
        ui->view_toggle->setToolTip(tr("切换到播放列表"));
        updateLyricsDisplay();
    } else {
        // 切换到播放列表
        viewStack->setCurrentWidget(ui->music_list);
        ui->view_toggle->setToolTip(tr("切换到歌词视图"));
    }
}

void MainWindow::updateLyricsDisplay() {
    if (currentTrackIndex >= 0 && currentTrackIndex < playlistModel.getTrackCount()) {
        QString filePath = playlistModel.getTrackPath(currentTrackIndex);
        QString lyrics = loadLyrics(filePath);
        
        if (lyrics.isEmpty()) {
            lyricsDisplay->setPlainText("暂无歌词");
        } else {
            lyricsDisplay->setPlainText(lyrics);
        }
    } else {
        lyricsDisplay->setPlainText("暂无歌词");
    }
}

QString MainWindow::loadLyrics(const QString &filePath) const {
    QFileInfo fileInfo(filePath);
    QString baseName = fileInfo.completeBaseName();
    QString dirPath = fileInfo.absolutePath();
    
    // 调试信息：打印路径
    qDebug() << "音乐文件路径:" << filePath;
    qDebug() << "基础名称:" << baseName;
    qDebug() << "目录路径:" << dirPath;
    
    // 尝试加载同名的.lrc文件
    QString lrcPath = dirPath + "/" + baseName + ".lrc";
    QFile lrcFile(lrcPath);
    
    qDebug() << "尝试加载LRC文件:" << lrcPath;
    qDebug() << "LRC文件是否存在:" << lrcFile.exists();
    
    if (lrcFile.exists() && lrcFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&lrcFile);
        QString lyrics = in.readAll();
        lrcFile.close();
        return lyrics;
    }
    
    // 如果没有找到.lrc文件，尝试.txt文件
    QString txtPath = dirPath + "/" + baseName + ".txt";
    QFile txtFile(txtPath);
    
    qDebug() << "尝试加载TXT文件:" << txtPath;
    qDebug() << "TXT文件是否存在:" << txtFile.exists();
    
    if (txtFile.exists() && txtFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&txtFile);
        QString lyrics = in.readAll();
        txtFile.close();
        return lyrics;
    }
    
    return QString();  // 没有找到歌词文件
}

void MainWindow::onPlaylistDoubleClicked(const QModelIndex &index) {
    if (index.isValid()) playTrack(index.row());
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

void MainWindow::updatePlaybackButtons() {
    QMediaPlayer::PlaybackState state = player.playbackState();
    
    // 如果播放列表为空，禁用播放按钮
    if (playlistModel.getTrackCount() == 0) {
        ui->play_pause->setEnabled(false);
        ui->play_pause->setIcon(QIcon(":/assets/material-symbols--play-arrow-rounded.png"));
        ui->play_pause->setToolTip(tr("播放"));
        return;
    }
    
    // 播放列表有内容时启用播放按钮
    ui->play_pause->setEnabled(true);
    
    if (state == QMediaPlayer::PlayingState) {
        // 正在播放，显示暂停图标
        ui->play_pause->setIcon(QIcon(":/assets/material-symbols--pause-rounded.png"));
        ui->play_pause->setToolTip(tr("暂停"));
    } else {
        // 暂停或停止状态，显示播放图标
        ui->play_pause->setIcon(QIcon(":/assets/material-symbols--play-arrow-rounded.png"));
        ui->play_pause->setToolTip(tr("播放"));
    }
}

MainWindow::~MainWindow() {
    delete ui;
}
