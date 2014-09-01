#include <QBuffer>
#include "kernel.h"
#include "version.h"
#include "ilwisdata.h"
#include "connectorinterface.h"
#include "mastercatalog.h"
#include "ilwisobjectconnector.h"
#include "catalogexplorer.h"
#include "catalogconnector.h"
#include "streamconnector.h"
#include "versionedserializer.h"
#include "domain.h"
#include "datadefinition.h"
#include "columndefinition.h"
#include "table.h"
#include "coverage.h"
#include "feature.h"
#include "featurecoverage.h"
#include "raster.h"
#include "factory.h"
#include "abstractfactory.h"
#include "downloadmanager.h"
#include "versioneddatastreamfactory.h"


using namespace Ilwis;
using namespace Stream;

ConnectorInterface *StreamConnector::create(const Resource &resource, bool load, const IOOptions &options) {
    return new StreamConnector(resource, load, options);

}

IlwisObject *StreamConnector::create() const
{
    switch(_resource.ilwisType()){
    case itFEATURE:
        return new FeatureCoverage(_resource);
    case itRASTER:
        return new RasterCoverage(_resource);
    default:
        return 0;
    }
}


StreamConnector::StreamConnector(const Ilwis::Resource &resource, bool load, const IOOptions &options) : IlwisObjectConnector(resource,load,options)
{
    QString url =  resource.url().toString();
    url.replace(0,7,"http:");
    _resource.code("serialized");

    _resource.setUrl(url, true);
}

StreamConnector::~StreamConnector()
{

}

bool StreamConnector::loadMetaData(IlwisObject *object, const IOOptions &options)
{
    DownloadManager manager(_resource);
    return manager.loadMetaData(object,options);


}

bool StreamConnector::loadData(IlwisObject *data, const IOOptions &options){
    if (!openSource(true))
        return false;
    QDataStream stream(_datasource.get());
    quint64 type;
    stream >> type;
    QString version;
    stream >> version;


    VersionedDataStreamFactory *factory = kernel()->factory<VersionedDataStreamFactory>("ilwis::VersionedDataStreamFactory");
    if (factory){
        _versionedConnector.reset( factory->create(version,type,stream));
    }

    if (!_versionedConnector)
        return false;

    bool ok =  _versionedConnector->loadData(data, options);

    _datasource->close();
    _versionedConnector.reset(0);

    return ok;
}

bool StreamConnector::openSource(bool reading){
    QUrl url = _resource.url(true);
    QString scheme = url.scheme();
    if ( _resource.code() == "serialized" || scheme == "remote"){
        _bytes.resize(STREAMBLOCKSIZE);
        _bytes.fill(0);
        QBuffer *buf = new QBuffer(&_bytes);
        if (!buf->open(QIODevice::ReadWrite))
            return false;
        _datasource.reset(buf);
        return true;
    }
    else if ( scheme == "file"){
        QString filename = url.toLocalFile();
        QFile *file = new QFile(filename);

        if ( reading){
            if ( !file->exists()){
                delete file;
                return ERROR1(ERR_MISSING_DATA_FILE_1,file->fileName());
            }
        }
        if (file->open(reading  ? QIODevice::ReadOnly : QIODevice::ReadWrite)) {
            _datasource.reset(file);
            return true;
        }
    }
    return false;
}


bool StreamConnector::store(IlwisObject *obj, int options){
    if (!openSource(false))
        return false;
    QDataStream stream(_datasource.get());
    //stream << Version::IlwisVersion;

    const VersionedDataStreamFactory *factory = kernel()->factory<VersionedDataStreamFactory>("ilwis::VersionedDataStreamFactory");
    if (factory){
        _versionedConnector.reset( factory->create(Version::IlwisVersion,_resource.ilwisType(),stream));
    }

    if (!_versionedConnector)
        return false;
    _versionedConnector->connector(this);
    bool ok = _versionedConnector->store(obj, options);

    flush(true);

    _datasource->close();
    _versionedConnector.reset(0);

    return ok;

}

QString StreamConnector::provider() const
{
    return "Stream";
}

bool StreamConnector::needFlush() const
{
    return _datasource->pos() > STREAMBLOCKSIZE && _resource.url().scheme() != "file";
}

void StreamConnector::flush(bool last)
{
    if ( _resource.url().scheme() == "file") // we dont flush with files; OS does this
        return;
    QBuffer *buf = static_cast<QBuffer *>(_datasource.get());
    emit dataAvailable(buf,true);

}
