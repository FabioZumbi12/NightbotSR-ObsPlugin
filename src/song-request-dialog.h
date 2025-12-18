#ifndef SONG_REQUEST_DIALOG_H
#define SONG_REQUEST_DIALOG_H

#include <QDialog>

class QLineEdit;
class QPushButton;
class QLabel;

class SongRequestDialog : public QDialog {
	Q_OBJECT

public:
	explicit SongRequestDialog(QWidget *parent = nullptr);

private slots:
	void onSubmitClicked();
	void onSongAdded(bool success, const QString &message);

private:
	QLineEdit *songInput;
	QPushButton *submitButton;
	QLabel *statusLabel;
};

#endif // SONG_REQUEST_DIALOG_H
