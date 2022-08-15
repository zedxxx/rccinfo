#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "resinfo.h"

struct ResItemInfo
{
    QString name;
    ResItem info;
};

void callback(const QString &itemName, const ResItem &itemInfo, void *userData)
{
    if (userData) {
        QList<ResItemInfo> *list = reinterpret_cast<QList<ResItemInfo>*>(userData);
        list->append(ResItemInfo{itemName, itemInfo});
    }
}

const char* get_compressed_str(const int flag)
{
    if (flag & ResInfo::Flags::Compressed)
        return "zlib";

    if (flag & ResInfo::Flags::CompressedZstd)
        return "zstd";

    return nullptr;
}

QByteArray dump_to_json(const int formatVersion, const int flags, const QList<ResItemInfo> &items)
{
    QJsonObject obj;

    obj.insert("format_version", formatVersion);
    obj.insert("count", items.count());

    auto comp = get_compressed_str(flags);
    if (comp) {
        obj.insert("compressed", comp);
    }

    QJsonArray arr;
    for (const auto &item: items) {
        QJsonObject item_obj;

        item_obj.insert("name", item.name);
        item_obj.insert("size", item.info.size);

        auto comp = get_compressed_str(item.info.flags);
        if (comp) {
            item_obj.insert("compressed", comp);
        }

        QLocale loc(QLocale::Language(item.info.language), QLocale::Country(item.info.country));
        if ( (loc.country() != QLocale::AnyCountry) ||
             ((loc.language() != QLocale::AnyLanguage) && (loc.language() != QLocale::C)) )
        {
            item_obj.insert("lang", loc.bcp47Name());
        }

        if ((formatVersion >= 2) && (item.info.last_modified != 0)) {
            QDateTime lm = QDateTime::fromMSecsSinceEpoch(item.info.last_modified);
            item_obj.insert("last-modified", lm.toString("yyyy-MM-ddThh:mm:ss.zzzZ"));
        }

        arr.append(item_obj);
    }

    obj.insert("items", arr);

    QJsonDocument doc;
    doc.setObject(obj);

    return doc.toJson();
}

bool save_json_to_file(const QByteArray &json, const QString &fileName)
{
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream out(&file);
        out.setCodec("utf-8");
        out << json;
        file.close();

        return true;
    } else {
        qInfo() << "File open failed:" << fileName;
        return false;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        qInfo() << "Usage: rccinfo input.rcc [output.json]";
        return 1;
    }

    QString rccFileName(argv[1]);
    ResInfo info(rccFileName);

    QList<ResItemInfo> list;
    if ( info.read(&callback, &list) ) {

        qInfo() << "Found" << list.count() << "records";

        QByteArray json = dump_to_json(info.getFormatVersion(), info.getFlags(), list);

        QString jsonFileName;
        if (argc >= 3) {
            jsonFileName = argv[2];
        } else {
            jsonFileName = rccFileName + ".json";
        }

        if ( save_json_to_file(json, jsonFileName) ) {
            qInfo() << "Json saved to:" << jsonFileName;
        }

        return 0;
    }

    qInfo() << "Finished with error!";

    return 2;
}
