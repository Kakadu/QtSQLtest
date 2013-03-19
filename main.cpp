#include <QCoreApplication>
#include <QSqlDatabase>
#include <QFile>
#include <QDebug>
#include <QStringList>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QThread>
#include <QList>
#include <iostream>
#include <iomanip>

using namespace std;

const QString DB_FILE_NAME = "my.db.sqlite";
const int BUF_SIZE = 10;

void prepareDB() {
    cout << "Prepare DB\n";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(DB_FILE_NAME);
    db.open();

    QSqlQuery query(db);
    query.exec("drop table files");
    query.exec("create table files (filename TEXT, word TEXT, count INTEGER)");
    db.close();
}

inline void addString(QHash<QString,int>& counts, QString s) {
    if (counts.contains(s))
        counts[s]++;
    else
        counts.insert(s, 1);
}

void doFile(const QString& filename) {
    qDebug() << "Main work with " << filename;

    QFile file(filename);
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    QString buf = "";
    QHash<QString,int> counts;

    while(!file.atEnd()) {
        buf += QString( file.read(BUF_SIZE) );
        if (buf.isEmpty())
            break;
        QStringList lst = buf.split(QRegExp("[\t ' ' '\n']"));
        int len = lst.length();
        QString newbuf = lst.last();
        if (!newbuf.isEmpty())
            len--;
        for (int i=0; i<len; ++i) {
            QString s = lst.at(i);
            if (!s.isEmpty())
                addString(counts, s);
        }
        buf = newbuf;
    }
    file.close();

    if (buf.length() != 0)
        addString(counts, buf);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(DB_FILE_NAME);
    db.open();

    QSqlQuery query(db);
    QString q = QString("DELETE FROM files WHERE filename='%1'").arg(filename);
    //TODO: think what happens if there is '\'' character in filename
    if (!query.exec(q)) {
        qDebug() << q;
        qDebug() << "last error = " << query.lastError();
    }

    if (counts.count() > 0) {
        QStringList querylist;

        QString pattern("(\"%1\",\"%2\",%3)");
        cout << "Counts:\n";
        foreach(const QString& word, counts.keys()) {
            int count = counts[word];
            cout << setw(20) << word.toStdString() << ": " << count << "\n";
            querylist << pattern.arg(filename).arg(word).arg(count);
        }
        QString insertQuery = "INSERT INTO files VALUES " + querylist.join(",");
        if (!query.exec(insertQuery) ) {
            qDebug() << query.lastError();
            qFatal("Can't insert values to database\n");
        }
    } else {
        cout << "No words found. Don't insert anything\n";
    }
    db.close();
}

void listDataBase() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(DB_FILE_NAME);
    db.open();
    QSqlQuery query(db);

    if (!query.exec("select * from files") ) {
        qDebug() << query.lastError();
        qFatal("Error while listing database");
    } else {
        int filename = query.record().indexOf("filename"),
            word = query.record().indexOf("word"),
            count = query.record().indexOf("count");
        while (query.next()) {
            cout << setw(10) << query.value(filename).toString().toStdString() << " "
                 << setw(5)  << query.value(count).toString().toStdString() << " "
                 << setw(20) << query.value(word).toString().toStdString() << endl;
        }
    }

    db.close();
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if (a.arguments().count() > 1) {
        QString s = a.arguments().at(1);
        if (s==QString("-init"))
            prepareDB();
        else if (s==QString("-list"))
            listDataBase();
        else
            doFile(s);
    } else {
        std::cout << "First argument should be -init, -list or filename\n";
    }

    return 0;
}
