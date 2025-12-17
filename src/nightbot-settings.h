#ifndef NIGHTBOT_SETTINGS_H
#define NIGHTBOT_SETTINGS_H

#include <QDialog>

class QLabel;
class QPushButton;
class QCheckBox;
class QSpinBox;

class NightbotSettingsDialog : public QDialog {
	Q_OBJECT

public:
	explicit NightbotSettingsDialog(QWidget *parent = nullptr);

private slots:
	void OnConnectClicked();
	void OnDisconnectClicked();
	void onAuthTimerUpdate(int remainingSeconds);
	void onUserInfoFetched(const QString &userName);
	void onAutoRefreshToggled(bool checked);
	void onRefreshIntervalChanged(int value);

private:
	void UpdateUI(bool just_authenticated = false);

	QLabel *statusLabel;
	QPushButton *connectButton;
	QPushButton *disconnectButton;
	QCheckBox *autoRefreshCheckBox;
	QSpinBox *refreshIntervalSpinBox;
};

#endif // NIGHTBOT_SETTINGS_H
