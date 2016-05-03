#include <QCoreApplication>
#include <QFile>
#include <QDebug>
#include <QByteArray>
#include <QList>
#include <QSharedPointer>
#include <QRegularExpression>
#include <QImage>
#include <QXmlStreamReader>
#include <QListIterator>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>
#include <QBuffer>

#ifdef OS_WINDOWS
QDir outputSite("c:/Developpement");
QDir siteDir("C:/Developpement");
#else
QDir outputSite(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
QDir siteDir(QFileInfo(QDir::homePath(),"GDrive/site/").absolutePath());
#endif

const QByteArray REF_TAG_START="<!--REF";
const QByteArray REF_TAG_END="<!--/REF";
const QByteArray ICON_TAG="<!--ICON";
const QByteArray DOCUMENT_IMAGE="<!--IMG";
const QByteArray RMS_TAG="<!--REMOVESTART";
const QByteArray RME_TAG="<!--REMOVEEND";
const QByteArray STYLE_TAG="<!--EMBSTYLE";
const QByteArray SCREEN_TAG="<!--SCREEN";
const QByteArray BOOK_TAG="<!--BOOK";
const QByteArray EYOU_TAG="<!--EYOU";

bool inRemove =false;


enum TypeComment {
    VOID,
    DUMMY,
    REMOVE_START,
    REMOVE_END,
    REFERENCE_START,
    REFERENCE_END,
    ICON,
    GAMECOVERS,
    STYLE,
    SCREEN,
    BOOK,
    IMG,
    EYOU
};

class Entity;

class Comment {
public :
    qint64 commentStart,  commentEnd;
    TypeComment typeComment;
    QByteArray comment;
    bool autoClosing;
    TypeComment closingTag;
    bool html;

    Comment(TypeComment typeComment,qint64 commentStart, qint64 commentEnd, const QByteArray& comment=QByteArray(), TypeComment closingTag=VOID, bool html=true){
        this->commentStart=commentStart;
        this->commentEnd=commentEnd;
        this->typeComment=typeComment;
        this->comment= comment;
        this->autoClosing = closingTag==VOID;
        this->closingTag = closingTag;
        this->html = html;
    }

    bool isAutoClosing() const{
        return autoClosing;
    }

    bool isHTML() const{
        return html;
    }

    TypeComment getClosingTag() const{
        return closingTag;
    }

    TypeComment getTag() const {
        return typeComment;
    }

    qint64 getCommentStart() const{
        return commentStart;
    }

    qint64 getCommentEnd() const{
        return commentEnd;
    }

    QString getCommentInternalText(const QString tag) const {
        return comment.mid(tag.length(),comment.length()-tag.length()-3);
    }

    QMap<QString,QString> extractValue(const QString& tag)const{
        QMap<QString,QString> result;

        QStringList tokens= getCommentInternalText(tag).split(",",QString::SkipEmptyParts);

        for (const QString& token : tokens){
            QStringList values= token.split("=",QString::SkipEmptyParts);

            if (values.size()!=2) {
                throw "fddfdf";
            }

            result[values[0].trimmed().toUpper()]=values[1].trimmed();
        }

        return result;
    }

    virtual void output(Entity* entity, QIODevice& file) const {
        /*  if (typeComment==REFERENCE){
            file.write(comment);
            file.write("<!--REMOVESTART-->");
            file.write("<div>testTTT</div>");
            file.write("<!--REMOVEEND-->");
        }*/
    }
};



QString readTemplate(const QString& template_file_name){
    // ://templates/numero.js

    QFile fil(template_file_name);
    if (!fil.open(QIODevice::ReadOnly | QIODevice::Text))
        return "";

    QTextStream file_utf8(&fil);
    return file_utf8.readAll();

}



class IconComment : public Comment {
public:
    IconComment(qint64 commentStart, qint64 commentEnd, const QByteArray& comment=QByteArray()):
        Comment(ICON,commentStart,commentEnd,comment){
    }

    virtual void output(Entity* entity,QIODevice& file) const{


    }
};

class ImageComment : public Comment {
public:
    ImageComment(qint64 commentStart, qint64 commentEnd, const QByteArray& comment=QByteArray()):
        Comment(IMG,commentStart,commentEnd,comment){
    }

    virtual void output(Entity* entity, QIODevice& file) const{
        auto v = extractValue(DOCUMENT_IMAGE);

        for(auto e : v.keys())
        {
          qDebug() << e << "," << v.value(e) << '\n';
        }

    }
};

class RefStartComment : public Comment {
public:
    RefStartComment(qint64 commentStart, qint64 commentEnd, const QByteArray& comment=QByteArray()):
        Comment(REFERENCE_START,commentStart,commentEnd,comment,REFERENCE_END){
    }

    virtual void output(Entity* entity,QIODevice& file) const{

    }
};

class RefEndComment : public Comment {
public:
    RefEndComment(qint64 commentStart, qint64 commentEnd, const QByteArray& comment=QByteArray()):
        Comment(REFERENCE_END,commentStart,commentEnd,comment){
    }

    virtual void output(Entity* entity, QIODevice& file) const{

    }
};

class StyleComment : public Comment {
public:
    StyleComment(qint64 commentStart, qint64 commentEnd, const QByteArray& comment=QByteArray()):
        Comment(STYLE,commentStart,commentEnd,comment){
    }

    void output(Entity* entity , QIODevice& )const;
};

class ScreenComment : public Comment {
public:
    ScreenComment(qint64 commentStart, qint64 commentEnd, const QByteArray& comment=QByteArray()):
        Comment(SCREEN,commentStart,commentEnd,comment){
    }

    virtual void output(Entity* entity, QIODevice& iodevice) const{
        QString imageUrl=this->getCommentInternalText(SCREEN_TAG).trimmed();

        if (!imageUrl.isEmpty()){
            iodevice.write(QString("<div class=\"atari-screen\" style=\"background: url(\'").append(imageUrl).append("'\);\"></div>").toUtf8());
        }
    }
};

class EYouComment : public Comment {
public:
    EYouComment(qint64 commentStart, qint64 commentEnd, const QByteArray& comment=QByteArray()):
        Comment(EYOU,commentStart,commentEnd,comment){
    }

    virtual void output(Entity* entity, QIODevice& iodevice) const{
        auto v = extractValue(EYOU_TAG);

        QString url = v["URL"];
        if (url.isEmpty()){
            return;

        }
//      <iframe width="560" height="315" src="https://www.youtube.com/embed/vlDYoATH_jI" frameborder="0" allowfullscreen></iframe>
      //<iframe width="560" height="315" src="https://www.youtube.com/embed/vlDYoATH_jI" frameborder="0" allowfullscreen>
    //    <iframe width="560" height="315" src="https://youtu.be/vlDYoATH_jI" frameborder="0" allowfullscreen></iframe>

        iodevice.write(QString("<iframe width=\"560\" height=\"315\" src=\"https://www.youtube.com/embed/").append(url).append("\" frameborder=\"0\" allowfullscreen></iframe>").toUtf8());

    }
};

class BookComment : public Comment {
public:
    BookComment(qint64 commentStart, qint64 commentEnd, const QByteArray& comment=QByteArray()):
        Comment(BOOK,commentStart,commentEnd,comment){
    }

    virtual void output(Entity* entity, QIODevice& iodevice) const{

        QStringList tokens= getCommentInternalText(BOOK_TAG).split(",",QString::SkipEmptyParts);


        QString title = tokens.takeFirst();
        QString location=tokens.takeFirst();

        QStringList pages = tokens;

        QString declPagesArray="var pages = [";

        QString lastPage = pages.takeLast();

        for(const QString& pageNumber: pages){
            declPagesArray.append('"').append(pageNumber).append("\",\n");
        }

        declPagesArray.append('"').append(lastPage).append("\"];\n");

        qDebug() << declPagesArray << location;

        QFileInfo bookDir(outputSite,location);
        if (!bookDir.exists()){
            return;
        }

        QFile outputJsFile(QFileInfo(QDir(bookDir.absolutePath()),"numero.js").absoluteFilePath());

        if (!outputJsFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)){
            return ;
        }

        outputJsFile.write(declPagesArray.toUtf8());
        outputJsFile.write(readTemplate("://templates/numero.js").toUtf8());


        QFile outputHtmlFile(QFileInfo(QDir(bookDir.absolutePath()),"numero.html").absoluteFilePath());

        if (!outputHtmlFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)){
            return ;
        }

        outputHtmlFile.write(readTemplate("://templates/numero.html").toUtf8());

        QString anchor=QString("<a href=\"").append(location).append("numero.html#page/1/mode/2up\">").append(title).append("</a>");
        iodevice.write(anchor.toUtf8());


    }
};



void outputStringList(QIODevice& output, const QStringList& sl){
    QListIterator<QString> itr (sl);
    while (itr.hasNext()) {
        output.write(QString(itr.next()).toUtf8());

    }
}

QStringList static_header_1{
    "<!DOCTYPE html>",
    "<html>",
    "<head>",
    "<meta charset=\"utf-8\" />"

};

QStringList static_header_2{
    "</head>",
    "<body>"
};

QStringList static_footer {
    "",
    "</body>",
    "</html>"
};


void generatePageFooter(QIODevice& output){
    outputStringList(output,static_footer);
}

class Entity {
public :

    Entity(QFileInfo& fromFile);

    void parse();

    void addEmbeddedStyle(const QString& style);

private:
    void extractCommentPosition();
    bool executeComment();
    void createComment( QByteArray& qb,qint64 commentStart,qint64 commentEnd);
    void handleTag( const Comment* openingTag, const Comment* closingTag, QTextStream& file, QIODevice& output);

    const Comment* findClosingTag(const Comment* p, QTextStream& file );
    const Comment* isInComment(qint64 pos);
    const Comment* isSpecial(qint64 pos);
    bool isInOuput(qint64 pos);
    void check();

    QFileInfo fileInfo;
    QList<QSharedPointer<Comment> > comments;
    QStringList embedded_styles;

};

void StyleComment::output(Entity* entity , QIODevice& ) const{
    entity->addEmbeddedStyle(getCommentInternalText(STYLE_TAG));
}


Entity::Entity(QFileInfo &fromFile){
    fileInfo=fromFile;
}

void Entity::addEmbeddedStyle(const QString& style){
    embedded_styles.append(style);
}

const Comment* Entity::findClosingTag(const Comment* p, QTextStream& file ){
    while (!file.atEnd()) {
        qint64 pos=file.pos();
        QChar c1;
        file >> c1;
        const Comment* p2= isSpecial(pos);

        if (p2!=nullptr){
            if (p2->getTag() == p->getClosingTag()) {
                return p2;
            }
        }
    }

    return nullptr;
}


void Entity::check(){

}

const Comment* Entity::isInComment(qint64 pos){
    foreach(const QSharedPointer<Comment>& comment ,comments ){
        if (comment->commentStart<=pos&&comment->commentEnd>=pos) {
            return comment.data();
        }
    }

    return nullptr;
}

const Comment* Entity::isSpecial(qint64 pos){
    const Comment* p1 = isInComment(pos);
    const Comment* p2 = isInComment(pos+1);

    return (p1!=nullptr && p2!=p1)?p1:nullptr;
}

bool Entity::isInOuput(qint64 pos){
    static bool remove=false;
    const Comment* p = isInComment(pos);
    if (p!=nullptr){
        if (p->typeComment==REMOVE_START){
            remove=true;
        } else  if (p->typeComment==REMOVE_END){
            remove=false;
        }

        return false;
    } else {
        return !remove;

    }
}

bool Entity::executeComment(){

    QFile fil(fileInfo.absoluteFilePath());
    if (!fil.open(QIODevice::ReadOnly | QIODevice::Text))
        return -1;

    QTextStream file_utf8(&fil);
    file_utf8.setAutoDetectUnicode(true);

    Comment dummyStartTag(DUMMY,-1,-1);
    Comment dummyEndTag(DUMMY,fil.size(),fil.size());

    qDebug() << fil.size();

    QBuffer mem;
    mem.open(QBuffer::WriteOnly|QBuffer::Text);
    handleTag(&dummyStartTag, &dummyEndTag, file_utf8,mem );
    mem.close();

    check();

    QFileInfo outputfileInfo(outputSite,fileInfo.baseName()+".html");

    QFile output(outputfileInfo.absoluteFilePath());

    if (!output.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return -1;

    outputStringList(output,static_header_1);

    if (!embedded_styles.isEmpty()){
        output.write(QString("<style>").toUtf8());
        outputStringList(output,embedded_styles);
        output.write(QString("</style>").toUtf8());
    }

    outputStringList(output,static_header_2);

    output.write(mem.buffer());

    generatePageFooter(output);

    output.close();

}

void Entity::parse(){
    extractCommentPosition();
    executeComment();
}

void Entity::createComment( QByteArray& qb, qint64 commentStart, qint64 commentEnd){
    if (!inRemove && qb.startsWith(RMS_TAG)){
        inRemove=true;
        comments.append(QSharedPointer<Comment>(new Comment(REMOVE_START,commentStart,commentEnd)));
    }else if (inRemove && qb.startsWith(RME_TAG)){
        inRemove=false;
        comments.append(QSharedPointer<Comment>(new Comment(REMOVE_END,commentStart,commentEnd)));
    }else if (qb.startsWith(REF_TAG_START)){
        comments.append(QSharedPointer<Comment>(new RefStartComment(commentStart,commentEnd,qb)));
    }else if (qb.startsWith(STYLE_TAG)){
        comments.append(QSharedPointer<Comment>(new StyleComment(commentStart,commentEnd,qb)));
    }else if (qb.startsWith(REF_TAG_END)){
        comments.append(QSharedPointer<Comment>(new RefEndComment(commentStart,commentEnd,qb)));
    } else if (qb.startsWith(ICON_TAG)){
        comments.append(QSharedPointer<Comment>(new IconComment(commentStart,commentEnd,qb)));
    } else if (qb.startsWith(DOCUMENT_IMAGE)){
        comments.append(QSharedPointer<Comment>(new ImageComment(commentStart,commentEnd,qb)));
    } else if (qb.startsWith(SCREEN_TAG)){
        comments.append(QSharedPointer<Comment>(new ScreenComment(commentStart,commentEnd,qb)));
    } else if (qb.startsWith(BOOK_TAG)){
        comments.append(QSharedPointer<Comment>(new BookComment(commentStart,commentEnd,qb)));
    }else if (qb.startsWith(EYOU_TAG)){
        comments.append(QSharedPointer<Comment>(new EYouComment(commentStart,commentEnd,qb)));
    }
}

void Entity::extractCommentPosition(){

    QFile fil(fileInfo.absoluteFilePath());
    if (!fil.open(QIODevice::ReadOnly | QIODevice::Text))
        return ;

    QTextStream file_utf8(&fil);
    file_utf8.setAutoDetectUnicode(true);


    QChar c1='\0',c2='\0',c3='\0',c4='\0';
    bool inComment=false;
    QByteArray qb;
    qint64 commentStart;
    qint64 commentEnd;

    while (!file_utf8.atEnd()) {
        c4=c3;
        c3=c2;
        c2=c1;
        file_utf8 >> c1;
        if (!inComment && (c4=='<')&& (c3=='!')&&(c2=='-')&&(c1=='-')){
            inComment=true;
            qb.clear();
            qb.append("<!--");
            commentStart=file_utf8.pos()-4;
        } else if (inComment && (c3=='-')&& (c2=='-')&&(c1=='>')){
            inComment=false;
            commentEnd=file_utf8.pos()-1;
            qb.append(">");
            createComment(qb,commentStart,commentEnd);
        }
        else {
            if (inComment){
                qb.append(c1);
            }
        }
    }

    qDebug() << file_utf8.pos();
}


void Entity::handleTag( const Comment* openingTag, const Comment* closingTag, QTextStream& file, QIODevice& output) {

    file.seek(openingTag->getCommentEnd()+1);

    while (!file.atEnd()) {
        qint64 pos=file.pos();
        if (pos>=closingTag->getCommentStart()){
            file.seek(closingTag->getCommentEnd()+1);
            break;
        }
        QChar c1;
        file >> c1;
        const Comment* p= isSpecial(pos);

        if (p!=nullptr){
            if (p->isAutoClosing()){
                //     output.write(p->comment);
                //    output.write("<!--REMOVESTART-->");
                p->output(this,output);
                //    output.write("<!--REMOVEEND-->");
            } else {
                const Comment* closingTag=findClosingTag(p,file);
                if (closingTag != nullptr){
                    QBuffer buf;
                    buf.open(QBuffer::WriteOnly|QBuffer::Text);
                    handleTag(p,closingTag,file,buf);
                    buf.close();

                    if (false /*p->getTag()==STYLE_START*/){
                        //embedded_styles.append(buf.buffer());
                    } else {
                        output.write(buf.buffer());
                    }
                }
            }
        } else if (isInOuput(pos)){

            if (openingTag->isHTML() && c1=='\n'){
                output.write(QString("<BR/>").toUtf8());
            } else {
                if (!c1.isNonCharacter()) output.write(QString(c1).toUtf8());
            }
        }
    }

    return ;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    siteDir.setFilter(QDir::Files);
    siteDir.setNameFilters( QStringList() << "*.txt");
    QFileInfoList list = siteDir.entryInfoList();

    QFileInfoList::Iterator it= list.begin();

    while (it != list.end()) {
        QFileInfo fileInfo = *it++;

        qDebug() << fileInfo.absoluteFilePath();

        Entity entity(fileInfo);
        entity.parse();
    }

    qDebug() << "fini";
}
