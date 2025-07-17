#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QAbstractTableModel>
#include <QStringList>
#include <QFileInfo>
#include <QDir>

struct MusicTrack {
    QString filePath;
    QString title;
    QString artist;
    QString album;
    QString duration;
    
    MusicTrack(const QString& path) : filePath(path) {
        QFileInfo fileInfo(path);
        title = fileInfo.baseName();
        artist = "未知艺术家";
        album = "未知专辑";
        duration = "0:00";
    }
};

class PlaylistModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        Title = 0,
        Artist = 1,
        Album = 2,
        Duration = 3,
        ColumnCount = 4
    };

    explicit PlaylistModel(QObject *parent = nullptr);

    // QAbstractTableModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Playlist management
    void addMusicFile(const QString &filePath);
    void addMusicFolder(const QString &folderPath);
    void clearPlaylist();
    QString getTrackPath(int index) const;
    int getTrackCount() const;
    void removeTrack(int index);

signals:
    void playlistChanged();

private:
    QList<MusicTrack> m_tracks;
    QStringList m_supportedFormats;
};

#endif // PLAYLISTMODEL_H
