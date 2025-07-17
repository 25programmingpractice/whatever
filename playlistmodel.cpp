#include "playlistmodel.h"
#include <QDir>
#include <QFileInfo>
#include <QMimeData>

PlaylistModel::PlaylistModel(QObject *parent)
    : QAbstractTableModel(parent)
{
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
    if (!index.isValid() || index.row() >= m_tracks.size())
        return QVariant();

    const MusicTrack &track = m_tracks.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Title:
            return track.title;
        case Artist:
            return track.artist;
        case Album:
            return track.album;
        case Duration:
            return track.duration;
        default:
            return QVariant();
        }
    }

    return QVariant();
}

QVariant PlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case Title:
            return tr("标题");
        case Artist:
            return tr("艺术家");
        case Album:
            return tr("专辑");
        case Duration:
            return tr("时长");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

void PlaylistModel::addMusicFile(const QString &filePath) {
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists() && m_supportedFormats.contains(fileInfo.suffix().toLower())) {
        beginInsertRows(QModelIndex(), m_tracks.size(), m_tracks.size());
        m_tracks.append(MusicTrack(filePath));
        endInsertRows();
        emit playlistChanged();
    }
}

void PlaylistModel::addMusicFolder(const QString &folderPath) {
    QDir dir(folderPath);
    if (!dir.exists()) return;

    QStringList filters;
    for (const QString &format : m_supportedFormats) {
        filters << QString("*.%1").arg(format);
    }

    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    
    if (!files.isEmpty()) {
        beginInsertRows(QModelIndex(), m_tracks.size(), m_tracks.size() + files.size() - 1);
        for (const QFileInfo &fileInfo : files) {
            m_tracks.append(MusicTrack(fileInfo.absoluteFilePath()));
        }
        endInsertRows();
        emit playlistChanged();
    }
}

void PlaylistModel::clearPlaylist() {
    beginResetModel();
    m_tracks.clear();
    endResetModel();
    emit playlistChanged();
}

QString PlaylistModel::getTrackPath(int index) const {
    if (index >= 0 && index < m_tracks.size()) {
        return m_tracks.at(index).filePath;
    }
    return QString();
}

int PlaylistModel::getTrackCount() const {
    return m_tracks.size();
}

void PlaylistModel::removeTrack(int index) {
    if (index >= 0 && index < m_tracks.size()) {
        beginRemoveRows(QModelIndex(), index, index);
        m_tracks.removeAt(index);
        endRemoveRows();
        emit playlistChanged();
    }
}
