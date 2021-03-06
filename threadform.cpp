#include "threadform.h"
#include "ui_threadform.h"
#include "threadformcontext.h"
#include "filter.h"
#include "threadtab.h"
#include "mainwindow.h"
#include <QScrollArea>
#include <QPixmap>
#include <QImageReader>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDesktopServices>
#include <QSettings>
#include <QStringList>
#include <QListIterator>
#include <QPainter>
#include <QMenu>


//TODO get rid of #include threadtab.h and mainwindow.h by using signals/slots
//TODO Possibly decouple the file and thumb getters to another class class
ThreadForm::ThreadForm(Chan *api, ThreadFormStrings strings, bool root, bool autoExpand, QWidget *parent, int replyLevel) :
	QWidget(parent), api(api), strings(strings), root(root), autoExpand(autoExpand), replyLevel(replyLevel), tab(parent),
	ui(new Ui::ThreadForm)
{
	if(root) rootTF = this;
	ui->setupUi(this);
	ui->postLayout->setAlignment(static_cast<Qt::Alignment>(Qt::AlignTop | Qt::AlignLeft));
	this->board = strings.board;
	if(strings.thread == "index") this->type = PostType::Thread;
	else this->type = PostType::Reply;
	QSettings settings;
	setFontSize(settings.value("fontSize",14).toInt());
	setBackground();
	ui->quoteWidget->hide();
//	ui->tim->hide();
	ui->fileInfo->hide();
	connect(ui->hide,&ClickableLabel::clicked,this,&ThreadForm::hideClicked);
	comQuoteConnection = connect(ui->com,&QLabel::linkActivated,this,&ThreadForm::quoteClicked);
	infoQuoteConnection = connect(ui->info,&QLabel::linkActivated,this,&ThreadForm::quoteClicked);
	ui->info->installEventFilter(this);
	ui->com->installEventFilter(this);
	ui->fileInfo->installEventFilter(this);
	this->installEventFilter(this);
}

void ThreadForm::setBackground(){
	QSettings settings;
	if(replyLevel){
		QString bColor;
		if(!settings.value("style/ThreadForm/background-color","").toString().isEmpty())
			bColor = settings.value("style/ThreadForm/background-color","").toString();
		else if(!settings.value("style/MainWindow/background-color","").toString().isEmpty())
			bColor = settings.value("style/MainWindow/background-color","").toString();
		if(!bColor.isEmpty())background.setNamedColor(bColor);
		//darkness = darkness*qPow(0.8,replyLevel-1);
		background.setRgb(background.red()*qPow(0.8,replyLevel-1),
						  background.green()*qPow(0.8,replyLevel-1),
						  background.blue()*qPow(0.8,replyLevel-1));
		QString hidebColor = "#hide{background-color:" % background.name() % "}";
		background.setRgb(background.red()*0.8,
						  background.green()*0.8,
						  background.blue()*0.8);
		this->setStyleSheet(hidebColor + "*{background-color:" + background.name() +"}");
	}
}

void ThreadForm::appendQuote()
{
	static_cast<ThreadTab*>(tab)->quoteIt(">>"+post.no);
}

ThreadForm::~ThreadForm()
{
	inserted.clear();
	foreach(QNetworkReply *reply, networkReplies){
		reply->abort();
		reply->deleteLater();
	}
	//disconnect clones quote clicking in notifications
	foreach(QPointer<ThreadForm> tf, clones){
		if(tf){
			tf->disconnect(tf->comQuoteConnection);
			tf->disconnect(tf->infoQuoteConnection);
		}
	}
	delete ui;
}

void ThreadForm::setText(QString text)
{
	ui->com->setText(text);
}

QString ThreadForm::infoString(){
	tfInfoString = "<span style=\"color: rgb(152, 125, 62); font-weight: bold;\">" % post.sub % "</span> " +
			"<span style=\"color: rgb(163, 68, 67);\">" % post.name % "</span> " %
			strings.countryString % regionString %
			"<span>" % post.realNow % "</span> " %
			"<a href=\"#op" % post.no % "\" style=\"color:#bbbbbb; text-decoration: none\">No." % post.no % "</a> " %
			"<a href=\"#f" % post.no % "\" style=\"color:#897399; text-decoration: none\"></a> " %
			repliesString;
	return tfInfoString;
}

QString ThreadForm::matchThis(){
	return post.sub % post.com % tfInfoString % fileInfoString;
}

void ThreadForm::load(Post &post)
{
	this->post = post;
	ui->info->setText(infoString());
	ui->com->setText(post.com);

	getFlag();
	getThumb();
	//get file call in getThumb
	//if(autoExpand) getFiles();
}

void ThreadForm::getFlag(){
	if(post.country_name.isEmpty()) return;
	setInfoString();
	downloadFile(strings.flagUrl,strings.flagPath,nc.thumbManager);
}

void ThreadForm::getFile(ClickableLabel* label, int ind, bool andOpen){
	PostFile postFile = post.files.at(ind);
	gettingFile.replace(ind,true);
	QString url = api->apiBase() + postFile.fileUrlPath;
	QString message = andOpen ? "clicked" : "";
	downloadFile(url,strings.pathBase % postFile.filePath,nc.fileManager,"file",message,label);
}

void ThreadForm::getFiles(){
	int i = 0;
	foreach(PostFile postFile,post.files){
		if(gettingFile.at(i) || finished.at(i)){i++; continue;}
		ClickableLabel *label = labels.at(i);
		gettingFile.replace(i,true);
		QString url = api->apiBase() + postFile.fileUrlPath;
		downloadFile(url,strings.pathBase % postFile.filePath,nc.fileManager,"file","",label);
		i++;
	}
}

void ThreadForm::getThumb(){
	int numFiles = post.files.size();
	if(!numFiles) return;
	QLayoutItem *item = ui->contentLayout->takeAt(0);
	int i=0;
	foreach(PostFile postFile, post.files){
		gettingFile.insert(i,false);
		finished.insert(i,false);
		if(postFile.filedeleted) continue;
		if(numFiles == 1){
			ui->fileInfo->show();
			ui->fileInfo->setText(postFile.infoString);
		}
		QString url = api->apiBase() + postFile.tnUrlPath;
		ClickableLabel *label = new ClickableLabel(this);
		labels.append(label);
		connect(label,&ClickableLabel::clicked,this,&ThreadForm::imageClicked);
		/*QVBoxLayout *layout = new QVBoxLayout;
		layout->setContentsMargins(13,6,0,11);
		layout->insertWidget(0,label);
		layout->setAlignment(label,Qt::AlignTop);
		ui->contentLayout->addLayout(layout,0,i++);
		label->setStyleSheet("padding:13px 6px 0px 11px");*/
		ui->contentLayout->addWidget(label,0,i,static_cast<Qt::Alignment>(Qt::AlignLeft | Qt::AlignTop));
		downloadFile(url,strings.pathBase % postFile.tnPath,nc.thumbManager,"thumb",postFile.filename % postFile.ext,label);
		//if(QFileInfo::exists(strings.pathBase % postFile.filePath) || autoExpand) getFile(label,i,false);
		//Optimize getFile?
		if(QFileInfo::exists(strings.pathBase % postFile.filePath)){
			finished.replace(i,true);
			if(post.files.at(i).ext == ".jpg" || post.files.at(i).ext == ".png") {
				loadImage(i,strings.pathBase % postFile.filePath);
			}
		}
		else if(autoExpand) getFile(label,i,false);
		i++;
	}
	if(numFiles > 1) ui->contentLayout->addItem(item,1,0,1,numFiles);
	else ui->contentLayout->addItem(item,0,i);
}

//TODO: I want this in netcontroller
void ThreadForm::downloadFile(const QString &fileUrl,
								const QString &filePath,
								QNetworkAccessManager *manager,
								QString type,
								QString message,
								ClickableLabel *label){
	if(QFileInfo::exists(filePath)){
		downloadedSlot(filePath,type,message,label);
		return;
	}
	if(networkReplies.contains(fileUrl)){
		qDebug().noquote() << "already downloading" << fileUrl << "to" << filePath;
		return;
	}
	QNetworkRequest request;
	request.setUrl(fileUrl);
	if(api->requiresUserAgent()) request.setHeader(QNetworkRequest::UserAgentHeader,api->requiredUserAgent());
	QNetworkReply *reply = manager->get(request);
	networkReplies.insert(fileUrl,reply);
	qDebug().noquote() << "downloading" << fileUrl << "to" << filePath;
	connect(reply,&QNetworkReply::finished,[=]{
		reply->deleteLater();
		QFile file(filePath);
		networkReplies.remove(fileUrl);
		if(!reply) return;
		if(reply->error()){
			qDebug().noquote().nospace() << "error downloading " << fileUrl << ": " << reply->errorString();
			return;
		}
		if(!file.open(QIODevice::WriteOnly)){
			qDebug().noquote().nospace() << "error writing " << filePath << ": Could not open for writing";
			file.close();
			return;
		};
		file.write(reply->readAll());
		file.close();
		downloadedSlot(filePath,type,message,label);
	});
}

void ThreadForm::downloadedSlot(const QString &path, const QString &type, const QString &message, ClickableLabel* label){
	if(type == "flag"){
		ui->info->update();
		QListIterator< QPointer<ThreadForm> > i(rootTF->clones);
		QPointer<ThreadForm> cloned;
		while(i.hasNext()) {
			cloned = i.next();
			if(!cloned) continue;
			cloned->strings.countryString = this->strings.countryString;
			cloned->setInfoString();
		}
	}
	else if(type == "extraFlags"){
		if(++regionsGot != regionList.size()) return;
		ui->info->update();
		QListIterator< QPointer<ThreadForm> > i(rootTF->clones);
		QPointer<ThreadForm> cloned;
		while(i.hasNext()) {
			cloned = i.next();
			if(!cloned) continue;
			cloned->strings.regionString = this->strings.regionString;
			cloned->setInfoString();
		}
	}
	else if(type == "file"){
		int ind = labels.indexOf(label);
		gettingFile.replace(ind,false);
		finished.replace(ind,true);
		if(post.files.at(ind).ext == ".jpg" || post.files.at(ind).ext == ".png") {
			loadImage(ind,path);
		}
		if(message.compare("clicked") == 0 || loadIt){
			openImage(path);
		}
	}
	else if(type == "thumb"){
		int ind = labels.indexOf(label);
		if(!finished.at(ind)){
			loadImage(ind,path);
		}
	}
}

void ThreadForm::clickImage(){
	//if(QPointer<ClickableLabel>(ui->tim)) ui->tim->clicked();
}

QPair<int,QImage> ThreadForm::scaleImage(int labelInd, QString path, int scale){
	QImage pic(path);
	QImage scaled = (pic.height() > pic.width()) ?
				pic.scaledToHeight(scale, Qt::SmoothTransformation) :
				pic.scaledToWidth(scale, Qt::SmoothTransformation);
	return QPair<int,QImage>(labelInd,scaled);
}

void ThreadForm::setPixmap(int ind, QPixmap scaled){
	ClickableLabel *label = labels.at(ind);
	label->setPixmap(scaled);
	//label->setFixedSize(scaled.size());
	//label->show();
}

void ThreadForm::scaleFinished(){
	QFutureWatcher< QPair<int,QImage> > *fw = static_cast<QFutureWatcher< QPair<int,QImage> > *>(sender());
	int labelInd = fw->result().first;
	ClickableLabel *label = labels.at(labelInd);
	QPixmap scaled = QPixmap::fromImage(fw->result().second);
	label->setPixmap(scaled);
	fw->deleteLater();
	if(!root)return;
	QListIterator< QPointer<ThreadForm> > i(clones);
	QPointer<ThreadForm> cloned;
	while(i.hasNext()) {
		cloned = i.next();
		if(!cloned) continue;
		cloned->setPixmap(labelInd,scaled);
	}
}

void ThreadForm::loadImage(int labelInd, QString path) {
	QSettings settings;
	int maxWidth = settings.value("imageSize",250).toInt();
	QFutureWatcher< QPair<int,QImage> > *fw = new QFutureWatcher< QPair<int,QImage> >(this);
    connect(fw,&QFutureWatcher< QPair<int,QImage> >::finished,this,&ThreadForm::scaleFinished);
    fw->setFuture(QtConcurrent::run(&ThreadForm::scaleImage,labelInd,path,maxWidth));
	//Possible to wrap text around image; decided not to for now
	//no smooth scaling doesn't look good
	//or scaling then storing image as base64 data takes too much memory
	//save scaled thumbnails? use qml?
	/*
	QImage scaled = scaleImage(path, settings.value("imageSize",250).toInt());
	QString url;
	if(!scaled.isNull()) {
		QPixmap pixmap = QPixmap::fromImage(scaled);
		QByteArray byteArray;
		QBuffer buffer(&byteArray);
		pixmap.save(&buffer, "JPG");
		url = QString("<img style=\"float:left\" src=\"data:image/jpg;base64,") + byteArray.toBase64() + "\"/>";
	}
	ui->com->setText("<a href=\"#t" % path % "\">" % url % "</a><p>" + post.com + "</p>");
	*/
}

void ThreadForm::imageClicked()
{
	if(!labels.size())return;
	ClickableLabel *label = qobject_cast<ClickableLabel*>(sender());
	int ind = labels.indexOf(label);
	if(ind == -1){
		ind=0;
		label = labels.at(0);
	}
	//qDebug().noquote().nospace() << "clicked " << post.files.at(ind).filename << post.files.at(ind).ext;
	if(strings.path == "index") {
		TreeItem *childOf = mw->model->getItem(mw->selectionModel->currentIndex());
		mw->onNewThread(mw,api,strings.board,strings.thread,QString(),childOf);
	}
	else{
		if(finished.at(ind)) openImage(strings.pathBase % post.files.at(ind).filePath);
		else if(gettingFile.at(ind)) loadIt = true;
		else getFile(label,ind,true);
	}
}

void ThreadForm::hideClicked()
{
	QSettings settings;
	//deprecated
	QString filterString = "filters/" % api->name() % '/' % board % "/id";
	QStringList idFilters = settings.value(filterString).toStringList();
	idFilters.append(strings.thread);
	settings.setValue(filterString,idFilters);
	//
	qDebug().noquote().nospace() << "hide Clicked so " << api->name() << '/' << board << '/' << post.no << " filtered!";
	this->hidden = true;
	emit removeMe(this);
	if(strings.thread != "index"){
		QListIterator< QPointer<ThreadForm> > i(clones);
		while(i.hasNext()) {
			i.next()->deleteLater();
		}
		QSet<QString> quotes = post.quotelinks;
		ThreadForm *replyTo;
		//TODO use QtConcurrent?
		foreach (const QString &orig, quotes)
		{
			replyTo = static_cast<ThreadTab*>(tab)->tfMap.find(orig).value();
			if(replyTo != nullptr) {
				replyTo->replies.remove(post.no.toDouble());
				replyTo->setReplies();
			}
		}
	}
}

void ThreadForm::openImage(const QString &path)
{
	QDesktopServices::openUrl(
				QUrl::fromLocalFile(
					QDir().absoluteFilePath(path)));
}

void ThreadForm::setFontSize(int fontSize){
	QFont temp = this->ui->info->font();
	temp.setPointSize(fontSize-4);
	this->ui->fileInfo->setFont(temp);
	temp.setPointSize(fontSize-2);
	this->ui->info->setFont(temp);
	temp.setPointSize(fontSize);
	this->ui->com->setFont(temp);
	if(root && clones.size()){
		QListIterator< QPointer<ThreadForm> > i(clones);
		while(i.hasNext()) {
			QPointer<ThreadForm> next = i.next();
			if(!next) continue;
			next->setFontSize(fontSize);
		}
	}
}

void ThreadForm::setImageSize(int imageSize){
	(void)imageSize;
	reloadFiles();
}

void ThreadForm::reloadFiles(){
	int i = 0;
	//TODO map label to current loaded image?
	foreach(PostFile postFile, post.files){
		if(QFileInfo::exists(strings.pathBase % postFile.filePath))
			loadImage(i,strings.pathBase % postFile.filePath);
		else if(QFileInfo::exists(strings.pathBase % postFile.tnPath))
			loadImage(i,strings.pathBase % postFile.tnPath);
		i++;
	}
}

//TODO put most of this in another thread
void ThreadForm::setRegion(const QString &region){
	regionList = region.split("||");
	if(regionList.isEmpty()) return;
	regionString = "<span style=\"color:lightblue\">" % region % "</span> ";
	ui->info->setText(infoString());
	QString flegUrl(strings.flegsUrlBase % post.country_name);
	QString flegPath("flags/flegs/" % post.country_name);
	QString temp(regionList.takeLast());
	QDir().mkpath(flegPath % '/' % regionList.join('/'));
	regionList.append(temp);
	int i = 0;
	regionString.clear();
	foreach(QString reg, regionList){
		flegPath += '/' % reg;
		flegUrl += '/' % reg;
		downloadFile(QUrl(flegUrl % ".png").toString(QUrl::FullyEncoded),flegPath % ".png",nc.fileManager);
		//regionString += flagString(flegPath % ".png",reg);
		regionString += "<a href=\"" % reg % "\" style=\"color:lightblue;text-decoration:none\" text><img src=\"" % flegPath % ".png\" width=\"32\" height=\"20\"> " % reg % "</a> ";
		i++;
	}
	ui->info->setText(infoString());
}

void ThreadForm::quoteClicked(const QString &link)
{
	qDebug().noquote() << link;
	if(link.startsWith("#p") && this->type == PostType::Reply) {
		QString postNo = link.mid(2);
		if(inserted.contains(postNo)){
			inserted.value(postNo)->deleteLater();
		}
		else{
			ThreadForm *tf = static_cast<ThreadTab*>(tab)->tfMap.value(postNo);
			if(tf != nullptr && !tf->hidden) this->insert(tf);
		}
	}
	else if(link.startsWith("#op")){
		if(this->type == PostType::Reply) static_cast<ThreadTab*>(tab)->quoteIt(">>"+post.no);
		else imageClicked();
	}
	else if(link.startsWith("#f")){
		postMenu();
	}
	else if(!link.isEmpty() && link.at(0)=='/') {
		mw->loadFromSearch(link,QString(),Q_NULLPTR,false);
	}
}

void ThreadForm::postMenu(){
	//tfc will delete on close
	ThreadFormContext *tfc = new ThreadFormContext(&post);
	connect(tfc,&ThreadFormContext::filtersChanged,mw,&MainWindow::reloadFilters,Qt::DirectConnection);
}

void ThreadForm::insert(ThreadForm *tf)
{
	ThreadForm *newtf = tf->clone(replyLevel);
	inserted.insert(tf->post.no,newtf);
	QMetaObject::Connection removeOnDestroy = connect(newtf,&ThreadForm::destroyed,this,&ThreadForm::removeFromInserted);
	insertedConnections.insert(tf->post.no,removeOnDestroy);
	if(ui->quoteWidget->isHidden())ui->quoteWidget->show();
	ui->quotes->addWidget(newtf);
}

void ThreadForm::removeFromInserted(){
	ThreadForm *tf = static_cast<ThreadForm*>(sender());
	QString postNo = tf->post.no;
	if(!postNo.isEmpty() && inserted.contains(postNo)){
		inserted.remove(postNo);
		insertedConnections.remove(postNo);
	}
}

void ThreadForm::addReply(ThreadForm *tf){
	if(ui->quoteWidget->isHidden()) ui->quoteWidget->show();
	tf->setParent(this);
	ui->quotes->addWidget(tf);
}

ThreadForm *ThreadForm::clone(int replyLevel)
{
	ThreadForm *tfs = new ThreadForm(this->api,this->strings,false,false,tab,replyLevel+1);
	tfs->rootTF = this->rootTF;
	//tfs->tab = tab;
	tfs->post = this->post;
	tfs->ui->com->setText(post.com);
	tfs->ui->info->setText(ui->info->text());
	tfs->replies = replies;
	//TODO check and account for if original is still getting file
	/*
	if(post.files.size() && !post.files.at(0).filedeleted) {
		tfs->strings = this->strings;
		tfs->files = files;
		tfs->thumbs = thumbs;
		tfs->ui->fileInfo->show();
		tfs->ui->fileInfo->setText(ui->fileInfo->text());
		const QPixmap *px = this->ui->tim->pixmap();
		//From load image but don't have to scale again
		tfs->ui->tim->show();
		//TODO make sure clone fits in window
		//tfs->setMinimumWidth(738);
		if(px){
			tfs->ui->tim->setPixmap(*px);
			tfs->ui->tim->setFixedSize(px->size());
			connect(tfs->ui->tim,&ClickableLabel::clicked,rootTF,&ThreadForm::imageClicked);
		}
	} else {
		tfs->ui->pictureLayout->deleteLater();
		tfs->hasImage = false;
	}*/
	int numFiles = post.files.size();
	if(numFiles){
		QString fileInfoText = ui->fileInfo->text();
		if(!fileInfoText.isEmpty()){
			tfs->ui->fileInfo->show();
			tfs->ui->fileInfo->setText(fileInfoText);
		}
		QLayoutItem *item = tfs->ui->contentLayout->takeAt(0);
		int i=0;
		foreach(ClickableLabel *label, labels){
			ClickableLabel *tfsLabel = new ClickableLabel(tfs);
			tfs->labels.append(tfsLabel);
			connect(tfsLabel,&ClickableLabel::clicked,this,&ThreadForm::imageClicked);
			tfs->ui->contentLayout->addWidget(tfsLabel,0,i,static_cast<Qt::Alignment>(Qt::AlignLeft | Qt::AlignTop));
			const QPixmap *pixToSet = label->pixmap();
			if(pixToSet) tfsLabel->setPixmap(*pixToSet);
			i++;
		}
		if(numFiles > 1) tfs->ui->contentLayout->addItem(item,1,0,1,numFiles);
		else tfs->ui->contentLayout->addItem(item,0,i);
	}

	tfs->regionString = regionString;
	if(repliesString.length()) {
		tfs->setRepliesString(repliesString);
	}
	//is load okay?
	//tfs->load(post);
	rootTF->clones.append(tfs);
	disconnect(tfs->ui->hide,&ClickableLabel::clicked,tfs,&ThreadForm::hideClicked);
	connect(tfs->ui->hide,&ClickableLabel::clicked,tfs,&ThreadForm::deleteLater);
	connect(tfs,&ThreadForm::removeMe,rootTF,&ThreadForm::removeClone);
	//TODO load and connect cross thread replies
	if(this->type == PostType::Reply){
		ThreadTab* temp = static_cast<ThreadTab*>(tab);
		connect(tfs,&ThreadForm::floatLink,temp,&ThreadTab::floatReply);
		connect(tfs,&ThreadForm::updateFloat,temp,&ThreadTab::updateFloat);
		connect(tfs,&ThreadForm::deleteFloat,temp,&ThreadTab::deleteFloat);
	}
	return tfs;
}

//TODO don't run this if destroying the whole threadtab
void ThreadForm::removeClone(QPointer<ThreadForm> tf)
{
	if(tf) clones.removeOne(tf);
}
void ThreadForm::addReplyLink(QString &reply, bool isYou){
	QString temp =" <a href=\"#p" % reply % "\" style=\"color:#897399\">&gt;&gt;" % reply % ((isYou) ? " (You)</a>" : "</a>");
	repliesString += temp;
	ui->info->setText(infoString());
	//update clones
	QListIterator< QPointer<ThreadForm> > i(clones);
	while(i.hasNext()) {
		QPointer<ThreadForm> next = i.next();
		if(!next) continue;
		next->setRepliesString(repliesString);
	}
}

void ThreadForm::setReplies()
{
	repliesString = "";
	QList<QString> list = replies.values();
	if(list.length()) {
		foreach (const QString &reply, list)
		{
			repliesString+=" <a href=\"#p" % reply % "\" style=\"color:#897399\">>>" % reply % (you.hasYou(api->name(),board,reply) ? " (You)</a>" : "</a>");
		}
		repliesString = repliesString.mid(1);
		ui->info->setText(infoString());
		//update clones
		QListIterator< QPointer<ThreadForm> > i(clones);
		while(i.hasNext()) {
			QPointer<ThreadForm> next = i.next();
			if(!next) continue;
			next->repliesString = repliesString;
			next->setInfoString();
		}
	}
	else{
		//ui->replies->hide();
	}
}

void ThreadForm::setRepliesString(const QString &repliesString)
{
	this->repliesString = repliesString;
	setInfoString();
}

void ThreadForm::setInfoString()
{
	ui->info->setText(infoString());
}

//showing and supposedToShow to save states and restore floating replies and quotes
void ThreadForm::on_info_linkHovered(const QString &link)
{
	if(this->type == PostType::Reply) {
		if(link.startsWith("#op")){
			emit deleteFloat();
		}
		else if(link.startsWith("#p")) {
			qDebug().noquote() << "hovering" << link;
			emit floatLink(link.mid(2),replyLevel);
		} else {
			//TODO check mouse cursor?
			emit deleteFloat();
		}
	}
}

bool ThreadForm::eventFilter(QObject *obj, QEvent *event)
{
	if(this->type == PostType::Reply){
		if(event->type() == QEvent::MouseMove) {
			emit updateFloat();
		}
		else if(event->type() == QEvent::Leave) {
			emit deleteFloat();
		}
	}
	if((event->type() == QEvent::MouseButtonRelease)
			&& (obj->objectName() == "com" || obj->objectName() == "info" || obj->objectName() == "fileInfo")){
		QLabel *temp = static_cast<QLabel*>(obj);
		if(temp->hasSelectedText()){
			temp->setTextInteractionFlags(
				Qt::TextSelectableByKeyboard |
				Qt::TextSelectableByMouse |
				Qt::LinksAccessibleByKeyboard |
				Qt::LinksAccessibleByMouse);
		}
		else{
			temp->setTextInteractionFlags(
				Qt::TextSelectableByMouse |
				Qt::LinksAccessibleByKeyboard |
				Qt::LinksAccessibleByMouse);
			ui->hide->setFocus();
			temp->setFocus();
		}
	}
	//can I just use replaceWidget instead of getItemPosition, takeAt, addWidget?
	if(event->type() == QEvent::Resize && obj->objectName() == "com"){
		QLabel *temp = qobject_cast<QLabel*>(obj); //ui->com
		if(temp->sizeHint().width() >= temp->size().width()){
			if(temp->parent()->objectName() != "qt_scrollarea_viewport"){
				int index = ui->contentLayout->indexOf(temp);
				if(index != -1){
					int row,col,rs,cs;
					ui->contentLayout->getItemPosition(index,&row,&col,&rs,&cs);
					ui->contentLayout->takeAt(index);
					QScrollArea *scroller = new QScrollArea(this);
					scroller->setFrameShape(QScrollArea::Shape::NoFrame);
					scroller->setWidgetResizable(true);
					scroller->setObjectName("scroller");
					scroller->setWidget(temp);
					ui->contentLayout->addWidget(scroller,row,col,rs,cs);
				}
			}
		}
		else if(temp->parent()->objectName() == "qt_scrollarea_viewport"){
			QScrollArea *scroller = qobject_cast<QScrollArea*>(temp->parent()->parent());
			int index = ui->contentLayout->indexOf(scroller);
			if(index != -1){
				int row,col,rs,cs;
				ui->contentLayout->getItemPosition(index,&row,&col,&rs,&cs);
				ui->contentLayout->takeAt(index);
				scroller->deleteLater();
				ui->contentLayout->addWidget(ui->com,row,col,rs,cs);
			}
		}
	}
	return QObject::eventFilter(obj, event);
}

void ThreadForm::on_com_linkHovered(const QString &link)
{
	on_info_linkHovered(link);
}

void ThreadForm::deleteHideLayout()
{
	ui->hide->deleteLater();
	ui->quoteWidget->deleteLater();
	hideButtonShown = false;
}
