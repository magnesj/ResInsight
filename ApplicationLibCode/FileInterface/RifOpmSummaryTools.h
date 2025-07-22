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

#pragma once

#include "RifEclipseSummaryAddress.h"

#include <QString>

#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace Opm
{
namespace EclIO
{
    class ESmry;
    class ExtESmry;
} // namespace EclIO
} // namespace Opm

namespace RifOpmSummaryTools
{
class RifEnsembleImportState
{
public:
    bool shouldCreateEsmyFile() const { return m_shouldCreateEsmyFile; }
    void setShouldCreateEsmyFile( bool val ) { m_shouldCreateEsmyFile = val; }

    const std::vector<QString> restartFiles( int realizationNumber ) const { return m_restartFiles; }
    void                       setRestartFiles( const std::vector<QString>& restartFiles ) { m_restartFiles = restartFiles; }

    QString pathToParameterFile() const { return m_pathToParameterFile; }
    void    setPathToParameterFile( const QString& val ) { m_pathToParameterFile = val; }

private:
    bool                 m_shouldCreateEsmyFile = false;
    std::vector<QString> m_restartFiles;
    QString              m_pathToParameterFile;
};

std::tuple<std::set<RifEclipseSummaryAddress>, std::map<RifEclipseSummaryAddress, size_t>, std::map<RifEclipseSummaryAddress, std::string>>
    buildAddressesSmspecAndKeywordMap( const Opm::EclIO::ESmry* summaryFile );

std::pair<std::set<RifEclipseSummaryAddress>, std::map<RifEclipseSummaryAddress, std::string>>
    buildAddressesAndKeywordMap( const std::vector<std::string>& keywords );

SummaryCategory categoryFromKeyword( const std::string& keyword );

QString enhancedSummaryFilename( const QString& fileName );
QString smspecSummaryFilename( const QString& fileName );
bool    isEsmryConversionRequired( const QString& fileName );

}; // namespace RifOpmSummaryTools
