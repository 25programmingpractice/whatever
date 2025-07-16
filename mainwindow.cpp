#include <QFileDialog>
#include <QMessageBox>
#include <QUrl>

#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) noexcept :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    audio(QAudioOutput(this)),
    player(QMediaPlayer(this))
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

    connect(ui->action_add_file, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->action_about, &QAction::triggered, this, &MainWindow::showAbout);

    connect(ui->action_exit, &QAction::triggered, this, &QApplication::quit);

    connect(ui->play_pause, &QPushButton::clicked, this, &MainWindow::togglePlayback);

    connect(&player, &QMediaPlayer::durationChanged, ui->music_progress, [this](qint64 d){
        ui->music_progress->setRange(0, int(d));
    });

    connect(&player, &QMediaPlayer::positionChanged, ui->music_progress, [this](qint64 p){
        ui->music_progress->setValue(int(p));
    });

    connect(ui->music_progress, &QSlider::sliderMoved, &player, &QMediaPlayer::setPosition);

    connect(ui->volume, &QSlider::valueChanged, this, [this](int v){
        audio.setVolume(v / 100.0f);
    });
}

void MainWindow::openFile() noexcept {
    const QString f = QFileDialog::getOpenFileName(this, tr("Choose audio"), QDir::homePath(), tr("Audio files (*.mp3 *.flac *.aac *.wav *.m4a *.ogg);;All files (*)"));
    if (!f.isEmpty()) {
        player.setSource(QUrl::fromLocalFile(f));
        player.play();
        ui->play_pause->setText("暂停");
        setWindowTitle(QFileInfo(f).fileName());
        setWindowFilePath(f);
    }
}

void MainWindow::togglePlayback() noexcept {
    if (player.playbackState() == QMediaPlayer::PlayingState) {
        player.pause();
        ui->play_pause->setText("播放");
    }
    else {
        player.play();
        ui->play_pause->setText("暂停");
    }
}

void MainWindow::showAbout() noexcept{
    QMessageBox::about(this, tr("Whatever 播放器"),
        tr("<div style='text-align: center'><h1>做点啥呢？Whatever.</h1><h2>2025 编程实训项目</h2><h2>组员：林峻茗、张峻鸣、易治行</h2></div><div><a href='https://github.com/25programmingpractice/whatever'>https://github.com/25programmingpractice/whatever</a></div>")
    );
}

MainWindow::~MainWindow() {
    delete ui;
}
