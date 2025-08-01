/***************************************************************************
  distancearea.cpp - DistanceArea

 ---------------------
 begin                : 23.2.2017
 copyright            : (C) 2017 by Matthias Kuhn
 email                : matthias@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "distancearea.h"
#include "rubberbandmodel.h"

#include <qgis.h>
#include <qgslinestring.h>
#include <qgspolygon.h>
#include <qgsproject.h>
#include <qgsvectorlayer.h>

DistanceArea::DistanceArea( QObject *parent )
  : QObject( parent )
  , mRubberbandModel( nullptr )
  , mProject( nullptr )
{
}

void DistanceArea::init()
{
  if ( mProject )
  {
    mDistanceArea.setEllipsoid( mProject->ellipsoid() );
    mDistanceArea.setSourceCrs( mCrs, mProject->transformContext() );
  }
  else
  {
    mDistanceArea.setEllipsoid( Qgis::geoNone() );
  }

  emit lengthUnitsChanged();
  emit areaUnitsChanged();
}

QgsProject *DistanceArea::project() const
{
  return mProject;
}

void DistanceArea::setProject( QgsProject *project )
{
  if ( mProject == project )
    return;

  if ( mProject )
    disconnect( mProject, &QgsProject::readProject, this, &DistanceArea::init );

  mProject = project;

  if ( mProject )
    connect( mProject, &QgsProject::readProject, this, &DistanceArea::init );

  init();

  emit projectChanged();
}

RubberbandModel *DistanceArea::rubberbandModel() const
{
  return mRubberbandModel;
}

void DistanceArea::setRubberbandModel( RubberbandModel *rubberbandModel )
{
  if ( mRubberbandModel == rubberbandModel )
    return;

  if ( mRubberbandModel )
  {
    disconnect( mRubberbandModel, &RubberbandModel::vertexChanged, this, &DistanceArea::lengthChanged );
    disconnect( mRubberbandModel, &RubberbandModel::vertexChanged, this, &DistanceArea::perimeterChanged );
    disconnect( mRubberbandModel, &RubberbandModel::vertexChanged, this, &DistanceArea::areaChanged );
    disconnect( mRubberbandModel, &RubberbandModel::vertexChanged, this, &DistanceArea::segmentLengthChanged );
    disconnect( mRubberbandModel, &RubberbandModel::vertexChanged, this, &DistanceArea::azimuthChanged );
  }

  mRubberbandModel = rubberbandModel;

  if ( mRubberbandModel )
  {
    connect( mRubberbandModel, &RubberbandModel::vertexChanged, this, &DistanceArea::lengthChanged );
    connect( mRubberbandModel, &RubberbandModel::vertexChanged, this, &DistanceArea::perimeterChanged );
    connect( mRubberbandModel, &RubberbandModel::vertexChanged, this, &DistanceArea::areaChanged );
    connect( mRubberbandModel, &RubberbandModel::vertexChanged, this, &DistanceArea::segmentLengthChanged );
    connect( mRubberbandModel, &RubberbandModel::vertexChanged, this, &DistanceArea::azimuthChanged );
  }

  emit rubberbandModelChanged();
}

QgsCoordinateReferenceSystem DistanceArea::crs() const
{
  return mCrs;
}

void DistanceArea::setCrs( const QgsCoordinateReferenceSystem &crs )
{
  if ( mCrs == crs )
    return;

  mCrs = crs;
  init();
  emit crsChanged();
}
#if 0
void DistanceArea::setGeometry( Geometry *geometry )
{
  if ( mGeometry == geometry )
    return;

  if ( mGeometry )
    disconnect( mGeometry, &Geometry::rubberbandModelChanged, this, &DistanceArea::rubberbandModelChanged );

  mGeometry = geometry;
  rubberbandModelChanged();

  if ( mGeometry )
    connect( mGeometry, &Geometry::rubberbandModelChanged, this, &DistanceArea::rubberbandModelChanged );

  init();

  emit geometryChanged();
}
#endif

qreal DistanceArea::length() const
{
  double length = std::numeric_limits<double>::quiet_NaN();
  if ( mRubberbandModel )
  {
    try
    {
      length = mDistanceArea.measureLine( mRubberbandModel->flatPointSequence( mCrs ) );
    }
    catch ( const QgsException & )
    {
      length = std::numeric_limits<double>::quiet_NaN();
    }
  }

  return length;
}

bool DistanceArea::lengthValid() const
{
  if ( !mRubberbandModel )
    return false;

  switch ( mRubberbandModel->geometryType() )
  {
    case Qgis::GeometryType::Point:
      return false;

    case Qgis::GeometryType::Line:
      [[fallthrough]];
    case Qgis::GeometryType::Polygon:
      return mRubberbandModel->vertexCount() >= 2;

    default:
      return false;
  }
}

qreal DistanceArea::perimeter() const
{
  if ( mRubberbandModel )
  {
    QgsGeometry geom( new QgsPolygon( new QgsLineString( mRubberbandModel->flatPointSequence( mCrs ) ) ) );
    return mDistanceArea.measurePerimeter( geom );
  }

  return qQNaN();
}

bool DistanceArea::perimeterValid() const
{
  if ( !mRubberbandModel )
    return false;

  switch ( mRubberbandModel->geometryType() )
  {
    case Qgis::GeometryType::Point:
      [[fallthrough]];
    case Qgis::GeometryType::Line:
      return false;

    case Qgis::GeometryType::Polygon:
      return mRubberbandModel->vertexCount() >= 3;

    default:
      return false;
  }
}

qreal DistanceArea::area() const
{
  double area = std::numeric_limits<double>::quiet_NaN();
  if ( mRubberbandModel )
  {
    try
    {
      area = mDistanceArea.measurePolygon( mRubberbandModel->flatPointSequence( mCrs ) );
    }
    catch ( const QgsException & )
    {
      area = std::numeric_limits<double>::quiet_NaN();
    }
  }

  return area;
}

bool DistanceArea::areaValid() const
{
  if ( !mRubberbandModel )
    return false;

  switch ( mRubberbandModel->geometryType() )
  {
    case Qgis::GeometryType::Point:
      return false;

    case Qgis::GeometryType::Line:
      return false;

    case Qgis::GeometryType::Polygon:
      return mRubberbandModel->vertexCount() >= 3;

    default:
      return false;
  }
}

qreal DistanceArea::segmentLength() const
{
  if ( !mRubberbandModel )
    return qQNaN();

  if ( mRubberbandModel->vertexCount() < 2 )
    return qQNaN();

  QVector<QgsPointXY> points = mRubberbandModel->flatPointSequence( mCrs );

  auto pointIt = points.constEnd() - 1;

  QVector<QgsPointXY> flatPoints;

  flatPoints << *pointIt;
  pointIt--;
  flatPoints << *pointIt;

  double length = std::numeric_limits<double>::quiet_NaN();
  try
  {
    length = mDistanceArea.measureLine( flatPoints );
  }
  catch ( const QgsException & )
  {
    length = std::numeric_limits<double>::quiet_NaN();
  }
  return length;
}

qreal DistanceArea::azimuth() const
{
  if ( !mRubberbandModel )
    return qQNaN();

  if ( mRubberbandModel->vertexCount() < 2 )
    return qQNaN();

  QVector<QgsPointXY> points = mRubberbandModel->flatPointSequence( mCrs );
  QgsPoint startPoint( points.at( points.size() - 2 ) );
  QgsPoint endPoint( points.at( points.size() - 1 ) );

  return startPoint.azimuth( endPoint );
}

Qgis::DistanceUnit DistanceArea::lengthUnits() const
{
  return mDistanceArea.lengthUnits();
}

Qgis::AreaUnit DistanceArea::areaUnits() const
{
  return mDistanceArea.areaUnits();
}

double DistanceArea::convertLengthMeansurement( double length, Qgis::DistanceUnit toUnits ) const
{
  return mDistanceArea.convertLengthMeasurement( length, toUnits );
}

double DistanceArea::convertAreaMeansurement( double area, Qgis::AreaUnit toUnits ) const
{
  return mDistanceArea.convertAreaMeasurement( area, toUnits );
}
