#ifndef NIGHTBOT_API_H
#define NIGHTBOT_API_H

#include <QObject>
#include <string>
#include <QList>
#include <QString>

// Estrutura para armazenar informações de uma música
struct SongItem {
	QString id;
	QString title;
	QString user;
	int position;
	int duration; // Duração em segundos
};

class NightbotAPI : public QObject {
	Q_OBJECT

public:
	static NightbotAPI &get();

	// Dispara a busca assíncrona pelas informações do usuário
	void FetchUserInfo();

	// Dispara a busca assíncrona pela fila de músicas
	void FetchSongQueue();

	// Funções para controlar o player
	void ControlPlay();
	void ControlPause();
	void ControlSkip();
	void DeleteSong(const QString &songId);
	void PromoteSong(const QString &songId);
	void SetSREnabled(bool enabled);

signals:
	void userInfoFetched(const QString &userName);
	void songQueueFetched(const QList<SongItem> &queue);
	void srStatusFetched(bool isEnabled);

private:
	NightbotAPI();
};

#endif // NIGHTBOT_API_H
