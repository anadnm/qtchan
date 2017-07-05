#include <QJsonArray>
#include "boardtabhelper.h"
#include "netcontroller.h"
#include <QSettings>
#include <QtConcurrent/QtConcurrent>

BoardTabHelper::BoardTabHelper() {

}

void BoardTabHelper::startUp(QString &board, BoardType type, QString search, QWidget *parent)
{
	this->parent = parent;
	this->board = board;
	this->type = type;
	this->search = search;
	//this->threadUrl = "https://a.4cdn.org/"+board+"/thread/"+thread+".json";
	if(type == BoardType::Index) boardUrl = "https://a.4cdn.org/"+board+"/1.json";
	else boardUrl = "https://a.4cdn.org/"+board+"/catalog.json";
	QSettings settings;
	this->expandAll = settings.value("autoExpand",false).toBool();
	QDir().mkpath(board+"/index/thumbs");
	qDebug() << boardUrl;
	request = QNetworkRequest(QUrl(boardUrl));
	request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
	getPosts();
	/*updateTimer = new QTimer();
	updateTimer->setInterval(60000);
	updateTimer->start();
	if(settings.value("autoUpdate").toBool()) {
		connectionUpdate = connect(updateTimer, &QTimer::timeout,
								   this,&BoardTabHelper::getPosts,UniqueDirect);
	}*/
}

BoardTabHelper::~BoardTabHelper() {
	disconnect(parent);
	disconnect(this);
	abort = true;
	/*updateTimer->stop();
	disconnect(connectionUpdate);
	if(updateTimer) delete updateTimer;*/
	if(gettingReply) {
		reply->abort();
		disconnect(reply);
		reply->deleteLater();
	}
	qDeleteAll(tfMap);
}

void BoardTabHelper::setAutoUpdate(bool update) {
	(void)update;
	/*if(update) {
		connectionUpdate = connect(updateTimer, &QTimer::timeout,
								   this,&BoardTabHelper::getPosts,UniqueDirect);
	}*/
}

void BoardTabHelper::getPosts() {
	disconnect(connectionPost);
	if(reply) reply->abort();
	qDebug() << "getting posts for" << boardUrl;
	reply = nc.jsonManager->get(request);
	gettingReply = true;
	connectionPost = connect(reply, &QNetworkReply::finished,
							 this,&BoardTabHelper::loadPosts, UniqueDirect);
}

void BoardTabHelper::writeJson(QString &board, QByteArray &rep) {
	QFile jsonFile(board+"/index.json");
	jsonFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
	QDataStream out(&jsonFile);
	out << rep;
	jsonFile.close();
}

void BoardTabHelper::loadPosts() {
	if(!reply) return;
	gettingReply = false;
	if(reply->error()) {
		qDebug().noquote() << "loading post error:" << reply->errorString();
		if(reply->error() == QNetworkReply::ContentNotFoundError) {
			//qDebug() << "Stopping timer for" << threadUrl;
			//updateTimer->stop();
			emit boardStatus("404");
		}
		reply->deleteLater();
		return;
	}
	qDeleteAll(tfMap);
	tfMap.clear();
	//write to file and make json array
	QByteArray rep = reply->readAll();
	reply->deleteLater();
	if(rep.isEmpty()) return;
	QJsonArray threads = filterThreads(rep);
	int length = threads.size();
	qDebug().noquote() << "length of" << boardUrl << "is" << QString::number(length);
	QtConcurrent::run(&BoardTabHelper::writeJson,board, rep);
	//load new posts
	QSettings settings;
	bool loadFile = settings.value("autoExpand",false).toBool() || this->expandAll;
	bool showIndexReplies = settings.value("showIndexReplies",false).toBool();
	QStringList idFilters = settings.value("filters/"+board+"/id").toStringList();
	QJsonArray t;
	QJsonObject p;
	int i = 0;
	while(!abort && i < length){
		if(type==BoardType::Index){
			t = threads.at(i).toObject().value("posts").toArray();
			p = t.at(0).toObject();
		}
		else{
			p = threads.at(i).toObject();
		}
		QString threadNum = QString("%1").arg(p.value("no").toDouble(),0,'f',0);
		if (idFilters.contains(threadNum)){
			qDebug("threadNum %s filtered!",threadNum.toLatin1().constData());
			i++;
			continue;
		}
		ThreadForm *tf = new ThreadForm(board,threadNum,Thread,true,loadFile,parent);
		tf->load(p);
		tfMap.insert(tf->post.no,tf);
		emit newThread(tf);
		if(type==BoardType::Index && showIndexReplies){
			for(int j=1;j<t.size();j++){
				p = t.at(j).toObject();
				ThreadForm *tfChild = new ThreadForm(board,threadNum,Thread,false,loadFile,parent);
				tfChild->load(p);
				//tfMap.insert(tfChild->post.no,tfChild);
				emit newTF(tfChild,tf);
			}
		}
		i++;
		QCoreApplication::processEvents();
	}
	if(!abort) emit addStretch();
}

QJsonArray BoardTabHelper::filterThreads(QByteArray &rep){
	QJsonArray threads;
	if(type==BoardType::Index) threads = QJsonDocument::fromJson(rep).object().value("threads").toArray();
	else{ //TODO put this is static concurrent function with future or stream?
		qDebug("searching %s",search.toLatin1().constData());
		QRegularExpression re(search,QRegularExpression::CaseInsensitiveOption);
		QRegularExpressionMatch match;
		QJsonArray allThreads = QJsonDocument::fromJson(rep).array();
		QJsonArray pageThreads;
		for(int i=0;i<allThreads.size();i++) {
			pageThreads = allThreads.at(i).toObject().value("threads").toArray();
			for(int j=0;j<pageThreads.size();j++) {
				match = re.match(pageThreads.at(j).toObject().value("sub").toString() % pageThreads.at(j).toObject().value("com").toString(),0,QRegularExpression::PartialPreferFirstMatch);
				if(match.hasMatch())threads.append(pageThreads.at(j));
			}
		}
	}
	return threads;
}

void BoardTabHelper::loadAllImages() {
	expandAll = !expandAll;
	qDebug() << "settings expandAll for" << boardUrl << "to" << expandAll;
	if(expandAll) {
		QMapIterator<QString,ThreadForm*> mapI(tfMap);
		while (mapI.hasNext()) {
			mapI.next();
			mapI.value()->loadOrig();
		}
	}
}