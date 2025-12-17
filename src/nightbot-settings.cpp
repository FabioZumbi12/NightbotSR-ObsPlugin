#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>

#include "nightbot-settings.h"
#include "nightbot-api.h"
#include "nightbot-auth.h"
#include "SettingsManager.h"
#include "nightbot-dock.h"

extern NightbotDock *g_dock_widget; // Acessa a instância global da dock

// Use the singleton accessor
#define auth NightbotAuth::get()

NightbotSettingsDialog::NightbotSettingsDialog(QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle(obs_module_text("Nightbot.Settings"));

	QVBoxLayout *mainLayout = new QVBoxLayout();
	setLayout(mainLayout);

	// Instruções
	QLabel *instructions = new QLabel(
		obs_module_text("Nightbot.Settings.Instructions"));
	instructions->setWordWrap(true);

	// Status da Conexão
	statusLabel = new QLabel();

	// Botões
	connectButton =
		new QPushButton(obs_module_text("Nightbot.Settings.Connect"));
	disconnectButton = new QPushButton(
		obs_module_text("Nightbot.Settings.Disconnect"));

	QHBoxLayout *buttonLayout = new QHBoxLayout();
	buttonLayout->addWidget(connectButton);
	buttonLayout->addWidget(disconnectButton);

	// Opções de atualização automática
	autoRefreshCheckBox = new QCheckBox(obs_module_text("Nightbot.Settings.AutoRefresh.Enable"));
	refreshIntervalSpinBox = new QSpinBox();
	refreshIntervalSpinBox->setMinimum(5); // Mínimo de 5 segundos
	refreshIntervalSpinBox->setMaximum(300); // Máximo de 5 minutos
	refreshIntervalSpinBox->setSuffix("s");

	QHBoxLayout *refreshLayout = new QHBoxLayout();
	refreshLayout->addWidget(autoRefreshCheckBox);
	refreshLayout->addWidget(refreshIntervalSpinBox);
	refreshLayout->addStretch();

	mainLayout->addWidget(instructions);
	mainLayout->addSpacing(15);
	mainLayout->addWidget(statusLabel);
	mainLayout->addLayout(buttonLayout);
	mainLayout->addSpacing(15);
	mainLayout->addLayout(refreshLayout);

	// Conecta os sinais das novas opções
	connect(autoRefreshCheckBox, &QCheckBox::toggled, this, &NightbotSettingsDialog::onAutoRefreshToggled);
	connect(refreshIntervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &NightbotSettingsDialog::onRefreshIntervalChanged);

	// Conecta os sinais dos botões aos slots
	connect(connectButton, &QPushButton::clicked, this,
		&NightbotSettingsDialog::OnConnectClicked);
	connect(disconnectButton, &QPushButton::clicked, this,
		&NightbotSettingsDialog::OnDisconnectClicked);

	// Connect to the auth finished signal to update the UI automatically
	connect(&auth, &NightbotAuth::authenticationFinished, this, [this](bool success){
		UpdateUI(success); });
	connect(&auth, &NightbotAuth::authTimerUpdate, this,
		&NightbotSettingsDialog::onAuthTimerUpdate);

	UpdateUI();

	connect(&NightbotAPI::get(), &NightbotAPI::userInfoFetched, this,
		&NightbotSettingsDialog::onUserInfoFetched);
}

void NightbotSettingsDialog::OnConnectClicked()
{
	auth.Authenticate();
}

void NightbotSettingsDialog::OnDisconnectClicked()
{
	auth.ClearTokens();
	UpdateUI(false);
}

void NightbotSettingsDialog::onAuthTimerUpdate(int remainingSeconds)
{
	statusLabel->setText(
		QString("<b>%1</b> <span style='color: #ffcc00;'>%2 (%3s)</span>")
			.arg(obs_module_text("Nightbot.Settings.Status"))
			.arg(obs_module_text(
				"Nightbot.Settings.Status.Authenticating"))
			.arg(remainingSeconds));
}

void NightbotSettingsDialog::onUserInfoFetched(const QString &userName)
{
	if (!userName.isEmpty()) {
		SettingsManager::get().SetUserName(userName.toStdString());

		// Atualiza a fila de músicas na dock
		if (g_dock_widget) {
			NightbotAPI::get().FetchSongQueue();
		}

		// Atualiza a UI com o nome do usuário
		UpdateUI();
	}
}

void NightbotSettingsDialog::onAutoRefreshToggled(bool checked)
{
	SettingsManager::get().SetAutoRefreshEnabled(checked);
	refreshIntervalSpinBox->setEnabled(checked);
	if (g_dock_widget)
		g_dock_widget->UpdateRefreshTimer();
}

void NightbotSettingsDialog::onRefreshIntervalChanged(int value)
{
	SettingsManager::get().SetAutoRefreshInterval(value);
	if (g_dock_widget)
		g_dock_widget->UpdateRefreshTimer();
}

void NightbotSettingsDialog::UpdateUI(bool just_authenticated)
{
	bool authenticated = auth.IsAuthenticated();

	if (authenticated) {
		std::string user_name = SettingsManager::get().GetNightUserName();
		blog(LOG_INFO, "[Nightbot SR/Settings] Authenticated user: %s",
				user_name.c_str());

		// Se acabamos de autenticar ou não temos o nome salvo, buscamos na API
		if (just_authenticated || user_name.empty()) {
			NightbotAPI::get().FetchUserInfo();
		}

		if (!user_name.empty()) {
			QString connected_as_format = obs_module_text(
				"Nightbot.Settings.Status.ConnectedAs");
			QString connected_as_text =
				connected_as_format.arg(user_name.c_str());
			statusLabel->setText(QString("<b>%1</b> %2")
						 .arg(obs_module_text("Nightbot.Settings.Status"))
						 .arg(connected_as_text));
		}

	} else {
		statusLabel->setText(QString("<b>%1</b> %2")
					 .arg(obs_module_text(
						 "Nightbot.Settings.Status"))
					 .arg(obs_module_text(
						 "Nightbot.Settings.Status.Disconnected")));
	}

	connectButton->setEnabled(!authenticated);
	disconnectButton->setEnabled(authenticated);

	// Atualiza os controles de auto-refresh
	bool autoRefreshEnabled = SettingsManager::get().GetAutoRefreshEnabled();
	autoRefreshCheckBox->setChecked(autoRefreshEnabled);
	refreshIntervalSpinBox->setEnabled(autoRefreshEnabled);
	refreshIntervalSpinBox->setValue(SettingsManager::get().GetAutoRefreshInterval());
}
