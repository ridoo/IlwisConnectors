#include <QString>
#include <QHash>
#include <QUrl>
#include <QLibrary>
#include <QStringList>
#include "kernel.h"
#include "ilwisdata.h"
#include "domain.h"
#include "connectorinterface.h"
#include "mastercatalog.h"
#include "ilwisobjectconnector.h"
#include "catalogexplorer.h"
#include "catalogconnector.h"
#include "resource.h"
#include "gdalproxy.h"
#include "gdalitem.h"
#include "proj4parameters.h"
#include "mastercatalog.h"
#include "size.h"

using namespace Ilwis;
using namespace Gdal;

GDALItems::GDALItems(const QUrl &url, const QFileInfo &localFile, IlwisTypes extTypes)
{
    if ( localFile.isRoot())
        return ;
    QFileInfo file = localFile;
    GdalHandle* handle = gdal()->openFile(file.absoluteFilePath(), i64UNDEF , GA_ReadOnly, false);
    if (handle){
        quint64 sz = file.size();
        int count = 0;
        if (handle->type() == GdalHandle::etGDALDatasetH){
            count = gdal()->layerCount(handle->handle());
        }else if(handle->type() == GdalHandle::etOGRDataSourceH){
            count = gdal()->getLayerCount(handle->handle());
        }
        if ( count == 0) {// could be a complex dataset
            char **pdatasets = gdal()->getMetaData(handle->handle(), "SUBDATASETS");
            if ( pdatasets != 0){
                auto datasets = kvp2Map(pdatasets);
                handleComplexDataSet(datasets);
            }
            return;
        }
        //TODO: at the moment simplistic approach; all is corners georef and domain value
        // and a homogenous type if files. when we have example of more complex nature we wille xtend this+
        quint64 csyId = addCsy(handle, file.absoluteFilePath(), url, false);
        if ( handle->type() == GdalHandle::etGDALDatasetH) {
            quint64 grfId = addItem(handle, url, csyId, 0, itGEOREF,itCOORDSYSTEM);
            addItem(handle, url, csyId, grfId, itRASTER,itGEOREF | itNUMERICDOMAIN | itCONVENTIONALCOORDSYSTEM,sz);
        } else{
            if ( count == 1) {//  default case, one layer per object
                OGRLayerH layerH = gdal()->getLayer(handle->handle(),0);
                int featureCount = gdal()->getFeatureCount(layerH, FALSE);
                sz = findSize(file);
                addItem(handle, url, csyId, featureCount, itFEATURE , itCOORDSYSTEM | itTABLE, sz);
                addItem(handle, url, 0, iUNDEF, itTABLE , 0, sz);
                if (! mastercatalog()->id2Resource(csyId).isValid())
                    addItem(handle, QUrl(url), 0, iUNDEF, itCONVENTIONALCOORDSYSTEM , 0, sz);
            }
            else { // multiple layers, the file itself will be marked as container; internal layers will be added using this file as container
                //TODO: atm the assumption is that all gdal containers are files. this is true in the majority of the cases but not in all. Without a proper testcase the non-file option will not(yet) be implemented
                addItem(handle, url, count, iUNDEF, itCATALOG , extTypes | itFILE);
                for(int i = 0; i < count; ++i){
                    OGRLayerH layerH = gdal()->getLayer(handle->handle(),i);
                    if ( layerH){
                        const char *cname = gdal()->getLayerName(layerH);
                        int featureCount = gdal()->getFeatureCount(layerH, FALSE);
                        if ( cname){
                            QString layerName(cname);
                            QString layerurl = url.toString() + "/" + layerName;
                            addItem(handle, QUrl(layerurl), csyId, featureCount, itFEATURE , itCOORDSYSTEM | itTABLE, sz);
                            addItem(handle, QUrl(layerurl), 0, iUNDEF, itTABLE , 0, sz);
                            if (! mastercatalog()->id2Resource(csyId).isValid())
                                addItem(handle, QUrl(layerurl), 0, iUNDEF, itCONVENTIONALCOORDSYSTEM , 0, sz);
                        }

                    }

                }
            }
        }


        gdal()->closeFile(file.absoluteFilePath(), i64UNDEF);
    }
}

quint64 GDALItems::findSize(const QFileInfo& inf){
    quint64 size = inf.size();
    if ( inf.suffix().toLower() == "shp"){
        QString path = inf.absolutePath() + "/" + inf.baseName();
        size += QFileInfo(path + ".dbf").size();
        size += QFileInfo(path + ".sbn").size();
        size += QFileInfo(path + ".prj").size();
        size += QFileInfo(path + ".sbx").size();
        size += QFileInfo(path + ".shx").size();
    }
    return size;
}

void GDALItems::handleComplexDataSet(const std::map<QString, QString>& datasetdesc){
    auto iter = datasetdesc.begin();
    // we know there is altijd  pairs SUBDATASET_<n>_DESC,SUBDATASET_<n>_NAME in the map
    while(iter !=  datasetdesc.end()) {
        Size<> sz;
        QString shortname;
        QStringList parts = iter->second.split("//");
        QString sizeString = parts[0].mid(1,parts[0].size() - 3);
        bool ok1, ok2, ok3;

        QStringList szMembers = sizeString.split("x");
        if ( szMembers.size() == 2){
            int x = szMembers[0].toInt(&ok1);
            int y = szMembers[1].toInt(&ok2);
            if ( ok1 && ok2)
                sz = Size<>(x,y,1);
        } else if ( szMembers.size() == 3){
            int z = szMembers[0].toInt(&ok1);
            int x = szMembers[1].toInt(&ok2);
            int y = szMembers[2].toInt(&ok3);
            if ( ok1 && ok2 && ok3)
                sz = Size<>(x,y,z);
        }
        QStringList secondPart = parts[1].split("(");
        shortname = secondPart[0].trimmed();
        QString numbertype = secondPart[1].left(secondPart[1].size() - 1);
        quint64 domid = numbertype2domainid(numbertype);
        ++iter;
        QString encodedUrl = QUrl::toPercentEncoding(iter->second,"/","\"");
        QString rawUrl = "gdal://"+ encodedUrl;
        parts = iter->second.split("\"");
        QString normalizedUrl = "file:///" + parts[1] + "/" + shortname;
        Resource gdalitem(normalizedUrl,rawUrl,itRASTER);
        gdalitem.code(iter->second);
        gdalitem.name(shortname, false);
        gdalitem.dimensions(sz.toString());
        GDALDatasetH handle = gdal()->open(gdalitem.code().toLocal8Bit(), GA_ReadOnly);
        GdalHandle ihandle(handle,GdalHandle::etGDALDatasetH);
        quint64 csyid = addCsy(&ihandle,gdalitem.code(),normalizedUrl,false);
        if ( csyid != i64UNDEF){
            gdalitem.addProperty("coordinatesystem", csyid);
        }
        gdal()->close(handle);
        gdalitem.addProperty("domain",domid);
        insert(gdalitem);
        ++iter;

    }
}

quint64 GDALItems::numbertype2domainid(const QString& numbertype) const{
    QString systemdomain="value";
    if ( numbertype.indexOf("integer") != -1)
        systemdomain = "integer";
    if ( numbertype == "8-bit integer"){
        systemdomain = "image";
    } if ( numbertype == "16-bit unsigned integer"){
        systemdomain = "image16"    ;
    } else if ( numbertype == "32-bit unsigned integer"){
        systemdomain = "count";
    }
    IDomain dom(systemdomain);
    return dom->id();
}

QString GDALItems::dimensions(GdalHandle* handle, bool & is3d) const
{
    Size<> sz(gdal()->xsize(handle->handle()), gdal()->ysize(handle->handle()), gdal()->layerCount(handle->handle()));
    QString dim = QString("%1 %2").arg(sz.xsize()).arg(sz.ysize());
    if ( sz.zsize() != 1){
        dim += " " + QString::number(sz.zsize());
        is3d = true;
    }

    return dim;
}

quint64 GDALItems::addItem(GdalHandle* handle, const QUrl& url, quint64 csyid, quint64 grfId, IlwisTypes tp, IlwisTypes extTypes, quint64 sz) {
    Resource gdalItem(url, tp);
    if ( sz != i64UNDEF)
        gdalItem.size(sz);
    bool is3D = false;
    if ( !hasType(tp,itCATALOG))
        gdalItem.addProperty("coordinatesystem", csyid);
    if ( tp == itFEATURE){
        QString count = grfId == -1 ? "" : QString::number(grfId);
        gdalItem.dimensions(count);// misuse of grfid
    }
    else if ( tp == itRASTER){
        Resource resValue = mastercatalog()->name2Resource("code=value",itNUMERICDOMAIN);
        gdalItem.addProperty("domain", resValue.id());
        gdalItem.addProperty("georeference", grfId);

        QString dim = dimensions(handle, is3D);
        gdalItem.dimensions(dim);
        if ( is3D)
            extTypes |= itCATALOG;

    } else if (hasType(tp, itGEOREF)){
        gdalItem.dimensions(dimensions(handle, is3D));
    }else{
        if ( tp == itCATALOG){
            QString dim = QString::number(csyid); // misuse of csyid :)
            gdalItem.dimensions(dim);
        }
    }
    gdalItem.setExtendedType(extTypes);

    insert(gdalItem);

    return gdalItem.id();
}

quint64 GDALItems::addCsy(GdalHandle* handle, const QString &path, const QUrl& url, bool message) {
    quint64 ret = i64UNDEF;
    OGRSpatialReferenceH srshandle = gdal()->srsHandle(handle, path, message);
    if (srshandle != 0){
        const char * projcs_epsg = gdal()->getAuthorityCode(srshandle, "PROJCS");
        const char * geocs_epsg = gdal()->getAuthorityCode(srshandle, "GEOGCS");
        if (QString(gdal()->getAuthorityName(srshandle, "PROJCS")).compare("EPSG", Qt::CaseInsensitive) == 0 && projcs_epsg) {
            Resource resource = mastercatalog()->name2Resource(QString("code=epsg:%1").arg(projcs_epsg), itCONVENTIONALCOORDSYSTEM);
            if ( resource.isValid())
                ret = resource.id();
        }else if (QString(gdal()->getAuthorityName(srshandle, "GEOGCS")).compare("EPSG", Qt::CaseInsensitive) == 0 && geocs_epsg){
            Resource resource = mastercatalog()->name2Resource(QString("code=epsg:%1").arg(geocs_epsg), itCONVENTIONALCOORDSYSTEM);
            if ( resource.isValid())
                ret = resource.id();
        }else {
            char *proj4;
            gdal()->export2Proj4(srshandle, &proj4);
            QString sproj4 = proj4;
            if ( proj4){
                gdal()->free(proj4);

                Proj4Def def = Proj4Parameters::lookupDefintion(sproj4);
                if ( def._epsg != sUNDEF){
                    return mastercatalog()->name2id("code=" + def._epsg);
                }else {
                    Resource res("code=proj4:" + sproj4, itCONVENTIONALCOORDSYSTEM);
                    QFileInfo inf(path);
                    res.name(inf.fileName());
                    if ( inf.exists()){
                        res.setUrl(QUrl::fromLocalFile(path), true);
                        res.setUrl(QUrl::fromLocalFile(path));
                    }
                    mastercatalog()->addItems({res});
                    //Proj4Parameters::add2lookup(res.name(),sproj4,0);
                    return res.id();
                }
            }



        }
    }
    if ( srshandle)
        gdal()->releaseSrsHandle(handle, srshandle, path);

    if(ret == i64UNDEF){
        Resource resource("code=csy:unknown",itCOORDSYSTEM);
        Envelope env = gdal()->envelope(handle,0);
        if ( env.isValid() && !env.isNull()){
            QString dim = QString("%1 x %2 x %3 x %4").arg(env.min_corner().x).arg(env.min_corner().y).arg(env.max_corner().x).arg(env.max_corner().y);
            resource.dimensions(dim);

        }
        insert(resource);
        return resource.id();
    }else
        return ret;
}

std::map<QString, QString> GDALItems::kvp2Map(char **kvplist)
{
    std::map<QString, QString> result;
    if ( kvplist != 0){

        quint32 nItems=0;

        while(*(kvplist + nItems) != NULL)
        {
            nItems++;
            //kvplist++;
        }


        for(int i =0; i < nItems; ++i){
            QString item(kvplist[i]);
            QStringList kvp = item.split("=");
            if ( kvp.size() == 2)
                result[kvp[0]] = kvp[1];

        }
    }
    return result;
}




