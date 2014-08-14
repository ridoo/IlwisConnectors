#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QList>

#include "kernel.h"
#include "ilwisdata.h"
#include "geometries.h"
#include "feature.h"
#include "coverage.h"
#include "table.h"
#include "basetable.h"
#include "flattable.h"
#include "featurecoverage.h"
#include "geometryhelper.h"

#include "postgresqlfeaturecoverageloader.h"
#include "postgresqltableloader.h"
#include "postgresqldatabaseutil.h"

using namespace Ilwis;
using namespace Postgresql;

PostgresqlFeatureCoverageLoader::PostgresqlFeatureCoverageLoader(const Resource resource): _resource(resource)
{
    PostgresqlDatabaseUtil::openForResource(_resource,"featurecoverageloader");
}

PostgresqlFeatureCoverageLoader::~PostgresqlFeatureCoverageLoader()
{
    QSqlDatabase::removeDatabase("featurecoverageloader");
}

bool PostgresqlFeatureCoverageLoader::loadMetadata(FeatureCoverage *fcoverage) const
{
    qDebug() << "PostgresqlFeatureCoverageLoader::loadMetadata()";

    QString name = fcoverage->name();
    quint64 id = fcoverage->id();
    QString schemaResource(PostgresqlDatabaseUtil::getInternalNameFrom(name, id));

    ITable featureTable;
    Resource resource(schemaResource, itFLATTABLE);
    if(!featureTable.prepare(resource)) {
        ERROR1(ERR_NO_INITIALIZED_1, resource.name());
        return false;
    }

    if ( !featureTable.isValid()) {
        ERROR0(TR("Could not prepare feature table for database feature."));
        return false;
    }

    PostgresqlTableLoader loader(_resource);
    if ( !loader.loadMetadata(featureTable.ptr())) {
        ERROR1("Could not load table metadata for table '%1'", featureTable->name());
        return false;
    }


    fcoverage->attributeTable(featureTable);
    if (sizeof(featureTable->column(FEATUREIDCOLUMN)) != 0) {
        setFeatureCount(fcoverage);
        setSpatialMetadata(fcoverage);
    }

    return true;
}

QSqlQuery PostgresqlFeatureCoverageLoader::selectGeometries(const QList<MetaGeometryColumn> metaGeometry) const
{
    QString columns;
    std::for_each(metaGeometry.begin(), metaGeometry.end(), [&columns](MetaGeometryColumn meta) {
        //columns.append(" ST_ASeWKB(");
        columns.append(" ST_AsText(");
        columns.append(meta.geomColumn).append(") AS ");
        columns.append(meta.geomColumn).append(",");
    });
    columns = columns.left(columns.size() - 1);

    QString sqlBuilder;
    sqlBuilder.append("SELECT ");
    sqlBuilder.append(columns);
    sqlBuilder.append(" FROM ");
    sqlBuilder.append(PostgresqlDatabaseUtil::qTableFromTableResource(_resource));
    qDebug() << "SQL: " << sqlBuilder;

    QSqlDatabase db = QSqlDatabase::database("featurecoverageloader");
    return db.exec(sqlBuilder);
}

bool PostgresqlFeatureCoverageLoader::loadData(FeatureCoverage *fcoverage) const
{
    qDebug() << "PostgresqlFeatureCoverageLoader::loadData()";

    PostgresqlTableLoader loader(_resource);
    if ( !loader.loadData(fcoverage->attributeTable().ptr())) {

        // TODO handle exception message

    }

    QList<MetaGeometryColumn> metaGeometries;
    PostgresqlDatabaseUtil::getMetaForGeometryColumns(_resource,metaGeometries);
    QSqlQuery query = selectGeometries(metaGeometries);

    while (query.next()) {
        if (metaGeometries.size() > 0) {
            MetaGeometryColumn meta = metaGeometries.at(0);
            geos::geom::Geometry *geom = createGeometry(query,meta);
            fcoverage->newFeature(geom,false);

            // TODO how to add further geometries to the feature (according to the trackindex)?
        }
    }

    return true;

}

geos::geom::Geometry* PostgresqlFeatureCoverageLoader::createGeometry(QSqlQuery &query, MetaGeometryColumn &meta) const
{
    ICoordinateSystem crs = meta.crs;

    // postgis wkb is different from ogc wkb
    // => select ewkb, but this seems to be slower than selecting wkt
    //ByteArray wkbBytes = variant.toByteArray();
    QVariant variant = query.value(meta.geomColumn);
    QString wkt = variant.toString();
    return GeometryHelper::fromWKT(wkt,crs);
}


void PostgresqlFeatureCoverageLoader::setFeatureCount(FeatureCoverage *fcoverage) const
{
    qDebug() << "PostgresqlFeatureCoverageLoader::setFeatureCount()";

    QList<MetaGeometryColumn> metaGeometries;
    PostgresqlDatabaseUtil::getMetaForGeometryColumns(_resource, metaGeometries);

    foreach (MetaGeometryColumn meta, metaGeometries) {
        //select distinct st_geometrytype(geom), count
        //    from public.tl_2010_us_rails,
        //    ( select count(*) from public.tl_2010_us_rails where not st_isEmpty(geom))
        //     AS count;

        QString sqlBuilder;
        sqlBuilder.append("SELECT  ");
        sqlBuilder.append(" count( * )");
        sqlBuilder.append(" AS count ");
        sqlBuilder.append(" FROM ");
        sqlBuilder.append(" (SELECT ");
        sqlBuilder.append(" * ");
        sqlBuilder.append(" FROM ");
        sqlBuilder.append(meta.qtablename());
        sqlBuilder.append(" WHERE NOT ");
        sqlBuilder.append(" ST_isEmpty( ");
        sqlBuilder.append(meta.geomColumn);
        sqlBuilder.append(" ) ");
        sqlBuilder.append(" ) AS not_null ;");
        qDebug() << "SQL: " << sqlBuilder;

        QSqlDatabase db = QSqlDatabase::database("featureconnector");
        QSqlQuery query = db.exec(sqlBuilder);


        // TODO Consider multiple geometries for each feature which might have
        // different geometry types as well! Shall we stretch the count for
        // features having multiple geometries, though the feature is the same
        // (is it really the same by definition?)


        if (query.next()) {
            IlwisTypes types = meta.geomType;
            int count = query.value("count").toInt();
            if (count > 0) {
                fcoverage->setFeatureCount(types, count, 0);
            }
        }
    }
}

void PostgresqlFeatureCoverageLoader::setSpatialMetadata(FeatureCoverage *fcoverage) const
{
    qDebug() << "PostgresqlFeatureCoverageLoader::setSpatialMetadata()";

    QList<MetaGeometryColumn> metaGeometries;
    PostgresqlDatabaseUtil::getMetaForGeometryColumns(_resource, metaGeometries);

    Envelope bbox;
    ICoordinateSystem crs;

    QSqlDatabase db = QSqlDatabase::database("featureconnector");
    foreach (MetaGeometryColumn meta, metaGeometries) {
        QString sqlBuilder;
        sqlBuilder.append("SELECT ");
        sqlBuilder.append("st_extent( ");
        sqlBuilder.append(meta.geomColumn);
        sqlBuilder.append(" ) ");
        sqlBuilder.append(" FROM ");
        sqlBuilder.append(meta.qtablename());
        sqlBuilder.append(";");
        qDebug() << "SQL: " << sqlBuilder;

        QSqlQuery envelopeQuery = db.exec(sqlBuilder);

        if (envelopeQuery.next()) {

            // TODO check with Martin how to handle multiple
            // geometries for one entity
            QString envString = envelopeQuery.value(0).toString();
            if ( !envString.isEmpty()) {
                Envelope envelope(envString);
                bbox += envelope;
            }
        }

        if ( !crs.isValid() && meta.crs.isValid()) {
            // first valid srid found is being considered as "main" crs.
            //
            // note: if multiple geom columns do exist, the geometries have
            // to be transformed to the "main" one when actual data is loaded
            crs = meta.crs;
        }

        fcoverage->coordinateSystem(crs);
        fcoverage->envelope(bbox);
    }
}




