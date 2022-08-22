#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "resinfo.h"

const char* get_compressed_str(const int flag)
{
    if (flag & ResInfo::Flags::Compressed)
        return "zlib";

    if (flag & ResInfo::Flags::CompressedZstd)
        return "zstd";

    return nullptr;
}

QByteArray dump_to_json(const ResInfo &info)
{    
    const int count = info.getItemsCount();

    qInfo() << "Found" << count << "records";

    QJsonObject obj;

    obj.insert("format_version", info.getFormatVersion());
    obj.insert("count", count);

    auto comp = get_compressed_str(info.getFlags());
    if (comp)
        obj.insert("compressed", comp);

    QJsonArray arr;
    int item_id = 0;

    const QList<QString> list = info.getItemNames();
    for (const auto &itemName: list) {

        const QList<ResItem> items = info.getInfo(itemName);
        for (const auto &item : items) {

            QJsonObject item_obj;

            item_obj.insert("id", item_id);
            item_id++;

            item_obj.insert("name", itemName);
            item_obj.insert("size", item.size);
            item_obj.insert("offset", item.offset);

            auto comp = get_compressed_str(item.flags);
            if (comp)
                item_obj.insert("compressed", comp);

            QLocale loc(QLocale::Language(item.language),
                        QLocale::Country(item.country));

            bool isDefaultLocale = (loc.country() == QLocale::AnyCountry) &&
                                   (loc.language() == QLocale::AnyLanguage || loc.language() == QLocale::C);

            if (!isDefaultLocale)
                item_obj.insert("lang", loc.bcp47Name());

            if ((info.getFormatVersion() >= 2) && (item.last_modified != 0)) {
                QDateTime lm = QDateTime::fromMSecsSinceEpoch(item.last_modified);
                item_obj.insert("last-modified", lm.toString("yyyy-MM-ddThh:mm:ss.zzzZ"));
            }

            arr.append(item_obj);
        }
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

    const QString rccFileName(argv[1]);

    ResInfo info;
    if (!info.read(rccFileName))
        return 2;

    const QByteArray json = dump_to_json(info);

    const QString jsonFileName = (argc >= 3) ? argv[2] : rccFileName + ".json";

    if (!save_json_to_file(json, jsonFileName))
        return 3;

    qInfo() << "Json saved to:" << jsonFileName;
    return 0;
}
