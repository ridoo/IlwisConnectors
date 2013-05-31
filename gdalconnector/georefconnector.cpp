#include <QString>
#include <QUrl>
#include <QFileInfo>
#include <QDir>

#include "kernel.h"
#include "geometries.h"
#include "ilwisdata.h"
#include "domain.h"
#include "connectorinterface.h"
#include "ilwisobjectconnector.h"
#include "gdalproxy.h"
#include "gdalconnector.h"
#include "coordinatesystem.h"
#include "georeference.h"
#include "boostext.h"
#include "simpelgeoreference.h"
#include "cornersgeoreference.h"
#include "georefconnector.h"


using namespace Ilwis;
using namespace Gdal;

ConnectorInterface *GeorefConnector::create(const Ilwis::Resource &item,bool load) {
    return new GeorefConnector(item,load);
}

GeorefConnector::GeorefConnector(const Ilwis::Resource &item, bool load) : GdalConnector(item, load)
{
}

bool GeorefConnector::loadMetaData(IlwisObject *data){
    if(!GdalConnector::loadMetaData(data))
        return false;

    //TODO tiepoints grf

    CornersGeoReference * grf = static_cast<CornersGeoReference *>(data);

    ICoordinateSystem csy = setObject<ICoordinateSystem>("coordinatesystem", _filename);
    if(!csy.isValid()) {
        return ERROR2(ERR_COULDNT_CREATE_OBJECT_FOR_2, "coordinatesystem", grf->name());
    }
    grf->coordinateSystem(csy);

    QSize sz(gdal()->xsize(_dataSet), gdal()->ysize(_dataSet));
    double geosys[6];
    CPLErr err = gdal()->getGeotransform(_dataSet, geosys) ;
    if ( err != CE_None) {
        return ERROR2(ERR_INVALID_PROPERTY_FOR_2, "Georeference", grf->name());
    }

    double a1 = geosys[0];
    double b1 = geosys[3];
    double a2 = geosys[1];
    double b2 = geosys[5];
    Coordinate crdLeftup( a1 , b1);
    Coordinate crdRightDown(a1 + sz.width() * a2, b1 + sz.height() * b2 ) ;
    Coordinate cMin( min(crdLeftup.x(), crdRightDown.x()), min(crdLeftup.y(), crdRightDown.y()));
    Coordinate cMax( max(crdLeftup.x(), crdRightDown.x()), max(crdLeftup.y(), crdRightDown.y()));

    grf->setEnvelope(Box2D<double>(cMin, cMax));


    return true;
}

IlwisObject *GeorefConnector::create() const{
    //TODO tiepoints georef
    return new CornersGeoReference(_resource);



}


