#ifndef CHAN_H
#define CHAN_H

#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

//TODO: does everything need to be a virtual function?

//TODO each Chan api has its own Captcha object

struct CaptchaLinks{
	QString server= "";
	QString siteKey = "";
	QString lang = "";
	QString challengeURL = "";
	QString imageBaseURL = "";
	QString refererURL = "";
};

struct PostKeys{
	QString no = "no";
	QString resto = "resto";

	//Only on OP (resto == 0) when true
	QString sticky = "sticky";
	QString closed = "closed";
	QString archived = "archived";
	QString archived_on = "0"; //Time when archived

	QString now = "now"; //Date and time (MM\/DD\/YY(Day)HH:MM (:SS on some boards), EST/EDT timezone)
	QString time = "time"; //Unix timestamp

	//might not be present
	QString name = "name";
	QString trip = "trip";
	QString id = "id";
	QString capcode = "capcode";
	QString country = "country";
	QString troll_country = "troll_country";
	QString country_name = "country_name";
	QString sub = "sub"; //Subject
	QString com = "com"; //Comment

	//Image only (tim != "")
	QString tim = "tim"; //Renamed filename (Unix timestamp + milliseconds)
	QString filename = "filename"; //Original filename
	QString ext = "ext"; //File extension (.jpg|.png|.gif|.pdf|.swf|.webm)
	QString fsize = "fsize"; //File size
	QString md5 = "md5"; //File MD5
	QString w = "w"; //Image width
	QString h = "h"; //Image Height
	QString tn_w = "tn_w"; //Thumbnail width
	QString tn_h = "tn_h"; //Thumbnail height
	QString filedeleted = "filedeleted"; //File deleted?
	QString spoiler = "spoiler"; //Spoiler image?
	QString custom_spoiler = "custom_spoiler"; //Custom spoilers? only if board has customs

	//only OPs (resto != 0)
	QString omitted_posts = "omitted_posts"; //# replies ommited
	QString omitted_images = "omitted_images"; //# images ommited
	QString bumplimit = "bumplimit"; //Bump limit met?
	QString imagelimit = "imagelimit"; //Image limit met?
	QString capcode_replies = "capcode_replies"; //only /q/ array of capcode type of post IDs ({"admin":[1234,1267]})
	QString last_modified = "last_modified"; //Time when last modified (UNIX timestamp)
	QString tag = "tag"; //only /f/ Thread tag
	QString semantic_url = "semantic_url"; //Thread URL slug

	QString since4pass = "since4pass"; //Year 4chan Pass bought (YYYY)
};

class Chan
{
public:
	inline Chan(){
		if(requiresCookies()) setCookies();
	}
	virtual inline ~Chan(){}
	virtual QString boardURL(QString &board) = 0;
	virtual QString catalogURL(QString &board) = 0;
	virtual QString threadURL(QString &board, QString &thread) = 0;
	virtual QString postURL(QString &board) = 0;
	virtual QString thumbURL() = 0;
	virtual QString imageURL() = 0;
	virtual QString regURL() = 0;
	virtual QString regToThread() = 0;
	virtual QString regToCatalog() = 0;
	virtual QString apiBase() = 0;
	virtual QJsonArray postsArray(QByteArray &data, QString type = QString()) = 0;
	virtual inline bool usesCaptcha(){return false;}
	virtual inline QString captchaURL(){return QString();}
	virtual inline CaptchaLinks captchaLinks(){CaptchaLinks defaultCaptchaLinks; return defaultCaptchaLinks;}
	virtual inline bool requiresUserAgent(){return false;}
	virtual inline QString requiredUserAgent(){return QString("Mozilla/5.0 (X11; Linux x86_64; rv:59.0) Gecko/20100101 Firefox/59.0");}
	virtual inline bool requiresCookies(){return false;}
	virtual inline void setCookies(){return;}
	virtual inline PostKeys postKeys(){PostKeys defaultPostKeys; return defaultPostKeys;}
};

#endif // CHANS_H
