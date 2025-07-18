#include <QDir>
#include <QFileInfo>
#include <QMimeData>
#include <QMessageBox>
#include <QSaveFile>
#include <QStandardPaths>
#include <QDebug>
#include <QPushButton>

#include "playlistmodel.h"

PlaylistModel::PlaylistModel(QWidget* parent) noexcept : QAbstractTableModel(parent), parent(parent) {
    m_supportedFormats << "mp3" << "flac" << "aac" << "wav" << "m4a" << "ogg" << "wma" << "mgg";
    connect(this, &PlaylistModel::playlistChanged, this, &PlaylistModel::savePlayList);
    qDebug() << "播放列表保存于：" << defaultPath();
}

int PlaylistModel::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent)
    return m_tracks.size();
}

int PlaylistModel::columnCount(const QModelIndex& parent) const {
    Q_UNUSED(parent)
    return ColumnCount;
}

QVariant PlaylistModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_tracks.size()) return QVariant();
    const MusicTrack& track = m_tracks.at(index.row());
    if (role == Qt::DisplayRole) switch (index.column()) {
        case Delete: return {};
        case Title: return track.title;
        case Artist: return track.artist;
        case Album: return track.album;
        case Duration: return formatDuration(track.duration);
        default: return QVariant();
    }
    else if (role == Qt::DecorationRole && index.column() == Delete) return QIcon(":/assets/material-symbols--delete-forever-rounded.png");
    else if (role == Qt::ToolTipRole && index.column() == Delete) return "从库中移除音乐";
    return QVariant();
}

QVariant PlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole) switch (section) {
        case Delete: return "";
        case Title: return "标题";
        case Artist: return "艺术家";
        case Album: return "专辑";
        case Duration: return "时长";
        default: return QVariant();
    }
    return QVariant();
}

void PlaylistModel::addMusicFile(const QString& filePath) noexcept {
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists() && m_supportedFormats.contains(fileInfo.suffix().toLower())) {
        for(int i = 0; i < m_tracks.size(); i++) if(m_tracks[i].filePath == filePath) {
            QMessageBox::warning(parent, "文件已存在", "文件已经存在于播放列表中！");
            return;
        }
        beginInsertRows(QModelIndex(), m_tracks.size(), m_tracks.size());
        m_tracks.append(MusicTrack(filePath));
        endInsertRows();
        emit playlistChanged();
    }
}

void PlaylistModel::addMusicFolder(const QString& folderPath) noexcept {
    QDir dir(folderPath);
    if (!dir.exists()) return;
    QStringList filters;
    for (int i = 0; i < m_supportedFormats.size(); i++) filters << QString("*.%1").arg(m_supportedFormats[i]);
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    if (!files.isEmpty()) for(int i = 0; i < files.size(); i++) {
        bool exist = false;
        for(int j = 0; j < m_tracks.size(); j++) if(m_tracks[j].filePath == files[i].filePath()) {
            exist = true;
            break;
        }
        if(!exist) {
            beginInsertRows(QModelIndex(), m_tracks.size(), m_tracks.size());
            m_tracks.append(MusicTrack(files[i].absoluteFilePath()));
            endInsertRows();
            emit playlistChanged();
        }
    }
}

void PlaylistModel::clearPlaylist() noexcept {
    beginResetModel();
    m_tracks.clear();
    endResetModel();
    emit playlistChanged();
}

const MusicTrack* PlaylistModel::getTrack(int index) const noexcept {
    if (index >= 0 && index < m_tracks.size()) return &m_tracks.at(index);
    return nullptr;
}

int PlaylistModel::getTrackCount() const noexcept {
    return m_tracks.size();
}

void PlaylistModel::removeTrack(int index) noexcept {
    if (index >= 0 && index < m_tracks.size()) {
        beginRemoveRows(QModelIndex(), index, index);
        m_tracks.removeAt(index);
        endRemoveRows();
        emit playlistChanged();
    }
}

QString PlaylistModel::formatDuration(qint64 milliseconds) const noexcept {
    if (milliseconds <= 0) return "未知时长";
    qint64 seconds = milliseconds / 1000;
    qint64 minutes = seconds / 60;
    seconds = seconds % 60;
    return tr("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

QString PlaylistModel::defaultPath() noexcept {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir{}.mkpath(dir);
    return dir + "/playlist.txt";
}

bool PlaylistModel::savePlayList() noexcept {
    QSaveFile file{defaultPath()};
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out{&file};
    for (int i = 0; i < m_tracks.size(); i++) out << m_tracks[i].filePath << "\n";
    return file.commit();
}

bool PlaylistModel::loadPlayList() noexcept {
    QFile file{defaultPath()};
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    beginResetModel();
    m_tracks.clear();
    QTextStream in{&file};
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.isEmpty()) continue;
        m_tracks.append(MusicTrack(line));
    }
    endResetModel();
    emit playlistChanged();
    return true;
}
