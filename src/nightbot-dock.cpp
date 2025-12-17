#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/platform.h>
#include <QDockWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QToolButton>
#include <QStyle>
#include <QTableWidget>
#include <QTimer>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QWidget>

#include "nightbot-dock.h"
#include "nightbot-api.h"
#include "SettingsManager.h"
#include "nightbot-auth.h"

NightbotDock::NightbotDock() : QWidget(nullptr)
{
	QVBoxLayout *mainLayout = new QVBoxLayout();

	QHBoxLayout *controlsLayout = new QHBoxLayout();
	controlsLayout->setContentsMargins(0, 0, 0, 0);

	auto createControlButton = [&](QIcon icon, const QString &tooltip) {
		QPushButton *button = new QPushButton();
		button->setIcon(icon);
		button->setFixedSize(32, 32);
		button->setIconSize(QSize(24, 24));
		button->setToolTip(tooltip);
		return button;
	};

	QPushButton *playButton = createControlButton(style()->standardIcon(QStyle::SP_MediaPlay), obs_module_text("Nightbot.Controls.Play"));
	QPushButton *pauseButton = createControlButton(style()->standardIcon(QStyle::SP_MediaPause), obs_module_text("Nightbot.Controls.Pause"));
	QPushButton *skipButton = createControlButton(style()->standardIcon(QStyle::SP_MediaSkipForward), obs_module_text("Nightbot.Controls.Skip"));

	controlsLayout->addWidget(playButton);
	controlsLayout->addWidget(pauseButton);
	controlsLayout->addWidget(skipButton);

	controlsLayout->addStretch();

	srToggleButton = new QToolButton();
	srToggleButton->setFixedSize(32, 32);
	srToggleButton->setIconSize(QSize(24, 24));
	srToggleButton->setCheckable(true);
	updateSRStatusButton(false);
	srToggleButton->setEnabled(NightbotAuth::get().IsAuthenticated());
	controlsLayout->addWidget(srToggleButton);


	QPushButton *refreshButton = createControlButton(
		style()->standardIcon(QStyle::SP_BrowserReload), obs_module_text("Nightbot.Dock.Refresh"));
	controlsLayout->addWidget(refreshButton);

	mainLayout->addLayout(controlsLayout);

	songQueueTable = new QTableWidget();
	songQueueTable->setColumnCount(4);

	QStringList headers;
	headers << obs_module_text("Nightbot.Queue.Position")
		<< obs_module_text("Nightbot.Queue.Title")
		<< obs_module_text("Nightbot.Queue.User")
		<< obs_module_text("Nightbot.Queue.Actions");
	songQueueTable->setHorizontalHeaderLabels(headers);

	QHeaderView *header = songQueueTable->horizontalHeader();
	header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(1, QHeaderView::Stretch);
	header->setSectionResizeMode(2, QHeaderView::ResizeToContents);

	header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	songQueueTable->verticalHeader()->hide();
	songQueueTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

	mainLayout->addWidget(songQueueTable);

	setLayout(mainLayout);

	connect(refreshButton, &QPushButton::clicked, this, &NightbotDock::onRefreshClicked);

	connect(playButton, &QPushButton::clicked, this, &NightbotDock::onPlayClicked);
	connect(pauseButton, &QPushButton::clicked, this, &NightbotDock::onPauseClicked);
	connect(skipButton, &QPushButton::clicked, this, &NightbotDock::onSkipClicked);

	connect(srToggleButton, &QToolButton::clicked, this, &NightbotDock::onToggleSRClicked);

	connect(&NightbotAPI::get(), &NightbotAPI::songQueueFetched, this,
		&NightbotDock::UpdateSongQueue);

	connect(&NightbotAPI::get(), &NightbotAPI::srStatusFetched, this,
		&NightbotDock::updateSRStatusButton);

	refreshTimer = new QTimer(this);
	connect(refreshTimer, &QTimer::timeout, this, &NightbotDock::onRefreshClicked);
	UpdateRefreshTimer();
};

void NightbotDock::UpdateRefreshTimer()
{
	if (NightbotAuth::get().GetAccessToken().empty()) {
		if (refreshTimer->isActive()) {
			refreshTimer->stop();
			blog(LOG_INFO, "[Nightbot SR/Dock] Not authenticated. Auto-refresh timer stopped.");
		}
		updateSRStatusButton(false);
		return;
	}

	bool enabled = SettingsManager::get().GetAutoRefreshEnabled();
	if (enabled) {
		int interval_s = SettingsManager::get().GetAutoRefreshInterval();
		if (interval_s > 0) {
			int interval_ms = interval_s * 1000;
			refreshTimer->start(interval_ms);
			blog(LOG_INFO, "[Nightbot SR/Dock] Auto-refresh timer started with %dms interval.", interval_ms);
		} else {
			refreshTimer->stop();
			blog(LOG_WARNING, "[Nightbot SR/Dock] Auto-refresh is enabled but interval is invalid (%d seconds). Timer stopped to prevent spam.", interval_s);
		}
	} else {
		refreshTimer->stop();
		blog(LOG_INFO, "[Nightbot SR/Dock] Auto-refresh timer stopped.");
	}
}

void NightbotDock::UpdateSongQueue(const QList<SongItem> &queue)
{
	songQueueTable->clearContents();
	songQueueTable->setRowCount(queue.size());

	for (int i = 0; i < queue.size(); ++i) {
		const SongItem &item = queue.at(i);

		int minutes = item.duration / 60;
		int seconds = item.duration % 60;
		QString durationStr = QStringLiteral("%1:%2").arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'));
		QString titleWithDuration = QStringLiteral("%1 (%2)").arg(item.title, durationStr);

		QTableWidgetItem *posItem;
		QTableWidgetItem *titleItem = new QTableWidgetItem(titleWithDuration);
		QTableWidgetItem *userItem = new QTableWidgetItem(item.user);

		if (i == 0 && !queue.isEmpty()) {
			posItem = new QTableWidgetItem();
			posItem->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

			QFont boldFont;
			boldFont.setBold(true);
			posItem->setFont(boldFont);
			titleItem->setFont(boldFont);
			userItem->setFont(boldFont);
		} else {
			posItem = new QTableWidgetItem(QString::number(item.position));

			QWidget *actionsWidget = new QWidget();
			QHBoxLayout *actionsLayout = new QHBoxLayout(actionsWidget);
			actionsLayout->setContentsMargins(5, 0, 5, 0);
			actionsLayout->setSpacing(5);

			if (item.position > 1) {
				QPushButton *promoteButton = new QPushButton();
				promoteButton->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
				promoteButton->setFixedSize(24, 24);
				promoteButton->setToolTip(obs_module_text("Nightbot.Queue.Promote"));
				connect(promoteButton, &QPushButton::clicked, this, [this, id = item.id]() {
					onPromoteSongClicked(id);
				});
				actionsLayout->addWidget(promoteButton);
			}

			QPushButton *deleteButton = new QPushButton();
			deleteButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
			deleteButton->setFixedSize(24, 24);
			deleteButton->setToolTip(obs_module_text("Nightbot.Queue.Delete"));
			connect(deleteButton, &QPushButton::clicked, this, [this, id = item.id]() {
				onDeleteSongClicked(id);
			});

			actionsLayout->addWidget(deleteButton);
			actionsWidget->setLayout(actionsLayout);

			songQueueTable->setCellWidget(i, 3, actionsWidget);
		}

		posItem->setTextAlignment(Qt::AlignCenter);
		userItem->setTextAlignment(Qt::AlignCenter);
		songQueueTable->setItem(i, 0, posItem);
		songQueueTable->setItem(i, 1, titleItem);
		songQueueTable->setItem(i, 2, userItem);

	}
}

void NightbotDock::onRefreshClicked()
{
	NightbotAPI::get().FetchSongQueue();
}

void NightbotDock::onPlayClicked()
{
	NightbotAPI::get().ControlPlay();	
	NightbotAPI::get().FetchSongQueue();
}

void NightbotDock::onPauseClicked()
{
	NightbotAPI::get().ControlPause();	
	NightbotAPI::get().FetchSongQueue();
}

void NightbotDock::onSkipClicked()
{
	NightbotAPI::get().ControlSkip();
	QTimer::singleShot(500, this, &NightbotDock::onRefreshClicked);
	
	NightbotAPI::get().FetchSongQueue();
}

void NightbotDock::onToggleSRClicked()
{
	bool isChecked = srToggleButton->isChecked();
	NightbotAPI::get().SetSREnabled(isChecked);
	updateSRStatusButton(isChecked);
	QTimer::singleShot(1000, this, &NightbotDock::onRefreshClicked);
}

void NightbotDock::updateSRStatusButton(bool isEnabled)
{
	srToggleButton->setEnabled(NightbotAuth::get().IsAuthenticated());
	srToggleButton->setChecked(isEnabled);
	if (isEnabled) {
		srToggleButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
		srToggleButton->setToolTip(obs_module_text("Nightbot.Dock.SR_Enabled"));
	} else {
		srToggleButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
		srToggleButton->setToolTip(obs_module_text("Nightbot.Dock.SR_Disabled"));
	}
}

void NightbotDock::onDeleteSongClicked(const QString &songId)
{
	NightbotAPI::get().DeleteSong(songId);
	QTimer::singleShot(500, this, &NightbotDock::onRefreshClicked);

	NightbotAPI::get().FetchSongQueue();
}

void NightbotDock::onPromoteSongClicked(const QString &songId)
{
	NightbotAPI::get().PromoteSong(songId);
	QTimer::singleShot(500, this, &NightbotDock::onRefreshClicked);

	NightbotAPI::get().FetchSongQueue();
}
