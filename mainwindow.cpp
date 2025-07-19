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
    volume_(50),
    viewStack(this),
    lyricsDisplay(this),
    shuffleIndex(0)
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

    playlistModel.loadPlayList();
    updatePlaybackButtons();
}

void MainWindow::setupPlaylist() noexcept {
    /*
     * 初始化和配置主窗口中的播放列表视图（music_list），包括设置数据模型、选择行为、显示样式、列委托、列宽和拉伸方式等，
     * 使播放列表在界面上以合适的方式展示音乐文件信息，并支持删除操作。
     */
    ui->music_list->setModel(&playlistModel);
    ui->music_list->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->music_list->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->music_list->setAlternatingRowColors(true);
    ui->music_list->setItemDelegateForColumn(PlaylistModel::Delete, new CenterIconDelegate(ui->music_list));
    auto* header = ui->music_list->horizontalHeader();
    header->setSectionResizeMode(PlaylistModel::Delete, QHeaderView::Fixed);
    header->setSectionResizeMode(PlaylistModel::Title, QHeaderView::Stretch);
    header->setSectionResizeMode(PlaylistModel::Artist, QHeaderView::Stretch);
    header->setSectionResizeMode(PlaylistModel::Album, QHeaderView::Stretch);
    header->setSectionResizeMode(PlaylistModel::Duration, QHeaderView::Fixed);
    header->resizeSection(PlaylistModel::Delete, 30);
    header->resizeSection(PlaylistModel::Title, 4);
    header->resizeSection(PlaylistModel::Artist, 3);
    header->resizeSection(PlaylistModel::Album, 3);
    header->resizeSection(PlaylistModel::Duration, 80);
    header->setStretchLastSection(false);
}

void MainWindow::setupLyricsView() noexcept {
    /*
     * 初始化和配置主窗口中的歌词视图。它将歌词显示控件 lyricsDisplay 设置为只读、居中对齐，并设置默认文本和样式。
     * 然后将播放列表和歌词视图添加到 viewStack，并将 viewStack 添加到主布局，实现歌词和播放列表视图的切换。最后默认显示播放列表视图。
     */
    lyricsDisplay.setReadOnly(true);
    lyricsDisplay.setAlignment(Qt::AlignCenter);
    lyricsDisplay.setPlainText("暂无歌词");
    lyricsDisplay.setStyleSheet("QTextEdit { font-size: 14px; text-align: center; line-height: 1.5; padding: 20px; }");
    viewStack.addWidget(ui->music_list);
    viewStack.addWidget(&lyricsDisplay);
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(ui->central_widget->layout());
    if (mainLayout) {
        mainLayout->removeWidget(ui->music_list);
        mainLayout->addWidget(&viewStack);
    }
    viewStack.setCurrentWidget(ui->music_list);
}

void MainWindow::setupConnections() noexcept {
    /*
     * 为主窗口中的各个控件（如按钮、菜单项、滑块等）建立信号与槽的连接。这样用户在界面上的操作（如点击、滑动等）就能触发相应的功能处理函数，实现界面与逻辑的联动。
     */
    connect(ui->action_add_file, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->action_add_folder, &QAction::triggered, this, &MainWindow::openFolder);
    connect(ui->action_about, &QAction::triggered, this, &MainWindow::showAbout);
    connect(ui->action_exit, &QAction::triggered, this, &QApplication::quit);

    connect(ui->play_pause, &QPushButton::clicked, this, &MainWindow::togglePlayback);
    connect(ui->previous_music, &QPushButton::clicked, this, &MainWindow::previousTrack);
    connect(ui->next_music, &QPushButton::clicked, this, &MainWindow::nextTrack);
    connect(ui->view_toggle, &QPushButton::clicked, this, &MainWindow::toggleView);

    connect(ui->music_list, &QTableView::clicked, this, &MainWindow::onPlaylistClicked);

    connect(ui->play_mode, &QPushButton::clicked, this, &MainWindow::playModeClicked);

    connect(&player, &QMediaPlayer::durationChanged, this, &MainWindow::playerDurationChanged);
    connect(&player, &QMediaPlayer::positionChanged, this, &MainWindow::playerPositionChanged);
    connect(&player, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::playerMediaStatusChanged);
    connect(&player, &QMediaPlayer::playbackStateChanged, this, &MainWindow::updatePlaybackButtons);

    connect(ui->music_progress, &QSlider::sliderMoved, &player, &QMediaPlayer::setPosition);
    connect(ui->music_progress, &QSlider::sliderPressed, this, &MainWindow::musicProgressPressed);
    connect(ui->music_progress, &QSlider::sliderReleased, this, &MainWindow::musicProgressReleased);
    connect(ui->music_progress, &QSlider::valueChanged, this, &MainWindow::musicProgressValueChanged);

    connect(ui->volume, &QSlider::valueChanged, this, &MainWindow::volumeChanged);

    connect(ui->mute, &QPushButton::clicked, this, &MainWindow::toggleMuted);

    connect(&playlistModel, &QAbstractItemModel::rowsInserted, this, &MainWindow::updatePlaybackButtons);
    connect(&playlistModel, &QAbstractItemModel::rowsRemoved, this, &MainWindow::updatePlaybackButtons);
}

void MainWindow::openFile() noexcept {
    /*
     * 弹出文件选择对话框，让用户选择一个或多个音乐文件（如 mp3、flac 等），然后将选中的文件添加到播放列表模型 playlistModel 中，实现音乐文件的导入功能。
     */
    const QStringList files = QFileDialog::getOpenFileNames(this, "选择音乐文件", QDir::homePath(), "音乐文件 (*.mp3 *.flac *.aac *.wav *.m4a *.ogg *.wma *.mgg)");
    if (!files.isEmpty()) for(int i = 0; i < files.size(); i++) playlistModel.addMusicFile(files[i]);
}

void MainWindow::openFolder() noexcept {
    /*
     * 弹出一个文件夹选择对话框，让用户选择一个包含音乐文件的文件夹，然后将该文件夹下的所有音乐文件批量添加到播放列表模型 playlistModel，实现音乐文件夹的导入功能。
     */
    const QString folderPath = QFileDialog::getExistingDirectory(this, "选择音乐文件夹", QDir::homePath());
    if (!folderPath.isEmpty()) playlistModel.addMusicFolder(folderPath);
}

void MainWindow::togglePlayback() noexcept {
    /*
     * 用于切换音乐播放状态。当播放器正在播放时，调用后会暂停播放；当播放器处于暂停状态时，调用后会恢复播放。
     * 如果当前没有正在播放的曲目，则会根据播放列表情况自动选择并开始播放第一首或当前选中的曲目，实现播放/暂停的切换逻辑。
     */
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
    /*
     * 切换到上一首音乐。它根据当前播放模式（顺序、随机、单曲循环）决定上一首曲目的索引，并调用 playTrack() 播放上一首。
     * 如果播放列表为空则直接返回。顺序模式下，回到上一首或列表末尾；随机模式下，按乱序列表回退；循环模式下，重复当前曲目。
     */
    if (playlistModel.getTrackCount() == 0) return;
    switch(playlistModel.playMode) {
    case PlaylistModel::Ordered:
        if (currentTrackIndex > 0) playTrack(currentTrackIndex - 1);
        else playTrack(playlistModel.getTrackCount() - 1);
        break;
    case PlaylistModel::Shuffled:
        int previous;
        do {
            if(shuffleIndex == 0) shuffleIndex = playlistModel.order.size() - 1;
            else shuffleIndex--;
            previous = playlistModel.order[shuffleIndex];
        } while(previous > playlistModel.getTrackCount() - 1);
        playTrack(previous);
        break;
    case PlaylistModel::Looped:
        playTrack(currentTrackIndex);
        break;
    }
}

void MainWindow::nextTrack() noexcept {
    /*
     * 意图是切换到下一首音乐。它根据当前播放模式（顺序、随机、单曲循环）决定下一首曲目的索引，并调用 playTrack() 播放下一首。
     * 如果播放列表为空则直接返回。顺序模式下，进入下一首或回到列表开头；随机模式下，按乱序列表前进；循环模式下，重复当前曲目。
     */
    if (playlistModel.getTrackCount() == 0) return;
    switch(playlistModel.playMode) {
    case PlaylistModel::Ordered:
        if (currentTrackIndex < playlistModel.getTrackCount() - 1) playTrack(currentTrackIndex + 1);
        else playTrack(0);
        break;
    case PlaylistModel::Shuffled:
        int next;
        do {
            if(shuffleIndex == playlistModel.order.size() - 1) shuffleIndex = 0;
            else shuffleIndex++;
            next = playlistModel.order[shuffleIndex];
        } while(next > playlistModel.getTrackCount() - 1);
        playTrack(next);
        break;
    case PlaylistModel::Looped:
        playTrack(currentTrackIndex);
        break;
    }
}

void MainWindow::playTrack(int index) noexcept {
    /*
    意图是根据传入的索引播放指定的音乐曲目。它会：
    检查索引是否合法；
    获取对应的音乐文件信息；
    设置当前播放曲目索引；
    设置播放器播放该文件；
    更新窗口标题为当前曲目名；
    在系统托盘显示通知（带专辑封面或默认图标）；
    在播放列表中高亮选中当前曲目；
    更新界面上的播放信息（如元数据、封面等）。
    总之，实现了“播放指定索引的音乐并同步界面显示”的功能。
     */
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
            if(smallArt.isNull()) trayIcon.showMessage("正在播放", file->artist + " - " + file->title, QSystemTrayIcon::Information, 1000);
            else trayIcon.showMessage("正在播放", file->artist + " - " + file->title, smallArt, 1000);
        }
        ui->music_list->selectRow(index);
        updatePlayingInfo();
    }
}

void MainWindow::updatePlayingInfo() noexcept {
    /*
     * 根据当前播放的曲目索引，更新主界面上的播放信息，包括显示当前歌曲的元数据（如歌手和标题）、专辑封面、歌词视图内容等。
     * 如果没有正在播放的曲目，则显示默认提示和默认封面。这样可以保证界面信息与实际播放状态同步。
     */
    if (currentTrackIndex >= 0 && currentTrackIndex < playlistModel.getTrackCount()) {
        const auto* file = playlistModel.getTrack(currentTrackIndex);
        ui->metadata->setText(file->artist + " - " + file->title);
        if (isLyricsView) updateLyricsDisplay();
        if (!file->cover.isNull()) ui->album_cover->setPixmap(QPixmap::fromImage(file->cover));
        else {
            bool found = false;
            const QDir dir{QFileInfo(file->filePath).absolutePath()};
            static const QStringList names = { "cover.jpg", "Cover.jpg", "folder.jpg", "Folder.jpg" };
            for (const QString& n : names) {
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
        if (isLyricsView) lyricsDisplay.setPlainText("暂无歌词");
    }
}

void MainWindow::toggleView() noexcept {
    /*
     * 用于在主窗口中切换“播放列表视图”和“歌词视图”。
     * 每次调用会反转 isLyricsView 状态，根据当前状态显示对应的视图控件，并更新切换按钮的提示文本。
     * 如果切换到歌词视图，还会刷新歌词内容显示。
     */
    isLyricsView = !isLyricsView;
    if (isLyricsView) {
        viewStack.setCurrentWidget(&lyricsDisplay);
        ui->view_toggle->setToolTip("切换到播放列表视图");
        updateLyricsDisplay();
    }
    else {
        viewStack.setCurrentWidget(ui->music_list);
        ui->view_toggle->setToolTip("切换到歌词视图");
    }
}

void MainWindow::updateLyricsDisplay() noexcept {
    /*
     * 用于根据当前播放的曲目，更新歌词显示区域的内容。如果有正在播放的曲目且能成功加载歌词文件，则显示歌词；否则显示“暂无歌词”。这样可以保证歌词视图与当前播放状态同步。
     */
    if (currentTrackIndex >= 0 && currentTrackIndex < playlistModel.getTrackCount()) {
        const auto* file = playlistModel.getTrack(currentTrackIndex);
        if(file == nullptr) return;
        QString lyrics = loadLyrics(file->filePath);
        if (lyrics.isEmpty()) lyricsDisplay.setPlainText("暂无歌词");
        else lyricsDisplay.setPlainText(lyrics);
    }
    else lyricsDisplay.setPlainText("暂无歌词");
}

QString MainWindow::loadLyrics(const QString &filePath) const noexcept {
    /*
     * 根据传入的音乐文件路径，尝试在同目录下加载对应的歌词文件（优先.lrc，其次.txt），并返回歌词内容字符串。如果没有找到歌词文件，则返回空字符串。这样可以实现自动匹配和显示当前播放音乐的歌词。
     */
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

void MainWindow::onPlaylistClicked(const QModelIndex& index) noexcept {
    /*
     * 处理播放列表视图的点击事件。当用户点击某一行时，如果点击的是删除按钮，则调用 removeTrack() 删除该曲目；否则调用 playTrack() 播放选中的曲目。
     */
    if (index.isValid()) {
        if(index.column() == PlaylistModel::Delete) playlistModel.removeTrack(index.row());
        else playTrack(index.row());
    }
}

void MainWindow::updateDurationDisplay() noexcept {
    /*
     * 更新播放器的总时长显示。它会获取当前播放器的总时长（以毫秒为单位），并将其格式化为“分钟:秒”形式的字符串，然后设置到界面上的 total_duration 标签中。
     */
    qint64 duration = player.duration();
    ui->total_duration->setText(formatTime(duration));
}

QString MainWindow::formatTime(qint64 milliseconds) const noexcept {
    /*
     * 将毫秒转换为“分钟:秒”格式的字符串。
     */
    qint64 seconds = milliseconds / 1000;
    qint64 minutes = seconds / 60;
    seconds = seconds % 60;
    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

void MainWindow::showAbout() noexcept{
    /*
     * 弹出一个关于对话框，显示应用程序的名称、版本、组员信息和项目链接等。
     */
    QMessageBox::about(this, "Whatever 播放器", "<div style='text-align: center'><h1>做点啥呢？Whatever.</h1><h2>2025 编程实训项目</h2><h2>组员：林峻茗、张峻鸣、易治行</h2></div><div><a href='https://github.com/25programmingpractice/whatever'>https://github.com/25programmingpractice/whatever</a></div>");
}

void MainWindow::updatePlaybackButtons() noexcept {
    /*
     * 更新播放按钮的状态和图标。根据当前播放器的播放状态（播放、暂停、停止等）和播放列表中的曲目数量，设置播放/暂停按钮的图标和提示文本。
     */
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
    /*
     * 设置系统托盘图标和菜单。它会创建一个 QSystemTrayIcon 实例，并添加播放、上一首、下一首等操作的菜单项。
     */
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
    /*
     * 处理系统托盘图标的激活事件。当用户点击托盘图标时，如果当前窗口处于最小化状态，则恢复窗口并激活它；如果窗口已处于活动状态，则最小化窗口。
     */
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
    /*
     * 切换音频静音状态。它会根据当前的静音状态（muted）来设置音量和按钮图标。如果当前是静音状态，则取消静音并恢复之前的音量；如果当前不是静音状态，则将音量设置为0并更新按钮图标。
     */
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
    /*
     * 处理拖放事件，当拖入的内容包含 URL 时，接受该事件以允许放置操作。
     */
    if (ev->mimeData()->hasUrls()) ev->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* ev) noexcept {
    /*
     * 处理拖放事件，将拖入的本地文件或文件夹添加到播放列表模型中。
     */
    const auto urls = ev->mimeData()->urls();
    for (const auto& u : urls) {
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

void MainWindow::playModeClicked() noexcept {
    /*
     * 切换播放模式。它会根据当前的播放模式（顺序、循环、随机）来切换到下一个模式，并更新按钮图标和提示文本。
     */
    switch(playlistModel.playMode) {
    case PlaylistModel::Ordered:
        playlistModel.playMode = PlaylistModel::Looped;
        ui->play_mode->setIcon(QIcon(":/assets/material-symbols--repeat-one-rounded.png"));
        ui->play_mode->setToolTip("单曲循环");
        break;
    case PlaylistModel::Looped: {
        playlistModel.playMode = PlaylistModel::Shuffled;
        ui->play_mode->setIcon(QIcon(":/assets/material-symbols--shuffle-rounded.png"));
        ui->play_mode->setToolTip("随机播放");
        playlistModel.shuffle();
        shuffleIndex = 0;
        break;
    }
    case PlaylistModel::Shuffled:
        playlistModel.playMode = PlaylistModel::Ordered;
        ui->play_mode->setIcon(QIcon(":/assets/material-symbols--playlist-play-rounded.png"));
        ui->play_mode->setToolTip("列表顺序播放");
        break;
    }
}

void MainWindow::playerDurationChanged(qint64 d) noexcept {
    /*
     * 更新播放器的总时长显示。它会将播放器的总时长（以毫秒为单位）设置到音乐进度条的范围，并更新总时长标签的文本。
     */
    ui->music_progress->setRange(0, int(d));
    updateDurationDisplay();
}

void MainWindow::playerPositionChanged(qint64 p) noexcept {
    /*
     * 更新播放器的当前位置显示。它会将播放器的当前位置（以毫秒为单位）设置到音乐进度条的值，并更新当前时长标签的文本。
     */
    ui->music_progress->setValue(int(p));
    ui->current_duration->setText(formatTime(p));
}

void MainWindow::playerMediaStatusChanged(QMediaPlayer::MediaStatus status) noexcept {
    /*
     * 处理播放器的媒体状态变化。当媒体状态变为已加载（Loaded）时，更新播放器的总时长显示；当媒体状态变为结束（EndOfMedia）时，自动切换到下一首曲目。
     */
    if (status == QMediaPlayer::EndOfMedia) nextTrack();
}

void MainWindow::musicProgressPressed() noexcept {
    /*
     * 暂停播放器的 positionChanged 信号，以避免在用户拖动进度条时频繁更新当前时长显示。
     */
    disconnect(&player, &QMediaPlayer::positionChanged, this, nullptr);
}

void MainWindow::musicProgressReleased() noexcept {
    /*
     * 恢复播放器的 positionChanged 信号连接，以便在用户拖动进度条后继续更新当前时长显示。
     */
    player.setPosition(ui->music_progress->value());
    connect(&player, &QMediaPlayer::positionChanged, this, [this](qint64 p){
        ui->music_progress->setValue(int(p));
        ui->current_duration->setText(formatTime(p));
    });
}

void MainWindow::musicProgressValueChanged(int value) noexcept {
    /*
     * 更新音乐进度条的值时，直接设置播放器的当前位置，并更新当前时长标签的文本。
     */
    ui->current_duration->setText(formatTime(value));
}

void MainWindow::volumeChanged(int v) noexcept {
    /*
     * 更新音量滑块的值时，根据当前音量值（0-100）来设置音频输出的音量。如果音量大于0且当前处于静音状态，则取消静音并更新按钮图标和提示文本；如果音量为0且当前未静音，则设置为静音状态并更新按钮图标和提示文本。
     */
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
}

MainWindow::~MainWindow() {
    delete ui;
}
