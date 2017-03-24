#include "netcontroller.h"
#include "threadtab.h"
#include "ui_threadtab.h"
#include "threadform.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QShortcut>
#include <QKeySequence>
#include <QDir>
#include "postform.h"
#include <QKeyEvent>

ThreadTab::ThreadTab(QString board, QString thread, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ThreadTab)
{
    this->updated = false;
    ui->setupUi(this);
    this->board = board;
    this->thread = thread;
    //QDir().mkpath(board+"/"+thread);
    QDir().mkpath(board+"/"+thread+"/thumbs");
    threadUrl = "https://a.4cdn.org/"+board+"/thread/"+thread+".json";
    request = QNetworkRequest(QUrl(threadUrl));
    request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    reply = nc.manager->get(request);
    connect(reply, &QNetworkReply::finished, this, &ThreadTab::loadPosts);
    myProcess = new QProcess(parent);
    QAction *foo = new QAction(this);
    foo->setShortcut(Qt::Key_G);
    connect(foo, &QAction::triggered, this, &ThreadTab::gallery);
    this->addAction(foo);
    QAction *postForm = new QAction(this);
    postForm->setShortcut(Qt::Key_Q);
    connect(postForm, &QAction::triggered, this, &ThreadTab::openPostForm);
    this->addAction(postForm);
    QAction *refresh = new QAction(this);
    refresh->setShortcut(Qt::Key_R);
    connect(refresh, &QAction::triggered, this, &ThreadTab::getPosts);
    this->addAction(refresh);
    QAction *focusSearch = new QAction(this);
    focusSearch->setShortcut(Qt::ControlModifier+Qt::Key_F);
    connect(focusSearch,&QAction::triggered,this,&ThreadTab::focusIt);
    this->addAction(focusSearch);
    //this->installEventFilter(this);
}

void ThreadTab::getPosts(){
    qDebug() << "refreshing /" + board + "/" + thread;
    reply = nc.manager->get(request);
    connect(reply, &QNetworkReply::finished, this, &ThreadTab::loadPosts);
}

ThreadTab::~ThreadTab()
{
    int i = tfs.size();
    while(i--){
        ((ThreadForm*)tfs.at(i))->deleteLater();
        tfs.pop_back();
    }
    delete ui;
}

void ThreadTab::openPostForm(){
    PostForm *myPostForm = new PostForm(board,thread);
    myPostForm->show();
}

void ThreadTab::gallery(){
    QString command = "feh"; QStringList arguments; arguments << QDir("./"+board+"/"+thread).absolutePath()
                                                              << "--auto-zoom"
                                                              << "--index-info" << "\"%n\n%S\n%wx%h\""
                                                              << "--borderless"
                                                              << "--image-bg" << "black"
                                                              << "--preload";
    myProcess->startDetached(command,arguments);
}

void ThreadTab::addPost(ThreadForm *tf){
    ui->threads->addWidget(tf);
}

void ThreadTab::addStretch(){
    ui->threads->addStretch(1);
}

void ThreadTab::loadPosts(){
    QJsonArray posts = QJsonDocument::fromJson(reply->readAll()).object()["posts"].toArray();
    int length = posts.size();
    qDebug() << QString("length is ").append(QString::number(length));
    int i=tfs.size();
    for(;i<length;i++){
        QJsonObject p = posts.at(i).toObject();
        ThreadForm *tf = new ThreadForm(board,thread);
        ui->threads->addWidget(tf);
        connect(tf,&ThreadForm::searchPost,this,&ThreadTab::findPost);
        tf->load(p);
        tfs.push_back(tf);
    }
    ui->threads->addStretch(1);
    reply->deleteLater();
}

void ThreadTab::updatePosts(){
    updated = false;
    int length = tfs.size();
    for(int i=0;i<length;i++){
        ((ThreadForm*)tfs.at(i))->updateComHeight();
    }
}

void ThreadTab::findPost(QString postNum, ThreadForm* thetf){
    int size = tfs.size();
    for(int i=0;i<size;i++){
        ThreadForm* tf = (ThreadForm*)(tfs.at(i));
        if(tf->post->no == postNum)
            //qobject_cast<ThreadForm*>(sender())->insert(position,tf);
            //qobject_cast<ThreadForm*>(sender())->insert(tf);
            thetf->insert(tf);
    }
}

void ThreadTab::findText(const QString text){
    QRegularExpression re(text,QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match;
    ThreadForm* tf;
    bool pass = false;
    if (text == "") pass = true;
    int size = tfs.size();
    for(int i=0;i<size;i++){
        tf = (ThreadForm*)(tfs.at(i));
        if(pass) { tf->show(); continue;};
        match = re.match(tf->post->com);
        if(!match.hasMatch()){
            tf->hide();
        }
    }
}

void ThreadTab::on_pushButton_clicked()
{
    findText(ui->lineEdit->text());
}

bool ThreadTab::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        int mod = keyEvent->modifiers();
        int key = keyEvent->key();
        qDebug("Ate modifier %d",mod);
        qDebug("Ate key press %d", key);
        qDebug() << obj->objectName();
        if(mod == 67108864 && key == 70){
            //ui->lineEdit->setFocus();
        }
        qDebug("Ate key press %d", key);
        if(key == 16777220){
            on_pushButton_clicked();
        }
        else if(key == 16777269){
            ui->lineEdit->setFocus();
        }
        else{
            return QObject::eventFilter(obj, event);
        }
        return true;
    } else {
        // standard event processing
        return QObject::eventFilter(obj, event);
    }
}

void ThreadTab::focusIt(){
    ui->lineEdit->setFocus();
}

void ThreadTab::on_lineEdit_returnPressed()
{
    findText(ui->lineEdit->text());
}
