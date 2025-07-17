#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QAbstractTableModel>
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QMediaPlayer>
#include <QEventLoop>
#include <QMediaMetaData>

struct MusicTrack {
    QString filePath, title, artist, album;
    qint64 duration;

    MusicTrack(const QString& path) noexcept : filePath(path) {
        QMediaPlayer probe;
        probe.setSource(QUrl::fromLocalFile(path));
        QEventLoop loop;
        QObject::connect(&probe, &QMediaPlayer::metaDataChanged,&loop, &QEventLoop::quit);
        loop.exec();
        duration = probe.duration();
        title = probe.metaData().stringValue(QMediaMetaData::Title);
        if (title.isEmpty()) title = QFileInfo(path).baseName();
        artist = probe.metaData().stringValue(QMediaMetaData::Author);
        if (artist.isEmpty()){
            artist = probe.metaData().stringValue(QMediaMetaData::AlbumArtist);
            if(artist.isEmpty()){
                artist = probe.metaData().stringValue(QMediaMetaData::ContributingArtist);
                if(artist.isEmpty()) artist = "未知艺术家";
            }
        }
        album = probe.metaData().stringValue(QMediaMetaData::AlbumTitle);
        if (album.isEmpty()) album = "未知专辑";
    }
};

class PlaylistModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        Title = 0, Artist, Album, Duration,
        ColumnCount
    };

    explicit PlaylistModel(QObject *parent = nullptr) noexcept;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void addMusicFile(const QString& filePath) noexcept;
    void addMusicFolder(const QString& folderPath) noexcept;
    void clearPlaylist() noexcept;
    QString getTrackPath(int index) const noexcept;
    int getTrackCount() const noexcept;
    void removeTrack(int index) noexcept;

signals:
    void playlistChanged();

private:
    QList<MusicTrack> m_tracks;
    QStringList m_supportedFormats;
};

#endif // PLAYLISTMODEL_H
