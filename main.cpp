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

QDir outputSite("/home/teddy/Site");

const QByteArray REF_TAG_START="<!--REF";
const QByteArray REF_TAG_END="<!--/REF";
const QByteArray ICON_TAG="<!--ICON";
const QByteArray GAMECOVERS_TAG="<!--GAMECOVERS";
const QByteArray DOCUMENT_IMAGE="<!--DOCUMENT_IMAGE";
const QByteArray RMS_TAG="<!--REMOVESTART";
const QByteArray RME_TAG="<!--REMOVEEND";
const QByteArray STYLE_TAG="<!--EMBSTYLE";
const QByteArray SCREEN_TAG="<!--SCREEN";
const QByteArray BOOK_TAG="<!--BOOK";

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
    BOOK
};

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

    virtual void output(QIODevice& file) const{
        /*  if (typeComment==REFERENCE){
            file.write(comment);
            file.write("<!--REMOVESTART-->");
            file.write("<div>testTTT</div>");
            file.write("<!--REMOVEEND-->");
        }*/
    }
};



class HtmlSet {
public:
    class Html {
    private:
        QString _filePath;

    public:
        QString filePath(){
            return _filePath;
        }

        Html(const QString& filePath){
            _filePath=filePath;
        }
    };

private:
    QList<Html> _htmls;

public:
    bool readXmlFile(QXmlStreamReader &xmlReader) {

        Q_ASSERT(xmlReader.isStartElement() && xmlReader.name() == "htmlset");

        while (xmlReader.readNextStartElement()) {
            if (xmlReader.name()=="html"){
                _htmls.append(Html(xmlReader.readElementText()));
            } else {
                return false;
            }
        }


        if (xmlReader.hasError()) {
            return false;
        }

        return true;
    }

    QList<Html> htmls(){
        return _htmls;
    }
};

class PageSet {
public:
    class Page {
    private:
        QString _absoluteFilePath;

    public:
        QString absoluteFilePath(){
            return _absoluteFilePath;
        }

        Page(const QString& absoluteFilePath){
            _absoluteFilePath=absoluteFilePath;
        }
    };

private:
    QList<Page> pages;

public:
    bool readXmlFile(QXmlStreamReader &xmlReader) {

        Q_ASSERT(xmlReader.isStartElement() && xmlReader.name() == "pageset");

        while (xmlReader.readNextStartElement()) {
            if (xmlReader.name()=="page"){
                pages.append(Page(xmlReader.readElementText()));
            } else {
                return false;
            }
        }


        if (xmlReader.hasError()) {
            qWarning() << xmlReader.errorString();
            return false;
        }

        return true;
    }

};


class Image {
private:
    QString _absoluteFilePath;

public:
    QString absoluteFilePath(){
        return _absoluteFilePath;
    }

    Image(){
    }

    bool readXmlFile(QXmlStreamReader &xmlReader) {

        Q_ASSERT(xmlReader.isStartElement() && xmlReader.name() == "image");

        QString filePath=xmlReader.readElementText();
        if (xmlReader.hasError()){
            return false;
        }

        _absoluteFilePath=filePath;

        return true;
    }
};


class Game {

    QString _title;
    QString _year;
    QString _publisher;

public :

    class Intruction {
    public:
        enum TYPE { TYPE_BEGIN, ORIGINAL_FR=TYPE_BEGIN, ORIGINAL_EN, UNKNOWN, TYPE_END};
    private:
        inline const char* ToString(TYPE v)
        {
            switch (v)
            {
            case ORIGINAL_FR:   return "OFR";
            case ORIGINAL_EN:   return "OEN";
            case UNKNOWN:       return "UNKNOWN";
            default:            return "";
            }
        }

        PageSet pages;

    public :
        TYPE type;

        bool writeXmlFile(QXmlStreamWriter &xmlWriter) {
            xmlWriter.writeStartElement("instruction");

            xmlWriter.writeTextElement("type", ToString(type));

            xmlWriter.writeEndElement();

            return true;
        }

        bool readXmlFile(QXmlStreamReader &xmlReader) {

            Q_ASSERT(xmlReader.isStartElement() && xmlReader.name() == "instruction");

            while (xmlReader.readNextStartElement()) {
                if (xmlReader.name()=="type"){
                    QString typeCode=xmlReader.readElementText();
                    if (xmlReader.hasError()){
                        return false;
                    }

                    bool typeOk=false;
                    for (size_t typeIdx = TYPE_BEGIN; typeIdx != TYPE_END && !typeOk; ++typeIdx)
                    {
                        TYPE currentType=static_cast<TYPE>(typeIdx);
                        if (typeCode==ToString(currentType)){
                            this->type=currentType;
                            typeOk=true;
                        }
                    }

                    if (!typeOk){
                        return false;
                    }
                } else if (xmlReader.name()=="pageset"){
                    PageSet pageSet;
                    if (!pageSet.readXmlFile(xmlReader)){
                        return false;
                    }

                    pages=pageSet;
                }
            }


            if (xmlReader.hasError()) {
                qWarning() << xmlReader.errorString();
                return false;
            }

            return true;
        }
    };

    void setTitle(const QString& title){
        this->_title=title;
    }

    void setYear(const QString& year){
        this->_year=year;
    }


    void setPublisher(const QString& publisher){
        this->_publisher=publisher;
    }


    const QString title() const{
        return _title;
    }

    const QString publisher() const {
        return _publisher;
    }

    const QString year() const {
        return _year;
    }

private:
    QList<Image> _covers;
    QList<Intruction> instructions;
    HtmlSet _htmls;
public:

    void addIntruction(Intruction::TYPE type){
        Intruction t;
        t.type=type;
        instructions.append(t);
    }

    bool writeXmlFile(QIODevice &device) {
        QXmlStreamWriter xmlWriter(&device);

        xmlWriter.setAutoFormatting(true);
        xmlWriter.writeStartDocument();
        xmlWriter.writeStartElement("game");
        xmlWriter.writeTextElement("title", title());
        xmlWriter.writeTextElement("publisher", publisher());
        xmlWriter.writeTextElement("year", year());

        xmlWriter.writeStartElement("instructions");
        foreach (Intruction intruction, instructions) {
            intruction.writeXmlFile(xmlWriter);
        }
        xmlWriter.writeEndElement();

        xmlWriter.writeEndElement();
        xmlWriter.writeEndDocument();

        return true;
    }


    bool readXmlFile(QXmlStreamReader &xmlReader) {
        Q_ASSERT(xmlReader.isStartElement() && xmlReader.name() == "game");

        while (xmlReader.readNextStartElement()) {
            if (xmlReader.name()=="instructions"){
                while (xmlReader.readNextStartElement()) {
                    if (xmlReader.name()=="instruction"){
                        Intruction intruction;
                        if (!intruction.readXmlFile(xmlReader)){
                            return false;
                        }
                        instructions.append(intruction);
                    } else {
                        return false;
                    }

                }
            } else if (xmlReader.name()=="covers"){
                while (xmlReader.readNextStartElement()) {
                    if (xmlReader.name()=="image"){
                        Image image;
                        if (!image.readXmlFile(xmlReader)){
                            return false;
                        }
                        _covers.append(image);
                    } else {
                        return false;
                    }
                }
            } else if (xmlReader.name()=="htmlset"){
                {
                    HtmlSet htmlSet;
                    if (!htmlSet.readXmlFile(xmlReader)){
                        return false;
                    }

                    _htmls=htmlSet;
                }
            }
            else {
                xmlReader.skipCurrentElement();
            }
        }


        if (xmlReader.hasError()) {
            qWarning() << xmlReader.errorString();
            return false;
        }

        return true;
    }

    QList<HtmlSet::Html> htmls(){
        return _htmls.htmls();
    }

    QList<Image> covers(){
        return _covers;
    }

    bool readXmlFile(QIODevice &device) {
        QXmlStreamReader reader(&device);
        if  (!reader.readNextStartElement()){
            return false;
        }
        return readXmlFile(reader);
    }

};

class Book {
    QString _title;
    QString _year;
    QString _author;
    QString _publisher;
    QString _isbn;
    QString _cover;

public:
    Book(){

    }

    void setTitle(const QString& title){
        this->_title=title;
    }

    void setYear(const QString& year){
        this->_year=year;
    }

    void setAuthor(const QString& author){
        this->_author=author;
    }

    void setPublisher(const QString& publisher){
        this->_publisher=publisher;
    }

    void setIsbn(const QString& isbn){
        this->_isbn=isbn;
    }

    void setCover(const QString& cover){
        this->_cover=cover;
    }

    const QString title() const{
        return _title;
    }

    const QString isbn() const{
        return _isbn;
    }

    const QString cover() const {
        return _cover;
    }

    const QString publisher() const {
        return _publisher;
    }

    const QString year() const {
        return _year;
    }

    bool readXmlFile(QIODevice &device) {
        QXmlStreamReader xmlReader(&device);

        QStringList fields;
        while (!xmlReader.atEnd() && !xmlReader.hasError()) {
            xmlReader.readNext();
            if (xmlReader.isStartElement()){
                fields.append(xmlReader.name().toString());
            } else if (xmlReader.isEndElement()){
                if(!fields.isEmpty()) fields.removeLast();
            } else if (xmlReader.isCharacters() && !xmlReader.isWhitespace()) {
                if(!fields.isEmpty()){
                    if (fields.last()=="title"){
                        setTitle(xmlReader.text().toString());
                    } else if (fields.last()=="isbn"){
                        setIsbn(xmlReader.text().toString());
                    } else if (fields.last()=="publisher"){
                        setPublisher(xmlReader.text().toString());
                    } else if (fields.last()=="year"){
                        setYear(xmlReader.text().toString());
                    }
                }
            }
        }


        if (xmlReader.hasError()) {
            qWarning() << xmlReader.errorString();
            return false;
        }

        return true;
    }

    bool writeXmlFile(QIODevice &device) {
        QXmlStreamWriter xmlWriter(&device);

        xmlWriter.setAutoFormatting(true);
        xmlWriter.writeStartDocument();
        xmlWriter.writeStartElement("Book");
        xmlWriter.writeTextElement("title", title());
        xmlWriter.writeTextElement("isbn", isbn());
        xmlWriter.writeTextElement("publisher", publisher());
        xmlWriter.writeTextElement("year", year());

        xmlWriter.writeEndElement();
        xmlWriter.writeEndDocument();

        return true;
    }
};

QList<QSharedPointer<Book> > books;


void initBookList(){
    Book* result = new Book;
    result->setTitle("L'assembleur de l'Atari");
    result->setAuthor("Daniel-Jean David");
    result->setYear("1985");
    result->setPublisher("PSI");
    result->setIsbn("9782865952014");
    result->setCover("lassembleurdelatari.png");
    books.append(QSharedPointer<Book>(result));

    result = new Book;
    result->setTitle("Les périphériques de l'Atari");
    result->setAuthor("Daniel-Jean David");
    result->setYear("1985");
    result->setPublisher("PSI");
    result->setIsbn("9782865951987");
    result->setCover("Daniel_Jean_DAVID._Périphériques_de_l'Atari._PSI.png");
    books.append(QSharedPointer<Book>(result));

    result = new Book;
    result->setTitle("La Découverte de L'Atari 400,800,600xl,800xl");
    result->setAuthor("Daniel-Jean David");
    result->setYear("1985");
    result->setPublisher("PSI");
    result->setIsbn("9782865950812");
    result->setCover("La Découverte de L'Atari 400,800,600xl,800xl.png");
    books.append(QSharedPointer<Book>(result));
}

const Book* getBookByRef(const QString& ref){

    foreach(const QSharedPointer<Book>& book ,books ){
        if (ref.compare(book->isbn())==0) {
            return book.data();
        }
    }

    return nullptr;
}



QString readTemplate(const QString& template_file_name){
   // ://templates/numero.js

    QFile fil(template_file_name);
    if (!fil.open(QIODevice::ReadOnly | QIODevice::Text))
        return "";

    QTextStream file_utf8(&fil);
    return file_utf8.readAll();

}

QList<QSharedPointer<Comment> > comments;
QStringList embedded_styles;

void clear_vars(){
    comments.clear();
    embedded_styles.clear();
}

class IconComment : public Comment {
public:
    IconComment(qint64 commentStart, qint64 commentEnd, const QByteArray& comment=QByteArray()):
        Comment(ICON,commentStart,commentEnd,comment){
    }

    virtual void output(QIODevice& file) const{

        QRegularExpression re("<!--ICON bookRef=(?<bookRef>\\w+) -->");
        QRegularExpressionMatch match=re.match(comment);

        if(match.hasMatch())
        {
            QString bookRef=match.captured("bookRef");

            const Book* b=getBookByRef(bookRef.trimmed());
            if (b != nullptr){
                if( !b->cover().isEmpty()){
                    QImage  p;
                    if (p.load("C:\\perso\\site\\srcimages\\"+b->cover())){
                        QImage  p2=p.scaled(QSize(100,100),Qt::KeepAspectRatio,Qt::SmoothTransformation);
                        p2.save("C:\\perso\\site\\images\\"+b->isbn()+".png");
                        QString tag=QString("<img src=\"images\\"+b->isbn()+".png\" class=\"shadow\" title=\"")+b->title()+"\">";

                        file.write(tag.toUtf8());
                    }
                }
            } else {

            }
        }
    }
};

class GameCoversComment : public Comment {
public:
    GameCoversComment(qint64 commentStart, qint64 commentEnd, const QByteArray& comment=QByteArray()):
        Comment(GAMECOVERS,commentStart,commentEnd,comment){
    }

    virtual void output(QIODevice& file) const{
        QString tag=QString("<img src=\"images.png\" class=\"shadow\" title=\"\">");
        file.write(tag.toUtf8());
    }
};

class RefStartComment : public Comment {
public:
    RefStartComment(qint64 commentStart, qint64 commentEnd, const QByteArray& comment=QByteArray()):
        Comment(REFERENCE_START,commentStart,commentEnd,comment,REFERENCE_END){
    }

    virtual void output(QIODevice& file) const{

    }
};

class RefEndComment : public Comment {
public:
    RefEndComment(qint64 commentStart, qint64 commentEnd, const QByteArray& comment=QByteArray()):
        Comment(REFERENCE_END,commentStart,commentEnd,comment){
    }

    virtual void output(QIODevice& file) const{

    }
};

class StyleComment : public Comment {
public:
    StyleComment(qint64 commentStart, qint64 commentEnd, const QByteArray& comment=QByteArray()):
        Comment(STYLE,commentStart,commentEnd,comment){
    }

    virtual void output(QIODevice& ) const{
           embedded_styles.append(getCommentInternalText(STYLE_TAG));
    }
};

class ScreenComment : public Comment {
public:
    ScreenComment(qint64 commentStart, qint64 commentEnd, const QByteArray& comment=QByteArray()):
        Comment(SCREEN,commentStart,commentEnd,comment){
    }

    virtual void output(QIODevice& iodevice) const{
           QString imageUrl=this->getCommentInternalText(SCREEN_TAG).trimmed();

           if (!imageUrl.isEmpty()){
             iodevice.write(QString("<div class=\"atari-screen\" style=\"background: url(\'").append(imageUrl).append("'\);\"></div>").toUtf8());
           }
    }
};

class BookComment : public Comment {
public:
    BookComment(qint64 commentStart, qint64 commentEnd, const QByteArray& comment=QByteArray()):
        Comment(BOOK,commentStart,commentEnd,comment){
    }

    virtual void output(QIODevice& iodevice) const{

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

const Comment* isInComment(qint64 pos){
    foreach(const QSharedPointer<Comment>& comment ,comments ){
        if (comment->commentStart<=pos&&comment->commentEnd>=pos) {
            return comment.data();
        }
    }

    return nullptr;
}

const Comment* isSpecial(qint64 pos){
    const Comment* p1 = isInComment(pos);
    const Comment* p2 = isInComment(pos+1);

    return (p1!=nullptr && p2!=p1)?p1:nullptr;
}

bool isInOuput(qint64 pos){
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
void handleComment( QByteArray& qb,qint64 commentStart,qint64 commentEnd){
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
    } else if (qb.startsWith(GAMECOVERS_TAG)){
        comments.append(QSharedPointer<Comment>(new GameCoversComment(commentStart,commentEnd,qb)));
    } else if (qb.startsWith(SCREEN_TAG)){
        comments.append(QSharedPointer<Comment>(new ScreenComment(commentStart,commentEnd,qb)));
    } else if (qb.startsWith(BOOK_TAG)){
        comments.append(QSharedPointer<Comment>(new BookComment(commentStart,commentEnd,qb)));
    }

}



const Comment* findClosingTag(const Comment* p, QTextStream& file ){
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



void handleTag( const Comment* openingTag, const Comment* closingTag, QTextStream& file, QIODevice& output) {

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
                p->output(output);
            //    output.write("<!--REMOVEEND-->");
            } else {
                const Comment* closingTag=findClosingTag(p,file);
                if (closingTag != nullptr){
                    QBuffer buf;
                    buf.open(QBuffer::WriteOnly|QBuffer::Text);
                    handleTag(p,closingTag,file,buf);
                    buf.close();

                    if (false /*p->getTag()==STYLE_START*/){
                        embedded_styles.append(buf.buffer());
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

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    ;
    /*
QFile input1("c:\\perso\\site\\atari.xml");
if (!input1.open(QIODevice::ReadOnly | QIODevice::Text ))
    return -1;

Game b;
qDebug() << b.readXmlFile(input1);

foreach (HtmlSet::Html html, b.htmls()) {
    qDebug() << html.filePath();
*/

    initBookList();

    //QString output=QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);


    QDir siteDir("/home/teddy/GDrive/site");

    siteDir.setFilter(QDir::Files);
    siteDir.setNameFilters( QStringList() << "*.txt");
    QFileInfoList list = siteDir.entryInfoList();

    QFileInfoList::Iterator it= list.begin();


    while (it != list.end()) {
        QFileInfo fileInfo = *it++;

        qDebug() << fileInfo.absoluteFilePath();




        QFile fil(fileInfo.absoluteFilePath());
        if (!fil.open(QIODevice::ReadOnly | QIODevice::Text))
            return -1;

        QTextStream file_utf8(&fil);
        file_utf8.setAutoDetectUnicode(true);


        QFileInfo outputfileInfo(outputSite,fileInfo.baseName()+".html");

        QFile output(outputfileInfo.absoluteFilePath());

        if (!output.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
            return -1;

        clear_vars();



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
                handleComment(qb,commentStart,commentEnd);
            }
            else {
                if (inComment){
                    qb.append(c1);
                }
            }
        }

        Comment dummyStartTag(DUMMY,-1,-1);
        Comment dummyEndTag(DUMMY,file_utf8.pos(),file_utf8.pos());

        QBuffer mem;
        mem.open(QBuffer::WriteOnly|QBuffer::Text);
        handleTag(&dummyStartTag, &dummyEndTag, file_utf8,mem );
        mem.close();

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

    qDebug() << "fini";
}
