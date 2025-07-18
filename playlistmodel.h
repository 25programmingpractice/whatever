#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <vector>

#include <QAbstractTableModel>
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QMediaPlayer>
#include <QEventLoop>
#include <QMediaMetaData>
#include <QImage>
#include <QStyledItemDelegate>

struct MusicTrack {
    QString filePath, title, artist, album;
    qint64 duration;
    QImage cover;

    MusicTrack(const QString& path) noexcept : filePath(path) {
        QMediaPlayer probe;
        probe.setSource(QUrl::fromLocalFile(path));
        QEventLoop loop;
        QObject::connect(&probe, &QMediaPlayer::metaDataChanged,&loop, &QEventLoop::quit);
        loop.exec();
        duration = probe.duration();
        cover = probe.metaData().value(QMediaMetaData::CoverArtImage).value<QImage>();
        if (cover.isNull()) cover = probe.metaData().value(QMediaMetaData::ThumbnailImage).value<QImage>();
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
        Delete = 0, Title, Artist, Album, Duration,
        ColumnCount
    };

    enum PlayMode : uint8_t {
        Ordered, Looped, Shuffled
    } playMode{PlayMode::Ordered};

    explicit PlaylistModel(QWidget* parent = nullptr) noexcept;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void addMusicFile(const QString& filePath) noexcept;
    void addMusicFolder(const QString& folderPath) noexcept;
    void clearPlaylist() noexcept;
    const MusicTrack* getTrack(int index) const noexcept;
    int getTrackCount() const noexcept;
    void removeTrack(int index) noexcept;
    void shuffle() noexcept;

    std::vector<int> order;
    bool shuffleStarted{false};

public slots:
    bool savePlayList() noexcept;
    bool loadPlayList() noexcept;

signals:
    void playlistChanged();

private:
    QString defaultPath() noexcept;
    QString formatDuration(qint64 milliseconds) const noexcept;

    QWidget* parent;
    QList<MusicTrack> m_tracks;
    QStringList m_supportedFormats;
};


class CenterIconDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void initStyleOption(QStyleOptionViewItem* opt, const QModelIndex& idx) const override {
        QStyledItemDelegate::initStyleOption(opt, idx);
        if (idx.column() == PlaylistModel::Delete) {
            opt->decorationAlignment = Qt::AlignCenter;
            opt->decorationPosition = QStyleOptionViewItem::Left;
            opt->decorationSize = opt->rect.size() / 1.25f;
        }
    }
};

#endif // PLAYLISTMODEL_H
