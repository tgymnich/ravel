#ifndef OTFIMPORTOPTIONS_H
#define OTFIMPORTOPTIONS_H

#include <QString>
#include <QList>

// Container for all the structure extraction options
class OTFImportOptions
{
public:
    OTFImportOptions(bool _waitall = true,
                     bool _leap = false, bool _skip = false,
                     bool _partition = false, QString _fxn = "");
    //OTFImportOptions(const OTFImportOptions& copy);
    //OTFImportOptions& operator=(const OTFImportOptions& copy);

    // Annoying stuff for cramming into OTF2 format
    QList<QString> getOptionNames();
    QString getOptionValue(QString option);
    void setOption(QString option, QString value);

    enum OriginFormat { OF_NONE, OF_SAVE_OTF2, OF_OTF2, OF_OTF, OF_CHARM };

    bool waitallMerge; // use waitall heuristic
    bool leapMerge; // merge to complete leaps
        bool leapSkip; // but skip if you can't gain processes
    bool partitionByFunction; // partitions based on functions
    bool globalMerge; // merge across steps
    bool cluster; // clustering on gnomes should be done
    bool isendCoalescing; // group consecutive isends
    bool enforceMessageSizes; // send/recv size must match
    OriginFormat origin;
    QString partitionFunction;

};

#endif // OTFIMPORTOPTIONS_H
