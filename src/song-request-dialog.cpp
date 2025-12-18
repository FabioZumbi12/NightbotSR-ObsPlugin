#include "song-request-dialog.h"
#include "nightbot-api.h"
#include "plugin-support.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

SongRequestDialog::SongRequestDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(get_obs_text("Nightbot.SongRequest.Title"));
	setMinimumWidth(300);

	QVBoxLayout *layout = new QVBoxLayout(this);

	songInput = new QLineEdit();
	songInput->setPlaceholderText(
		get_obs_text("Nightbot.SongRequest.Placeholder"));
	layout->addWidget(songInput);

	submitButton =
		new QPushButton(get_obs_text("Nightbot.SongRequest.Submit"));
	layout->addWidget(submitButton);

	statusLabel = new QLabel();
	statusLabel->setWordWrap(true);
	layout->addWidget(statusLabel);

	setLayout(layout);

	connect(submitButton, &QPushButton::clicked, this,
		&SongRequestDialog::onSubmitClicked);
	connect(&NightbotAPI::get(), &NightbotAPI::songAdded, this,
		&SongRequestDialog::onSongAdded);
}

void SongRequestDialog::onSubmitClicked()
{
	QString query = songInput->text().trimmed();
	if (!query.isEmpty()) {
		statusLabel->setText(
			get_obs_text("Nightbot.SongRequest.Submitting"));
		submitButton->setEnabled(false);
		NightbotAPI::get().AddSong(query);
	}
}

void SongRequestDialog::onSongAdded(bool success, const QString &message)
{
	submitButton->setEnabled(true);
	if (success) {
		accept(); // Fecha a janela em caso de sucesso
	} else {
		statusLabel->setText(QString("<font color='red'>%1</font>").arg(message));
	}
}
