#include "filter.h"
#include "you.h"
#include "chans.h"
#include "netcontroller.h"
#include "notificationview.h"
#include "mainwindow.h"
#include <QSettings>
#include <QApplication>
#include <QStandardPaths>

MainWindow *mw;
NotificationView *nv;
netController nc;
Chan *fourChanAPI = new FourChan();
Chan *eightChanAPI = new EightChan();
Chan *twoChHkAPI = new TwoChHk();
You you;
Filter filter;

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QCoreApplication::setOrganizationName("qtchan");
	QCoreApplication::setApplicationName("qtchan");
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation));
	//QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	//QApplication::setAttribute(QT_SCALE_FACTOR,"1.4");
	//qputenv("QT_SCALE_FACTOR","0.2");
	QFont font = QGuiApplication::font();
	QSettings settings;
	font.setPointSize(settings.value("fontSize",14).toInt());
	a.setFont(font);
	//flag path
	QDir().mkpath("flags/troll");
	MainWindow w;
	mw = &w;
	w.show();
	NotificationView view;
	nv = &view;
	/*NotificationTray t;
	t.setIcon(QIcon(":/icons/icon_22x22.png"));
	t.show();*/
	QString sessionSlot = settings.value("sessionSlot","0").toString();
	w.loadSession(sessionSlot);
	return a.exec();
}
