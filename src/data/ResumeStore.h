#pragma once
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QSet>
#include "ThumbnailService.h"
#include "RecentHistoryModel.h"
class MpvController;
class AppSettings;
class HistoryDb;
class ResumeStore final:public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList items READ items NOTIFY itemsChanged)
    Q_PROPERTY(QAbstractItemModel* homeModel READ homeModel CONSTANT)
    Q_PROPERTY(bool homeVisible READ homeVisible NOTIFY homeChanged)
    Q_PROPERTY(int continueIndex READ continueIndex NOTIFY itemsChanged)
    Q_PROPERTY(QVariantList playlist READ playlist NOTIFY playlistChanged)
    Q_PROPERTY(QAbstractItemModel* playlistModel READ playlistModel CONSTANT)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY playlistChanged)
public:
    ResumeStore(MpvController *controller,AppSettings *settings,HistoryDb *db);
    ~ResumeStore() override;
    QVariantList items() const {return m_items;}
    QAbstractItemModel *homeModel(){return &m_recentModel;}
    bool homeVisible() const {return m_home;}
    int continueIndex() const;
    QVariantList playlist() const {return m_playlist;}
    QAbstractItemModel *playlistModel(){return &m_playlistModel;}
    int currentIndex() const;
    Q_INVOKABLE void openMany(const QStringList &paths);
    Q_INVOKABLE void playIndex(int index);
    Q_INVOKABLE void reorder(int from,int to);
    Q_INVOKABLE void open(const QString &path,bool fromStart=false);
    Q_INVOKABLE void remove(const QString &key);
    Q_INVOKABLE void openFolder(const QString &path);
    Q_INVOKABLE void showHome();
    Q_INVOKABLE void save();
    Q_INVOKABLE void navigate(int direction);
    void launch(bool noFile);
signals:
    void itemsChanged();
    void homeChanged();
    void playlistChanged();
private:
    void updateActivity();
    void finish();
    void refreshPlaylist();
    void publishPlaylist(){m_playlistModel.replace(m_playlist);emit playlistChanged();}
    void advance(bool failure=false);
    MpvController *m_controller;
    AppSettings *m_settings;
    HistoryDb *m_db;
    ThumbnailService m_thumbnails;
    QTimer m_saveTimer;
    QVariantMap m_active;
    QVariantList m_items;
    RecentHistoryModel m_recentModel;
    bool m_home=true,m_loaded=false,m_positionReady=false,m_lastPause=true,m_fromStart=false,m_autoLaunch=false,m_launchReady=false,m_haveRecent=false;
    quint64 m_generation=0;
    double m_resumeStart=0;
    bool m_openPending=false;
    bool m_loadExpected=false;
    quint64 m_navigationToken=0;
    QVariantList m_playlist;
    RecentHistoryModel m_playlistModel{true};
    QStringList m_customPaths;
    QString m_playlistFolder;
    quint64 m_playlistToken=0;
    bool m_playlistPending=false,m_waitAdvance=false;
    QSet<QString> m_failedPaths;
};
