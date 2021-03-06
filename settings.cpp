#include "settings.h"
#include "ui_settings.h"
#include <QSettings>
#include <QFile>
#include <QDebug>

Settings::Settings(QTabWidget *parent) :
	QTabWidget(parent),
	ui(new Ui::Settings)
{
	ui->setupUi(this);
	bColorRegExp.setPattern("background-color[^:]*:\\s*(?<bColor>[^\\s;$]*)?");
	refreshValues();
	connect(ui->autoExpandLabel,&ClickableLabel::clicked,this,&Settings::clicked);
	connect(ui->autoUpdateLabel,&ClickableLabel::clicked,this,&Settings::clicked);
	connect(ui->sessionFileLabel,&ClickableLabel::clicked,this,&Settings::clicked);
	connect(ui->showIndexRepliesLabel,&ClickableLabel::clicked,this,&Settings::clicked);
	connect(ui->use4chanPassLabel,&ClickableLabel::clicked,this,&Settings::clicked);
	connect(ui->autoExpand,&QCheckBox::clicked,this,&Settings::clicked);
	connect(ui->autoUpdate,&QCheckBox::clicked,this,&Settings::clicked);
	connect(ui->showIndexReplies,&QCheckBox::clicked,this,&Settings::clicked);
	connect(ui->use4chanPass,&QCheckBox::clicked,this,&Settings::clicked);
}

Settings::~Settings()
{
	delete ui;
}

void Settings::clicked()
{
	QSettings settings;
	QObject *obj = sender();
	QString sender = obj->objectName();
	qDebug() << sender;
	if(sender == "sessionFileLabel") {
		QString sessionFile = ui->sessionFile->text();
		qDebug () << "setting sessionFile to" << sessionFile;
		settings.setValue("sessionFile",sessionFile);
		emit update("sessionFile", sessionFile);
		//refreshValues();
	}
	else if(sender == "autoExpandLabel" || sender == "autoExpand") {
		bool autoExpand = !settings.value("autoExpand", !ui->autoExpand->isChecked()).toBool();
		qDebug () << "setting autoExpand to" << autoExpand;
		settings.setValue("autoExpand",autoExpand);
		emit update("autoExpand", autoExpand);
		ui->autoExpand->setChecked(autoExpand);
	}
	else if(sender == "autoUpdateLabel" || sender == "autoUpdate") {
		bool autoUpdate = !settings.value("autoUpdate", !ui->autoUpdate->isChecked()).toBool();
		qDebug () << "setting autoUpdate to" << autoUpdate;
		settings.setValue("autoUpdate",autoUpdate);
		emit update("autoUpdate", autoUpdate);
		ui->autoUpdate->setChecked(autoUpdate);
	}
	else if(sender == "showIndexRepliesLabel" || sender == "showIndexReplies") {
		bool showIndexReplies = !settings.value("showIndexReplies", !ui->showIndexReplies->isChecked()).toBool();
		qDebug () << "setting showIndexReplies to" << showIndexReplies;
		settings.setValue("showIndexReplies",showIndexReplies);
		emit update("showIndexReplies", showIndexReplies);
		ui->showIndexReplies->setChecked(showIndexReplies);
	}
	else if(sender == "use4chanPassLabel" || sender == "use4chanPass") {
		bool use4chanPass = !settings.value("use4chanPass", !ui->use4chanPass->isChecked()).toBool();
		qDebug () << "setting use4chanPass to" << use4chanPass;
		settings.setValue("use4chanPass",use4chanPass);
		emit update("use4chanPass", use4chanPass);
		ui->use4chanPass->setChecked(use4chanPass);
	}
}

void Settings::refreshValues()
{
	QSettings settings;
	ui->autoExpand->setChecked(settings.value("autoExpand",false).toBool());
	ui->autoUpdate->setChecked(settings.value("autoUpdate",false).toBool());
	ui->showIndexReplies->setChecked(settings.value("showIndexReplies",false).toBool());
	ui->sessionFile->setText(settings.value("sessionFile","session").toString());
	ui->use4chanPass->setChecked(settings.value("use4chanPass",false).toBool());
	ui->styleMainWindowEdit->setText(settings.value("style/MainWindow","background-color: #191919; color:white").toString());
	ui->styleThreadFormEdit->setText(settings.value("style/ThreadForm","color:#bbbbbb;").toString());
}

void Settings::showEvent(QShowEvent *event)
{
	(void)event;
	refreshValues();
}




void Settings::on_sessionFile_editingFinished()
{
	QSettings settings;
	QString text = ui->sessionFile->text();
	if(!text.isEmpty()){
		qDebug().noquote() << "setting sessionFile to" << text;
		settings.setValue("sessionFile",text);
		emit update("sessionFile",text);
	}
}

void Settings::on_styleMainWindowEdit_editingFinished()
{
	QSettings settings;
	QString text = ui->styleMainWindowEdit->text();
	if(!text.isEmpty()){
		qDebug().noquote() << "setting style/MainWindow to" << text;
		settings.setValue("style/MainWindow",text);
		QRegularExpressionMatch match = bColorRegExp.match(text);
		if(match.hasMatch()){
			settings.setValue("style/MainWindow/background-color",match.captured("bColor"));
		}
		else settings.setValue("style/MainWindow/background-color",QString());
		emit update("style/MainWindow",text);
	}
}

void Settings::on_styleThreadFormEdit_editingFinished()
{
	QSettings settings;
	QString text = ui->styleThreadFormEdit->text();
	if(!text.isEmpty()){
		qDebug().noquote() << "setting style/ThreadForm to" << text;
		settings.setValue("style/ThreadForm",text);
		QRegularExpressionMatch match = bColorRegExp.match(text);
		if(match.hasMatch()){
			settings.setValue("style/ThreadForm/background-color",match.captured("bColor"));
		}
		else settings.setValue("style/ThreadForm/background-color",QString());
		emit update("style/ThreadForm",text);
	}

}
