#ifndef NIGHTBOT_DOCK_H
#define NIGHTBOT_DOCK_H

#include <QList>
#include <QWidget>

class QToolButton;
class QTableWidget;
class QTimer;
struct SongItem;

class NightbotDock : public QWidget {
	Q_OBJECT

public:
	explicit NightbotDock();
	void UpdateRefreshTimer();

private slots:
	void UpdateSongQueue(const QList<SongItem> &queue);
	void onRefreshClicked();
	void onPlayClicked();
	void onPauseClicked();
	void onSkipClicked();
	void onDeleteSongClicked(const QString &songId);
	void onPromoteSongClicked(const QString &songId);
	void onToggleSRClicked();
	void updateSRStatusButton(bool isEnabled);

private:
	QTableWidget *songQueueTable;
	QTimer *refreshTimer;
	QToolButton *srToggleButton;
};

#endif // NIGHTBOT_DOCK_H
