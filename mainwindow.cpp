#include <memory>

#include <QFileDialog>

#include "mainwindow.h"
#include "./ui_mainwindow.h"

using std::unique_ptr, std::make_unique;

MainWindow::MainWindow(QWidget *parent) noexcept :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    audio(make_unique<QAudioOutput>(this)),
    player(make_unique<QMediaPlayer>(this))
{
    ui->setupUi(this);
    player->setAudioOutput(audio.get());
    audio->setVolume(0.5f);

    connect(ui->action_add_file, &QAction::triggered,
            this, &MainWindow::openFile);

    connect(ui->play_pause, &QPushButton::clicked,
            this, &MainWindow::togglePlayback);

    connect(player.get(), &QMediaPlayer::durationChanged,
            ui->music_progress, [this](qint64 d){
            ui->music_progress->setRange(0, int(d));
        });

    connect(player.get(), &QMediaPlayer::positionChanged,
            ui->music_progress, [this](qint64 p){
                ui->music_progress->setValue(int(p));
            });
    connect(ui->music_progress, &QSlider::sliderMoved,
            player.get(), &QMediaPlayer::setPosition);

    connect(ui->volume, &QSlider::valueChanged,
            this, [this](int v){ audio->setVolume(v / 100.0f); });
}

void MainWindow::openFile() noexcept {
    const QString f = QFileDialog::getOpenFileName(this, tr("Choose audio"), QDir::homePath(), tr("Audio files (*.mp3 *.flac *.aac *.wav);;All files (*)"));
    if (!f.isEmpty()) {
        player->setSource(QUrl::fromLocalFile(f));
        player->play();
        ui->play_pause->setText("⏸");
    }
}

void MainWindow::togglePlayback() noexcept {
    if (player->playbackState() == QMediaPlayer::PlayingState) {
        player->pause();
        ui->play_pause->setText("▶");
    }
    else {
        player->play();
        ui->play_pause->setText("⏸");
    }
}

MainWindow::~MainWindow() {
    delete ui;
}
