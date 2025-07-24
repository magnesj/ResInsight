/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025     Equinor ASA
//
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
//
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#include "RiaFileSearchTools.h"

#include "RiaFilePathTools.h"

#include <QDir>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QStringList RiaFileSearchTools::findFilesInDirs( const QStringList& dirs, const QStringList& filters )
{
    QStringList allFiles;

    for ( const auto& dir : dirs )
    {
        QDir        qdir( dir );
        QStringList files = qdir.entryList( filters, QDir::Files );

        for ( const auto& file : files )
        {
            QString absFilePath = qdir.absoluteFilePath( file );
            allFiles.append( absFilePath );
        }
    }
    return allFiles;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaFileSearchTools::buildDirectoryListRecursiveSimple( const QString& currentDir,
                                                            const QString& currentPathFilter,
                                                            QStringList*   accumulatedDirs )
{
    QString currDir    = currentDir;
    QString pathFilter = currentPathFilter;

    QStringList pathFilterPartList = pathFilter.split( RiaFilePathTools::separator() );
    if ( pathFilterPartList.isEmpty() )
    {
        accumulatedDirs->push_back( currentDir );
        return;
    }

    QDir        qdir( currDir, pathFilterPartList[0], QDir::NoSort, QDir::Dirs | QDir::NoDotAndDotDot );
    QStringList subDirs = qdir.entryList();

    if ( pathFilterPartList.size() == 1 && pathFilterPartList[0] == "*" )
    {
        accumulatedDirs->push_back( currDir );
    }

    for ( const QString& subDir : subDirs )
    {
        QString fullPath = qdir.absoluteFilePath( subDir );
        QString nextPathFilter;

        if ( pathFilterPartList.size() == 1 && pathFilterPartList[0] == "*" )
        {
            nextPathFilter = "*";
        }
        else
        {
            auto pf = pathFilterPartList;
            pf.removeFirst();
            nextPathFilter = pf.join( RiaFilePathTools::separator() );
        }

        buildDirectoryListRecursiveSimple( fullPath, nextPathFilter, accumulatedDirs );
    }
}
