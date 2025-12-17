#include "nightbot-api.h"
#include "nightbot-auth.h"

#include <obs-module.h>
#include <curl/curl.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QtConcurrent/QtConcurrent>
#include <QThreadPool>

static size_t auth_curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	((std::string *)userp)->append((char *)contents, realsize);
	return realsize;
}

// Função auxiliar para realizar uma requisição GET genérica
static std::pair<long, std::string>
PerformGetRequest(const std::string &url_str)
{
	const std::string &access_token = NightbotAuth::get().GetAccessToken();
	if (access_token.empty()) {
		blog(LOG_WARNING,
		     "[Nightbot SR/API] Tentativa de fazer requisição GET sem um token de acesso.");
		return {-1, ""};
	}

	CURL *curl = curl_easy_init();
	if (!curl) {
		blog(LOG_ERROR, "[Nightbot SR/API] Failed to initialize libcurl.");
		return {-1, ""};
	}

	std::string readBuffer;
	std::string auth_header = "Authorization: Bearer " + access_token;
	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, auth_header.c_str());

	curl_easy_setopt(curl, CURLOPT_URL, url_str.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, auth_curl_write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 0L);

	CURLcode res = curl_easy_perform(curl);
	long http_code = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	if (res != CURLE_OK) {
		blog(LOG_ERROR,
		     "[Nightbot SR/API] GET request to '%s' failed: %s",
		     url_str.c_str(), curl_easy_strerror(res));
		http_code = -1; // Indicate a curl error
	}

	return {http_code, readBuffer};
}

// Função auxiliar para realizar uma requisição POST genérica
static std::pair<long, std::string>
PerformPostRequest(const std::string &url_str, const std::string &post_body)
{
	const std::string &access_token = NightbotAuth::get().GetAccessToken();
	if (access_token.empty()) {
		blog(LOG_WARNING,
		     "[Nightbot SR/API] Tentativa de fazer requisição POST sem um token de acesso.");
		return {-1, ""};
	}

	CURL *curl = curl_easy_init();
	if (!curl) {
		blog(LOG_ERROR, "[Nightbot SR/API] Failed to initialize libcurl.");
		return {-1, ""};
	}

	std::string readBuffer;
	std::string auth_header = "Authorization: Bearer " + access_token;
	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded; charset=utf-8");
	headers = curl_slist_append(headers, auth_header.c_str());

	curl_easy_setopt(curl, CURLOPT_URL, url_str.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_body.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, auth_curl_write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 0L);

	CURLcode res = curl_easy_perform(curl);
	long http_code = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

	if (res != CURLE_OK) {
		blog(LOG_ERROR,
		     "[Nightbot SR/API] POST request to '%s' failed: %s",
		     url_str.c_str(), curl_easy_strerror(res));
		http_code = -1; // Indica um erro do cURL
	}

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	return {http_code, readBuffer};
}

// Função auxiliar para realizar uma requisição PUT genérica com corpo JSON
static std::pair<long, std::string>
PerformPutRequest(const std::string &url_str, const std::string &put_body)
{
	const std::string &access_token = NightbotAuth::get().GetAccessToken();
	if (access_token.empty()) {
		blog(LOG_WARNING,
		     "[Nightbot SR/API] Tentativa de fazer requisição PUT sem um token de acesso.");
		return {-1, ""};
	}

	CURL *curl = curl_easy_init();
	if (!curl) {
		blog(LOG_ERROR, "[Nightbot SR/API] Failed to initialize libcurl.");
		return {-1, ""};
	}

	std::string readBuffer;
	std::string auth_header = "Authorization: Bearer " + access_token;
	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	headers = curl_slist_append(headers, auth_header.c_str());

	curl_easy_setopt(curl, CURLOPT_URL, url_str.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, put_body.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, auth_curl_write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 0L);

	CURLcode res = curl_easy_perform(curl);
	long http_code = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

	if (res != CURLE_OK) {
		blog(LOG_ERROR, "[Nightbot SR/API] PUT request to '%s' failed: %s",
		     url_str.c_str(), curl_easy_strerror(res));
		http_code = -1; // Indica um erro do cURL
	}

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	return {http_code, readBuffer};
}

// Função auxiliar para realizar requisições com métodos customizados (PUT, DELETE)
static std::pair<long, std::string>
PerformRequestWithMethod(const std::string &url_str, const std::string &method)
{
	const std::string &access_token = NightbotAuth::get().GetAccessToken();
	if (access_token.empty()) {
		blog(LOG_WARNING,
		     "[Nightbot SR/API] Tentativa de fazer requisição %s sem um token de acesso.",
		     method.c_str());
		return {-1, ""};
	}

	CURL *curl = curl_easy_init();
	if (!curl) {
		blog(LOG_ERROR, "[Nightbot SR/API] Failed to initialize libcurl.");
		return {-1, ""};
	}

	std::string readBuffer;
	std::string auth_header = "Authorization: Bearer " + access_token;
	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, auth_header.c_str());

	curl_easy_setopt(curl, CURLOPT_URL, url_str.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str()); // Define o método (POST/DELETE)
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, auth_curl_write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 0L);

	CURLcode res = curl_easy_perform(curl);
	long http_code = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

	if (res != CURLE_OK) {
		blog(LOG_ERROR, "[Nightbot SR/API] %s request to '%s' failed: %s",
		     method.c_str(), url_str.c_str(), curl_easy_strerror(res));
		http_code = -1; // Indica um erro do cURL
	}

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	return {http_code, readBuffer};
}

NightbotAPI &NightbotAPI::get()
{
	static NightbotAPI instance;
	return instance;
}

NightbotAPI::NightbotAPI() {}

void NightbotAPI::FetchUserInfo()
{
	blog(LOG_INFO, "[Nightbot SR/API] Fetching user info...");

	const std::string url = "https://api.nightbot.tv/1/me";
	auto [http_code, response_body] = PerformGetRequest(url);

	if (http_code == 200) {
		QThreadPool::globalInstance()->start([this, response_body]() {
			QJsonParseError parseError;
			QByteArray response_data =
				QString::fromUtf8(response_body.c_str()).toUtf8();
			QJsonDocument doc = QJsonDocument::fromJson(response_data, &parseError);

			if (doc.isNull()) {
				blog(LOG_ERROR,
					 "[Nightbot SR/API] Failed to parse user info response: %s",
					 parseError.errorString().toUtf8().constData());
				emit userInfoFetched("");
			} else if (doc.isObject()) {
				QJsonObject userObj = doc.object()["user"].toObject();
				QString display_name = userObj["displayName"].toString();

				blog(LOG_INFO, "[Nightbot SR/API] Fetched user: %s",
					 display_name.toUtf8().constData());
				emit userInfoFetched(display_name);
			} else {
				emit userInfoFetched("");
			}
		});
	} else {
		blog(LOG_WARNING,
		     "[Nightbot SR/API] User info fetch failed with HTTP status %ld.",
		     http_code);
		emit userInfoFetched("");
	}
}

void NightbotAPI::FetchSongQueue()
{
	QThreadPool::globalInstance()->start([this]() {
		QList<SongItem> song_queue;

		const std::string url = "https://api.nightbot.tv/1/song_requests/queue";
		auto [http_code, response_body] = PerformGetRequest(url);

		if (http_code == 200) {
			QJsonParseError parseError;
			QJsonDocument doc = QJsonDocument::fromJson(
				response_body.c_str(), &parseError);

			if (doc.isNull() || !doc.isObject()) {
				blog(LOG_WARNING, "[Nightbot SR/API] Failed to parse song queue response.");
			} else {
				QJsonObject rootObj = doc.object();

				// Otimização: Pega o status do SR que já vem na resposta da fila
				if (rootObj.contains("_requestsEnabled")) {
					bool isEnabled = rootObj["_requestsEnabled"].toBool();
					emit srStatusFetched(isEnabled);
				}

				// 1. Pega a música que está tocando agora (_currentSong)
				if (rootObj.contains("_currentSong") && rootObj["_currentSong"].isObject()) {
					QJsonObject songObj = rootObj["_currentSong"].toObject();
					SongItem item;
					item.id = songObj["_id"].toString();
					item.position = 0; // Música atual é sempre a posição 0
					QJsonObject trackObj = songObj["track"].toObject();
					item.title = trackObj["title"].toString();
					item.duration = trackObj["duration"].toInt();

					// A música pode ser da playlist e não ter um 'user'
					if (songObj.contains("user") && songObj["user"].isObject()) {
						item.user = songObj["user"].toObject()["displayName"].toString();
					} else {
						item.user = obs_module_text("Nightbot.Queue.PlaylistUser");
					}
					song_queue.append(item);
				}

				// 2. Pega o resto da fila (queue)
				QJsonArray queueArray = rootObj["queue"].toArray();
				for (const QJsonValue &value : queueArray) {
					QJsonObject songObj = value.toObject();
					SongItem item;
					item.id = songObj["_id"].toString();
					item.position = songObj["_position"].toInt();
					QJsonObject trackObj = songObj["track"].toObject();
					item.title = trackObj["title"].toString();
					item.duration = trackObj["duration"].toInt();
					item.user = songObj["user"].toObject()["displayName"].toString();
					song_queue.append(item);
				}
			}

			// Ordena a lista de músicas pela posição para garantir a ordem correta
			std::sort(song_queue.begin(), song_queue.end(),
				  [](const SongItem &a, const SongItem &b) {
					  return a.position < b.position;
				  });
		} else {
			blog(LOG_WARNING,
			     "[Nightbot SR/API] User song queue fetch failed with HTTP status %ld.",
			     http_code);
		}

		emit songQueueFetched(song_queue);
	});
}

void NightbotAPI::ControlPlay()
{
	QThreadPool::globalInstance()->start([this]() {
		blog(LOG_INFO, "[Nightbot SR/API] Sending PLAY command...");
		const std::string url =
			"https://api.nightbot.tv/1/song_requests/queue/play";
		auto [http_code, response_body] = PerformPostRequest(url, "");

		if (http_code >= 200 && http_code < 300) {
			blog(LOG_INFO, "[Nightbot SR/API] PLAY command successful.");
		} else {
			blog(LOG_WARNING,
			     "[Nightbot SR/API] PLAY command failed with HTTP status %ld.",
			     http_code);
		}
	});
}

void NightbotAPI::ControlPause()
{
	QThreadPool::globalInstance()->start([this]() {
		blog(LOG_INFO, "[Nightbot SR/API] Sending PAUSE command...");
		const std::string url =
			"https://api.nightbot.tv/1/song_requests/queue/pause";
		auto [http_code, response_body] = PerformPostRequest(url, "");

		if (http_code >= 200 && http_code < 300) {
			blog(LOG_INFO, "[Nightbot SR/API] PAUSE command successful.");
		} else {
			blog(LOG_WARNING,
			     "[Nightbot SR/API] PAUSE command failed with HTTP status %ld.",
			     http_code);
		}
	});
}

void NightbotAPI::ControlSkip()
{
	QThreadPool::globalInstance()->start([this]() {
		blog(LOG_INFO, "[Nightbot SR/API] Sending SKIP command...");
		const std::string url = "https://api.nightbot.tv/1/song_requests/queue/skip";
		PerformPostRequest(url, "");
	});
}

void NightbotAPI::DeleteSong(const QString &songId)
{
	if (songId.isEmpty())
		return;

	QThreadPool::globalInstance()->start([songId]() {
		blog(LOG_INFO, "[Nightbot SR/API] Deleting song with ID: %s", songId.toUtf8().constData());
		std::string url = "https://api.nightbot.tv/1/song_requests/queue/" + songId.toStdString();
		PerformRequestWithMethod(url, "DELETE");
	});
}

void NightbotAPI::SetSREnabled(bool enabled)
{
	QThreadPool::globalInstance()->start([this, enabled]() {
		blog(LOG_INFO, "[Nightbot SR/API] Setting Song Requests to %s...",
		     enabled ? "Enabled" : "Disabled");
		const std::string url = "https://api.nightbot.tv/1/song_requests";

		QJsonObject body;
		body["enabled"] = enabled;
		QJsonDocument doc(body);
		std::string put_body = doc.toJson(QJsonDocument::Compact).toStdString();

		PerformPutRequest(url, put_body);
	});
}

void NightbotAPI::PromoteSong(const QString &songId)
{
	if (songId.isEmpty())
		return;

	QThreadPool::globalInstance()->start([songId]() {
		blog(LOG_INFO, "[Nightbot SR/API] Promoting song with ID: %s", songId.toUtf8().constData());
		std::string url = "https://api.nightbot.tv/1/song_requests/queue/" + songId.toStdString() + "/promote";
		PerformPostRequest(url, "");
	});
}
