#include <QDir>
#include <QFileInfo>
#include <QMimeData>

#include "playlistmodel.h"

PlaylistModel::PlaylistModel(QObject* parent) noexcept : QAbstractTableModel(parent) {
    m_supportedFormats << "mp3" << "flac" << "aac" << "wav" << "m4a" << "ogg" << "wma";
}

int PlaylistModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_tracks.size();
}

int PlaylistModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return ColumnCount;
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_tracks.size()) return QVariant();
    const MusicTrack &track = m_tracks.at(index.row());
    if (role == Qt::DisplayRole) switch (index.column()) {
        case Title: return track.title;
        case Artist: return track.artist;
        case Album: return track.album;
        case Duration: return track.duration;
        default: return QVariant();
    }
    return QVariant();
}

QVariant PlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) switch (section) {
        case Title: return tr("标题");
        case Artist: return tr("艺术家");
        case Album: return tr("专辑");
        case Duration: return tr("时长");
        default: return QVariant();
    }
    return QVariant();
}

void PlaylistModel::addMusicFile(const QString &filePath) noexcept {
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists() && m_supportedFormats.contains(fileInfo.suffix().toLower())) {
        beginInsertRows(QModelIndex(), m_tracks.size(), m_tracks.size());
        m_tracks.append(MusicTrack(filePath));
        endInsertRows();
        emit playlistChanged();
    }
}

void PlaylistModel::addMusicFolder(const QString &folderPath) noexcept {
    QDir dir(folderPath);
    if (!dir.exists()) return;
    QStringList filters;
    for (int i = 0; i < m_supportedFormats.size(); i++) filters << QString("*.%1").arg(m_supportedFormats[i]);
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    if (!files.isEmpty()) {
        beginInsertRows(QModelIndex(), m_tracks.size(), m_tracks.size() + files.size() - 1);
        for (int i = 0; i < files.size(); i++) m_tracks.append(MusicTrack(files[i].absoluteFilePath()));
        endInsertRows();
        emit playlistChanged();
    }
}

void PlaylistModel::clearPlaylist() noexcept {
    beginResetModel();
    m_tracks.clear();
    endResetModel();
    emit playlistChanged();
}

QString PlaylistModel::getTrackPath(int index) const noexcept {
    if (index >= 0 && index < m_tracks.size()) return m_tracks.at(index).filePath;
    return QString();
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
