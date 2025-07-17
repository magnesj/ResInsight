/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Statoil ASA
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

#include "RifCaseRealizationParametersReader.h"
#include "RifFileParseTools.h"

#include "RiaLogging.h"
#include "RiaStdStringTools.h"
#include "RiaTextStringTools.h"

#include <QDir>
#include <QString>
#include <QStringList>

#include <functional>
#include <memory>

#pragma optimize( "", off )

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RifCaseRealizationReader::RifCaseRealizationReader( const QString& fileName )
{
    m_parameters = std::make_shared<RigCaseRealizationParameters>();
    m_fileName   = fileName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RifCaseRealizationReader::~RifCaseRealizationReader()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const std::shared_ptr<RigCaseRealizationParameters> RifCaseRealizationReader::parameters() const
{
    return m_parameters;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::shared_ptr<RifCaseRealizationReader> RifCaseRealizationReader::createReaderFromFileName( const QString& fileName )
{
    std::shared_ptr<RifCaseRealizationReader> reader;

    if ( fileName.endsWith( parametersFileName() ) )
    {
        reader = std::make_shared<RifCaseRealizationParametersReader>( fileName );
    }
    else if ( fileName.endsWith( runSpecificationFileName() ) )
    {
        reader = std::make_shared<RifCaseRealizationRunspecificationReader>( fileName );
    }
    return reader;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RifCaseRealizationReader::parametersFileName()
{
    return "parameters.txt";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RifCaseRealizationReader::runSpecificationFileName()
{
    return "runspecification.xml";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RifCaseRealizationParametersReader::RifCaseRealizationParametersReader( const QString& fileName )
    : RifCaseRealizationReader( fileName )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RifCaseRealizationParametersReader::~RifCaseRealizationParametersReader()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RifCaseRealizationParametersReader::parse()
{
    QFile file( m_fileName );
    if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) ) return;

    QTextStream dataStream( &file );

    int         lineNo = 0;
    QStringList errors;

    const char decimalPoint = '.';

    while ( !dataStream.atEnd() )
    {
        QString line = dataStream.readLine();

        lineNo++;

        auto splitFast = []( std::string_view str ) -> std::pair<std::string_view, std::string_view>
        {
            const char* data = str.data();
            const char* end  = data + str.size();
            const char* pos  = data;

            // Find separator, which is the first space or tab character
            while ( pos < end && *pos != ' ' && *pos != '\t' )
                ++pos;

            if ( pos >= end ) return { str, {} };

            std::string_view first( data, pos - data );

            // Skip all separators (multiple whitespace or tab characters)
            while ( pos < end && ( *pos == ' ' || *pos == '\t' ) )
                ++pos;

            if ( pos >= end ) return { first, {} };

            std::string_view second( pos, end - pos );

            return { first, second };
        };

        auto [name, strValue] = splitFast( line.toStdString() );

        if ( name.empty() || strValue.empty() )
        {
            errors << QString( "RifCaseRealizationParametersReader: Invalid file format in line %1" ).arg( lineNo );
            continue;
        }

        if ( RiaStdStringTools::isNumber( strValue, decimalPoint ) )
        {
            double value = 0.0;
            RiaStdStringTools::toDouble( strValue, value );
            m_parameters->addParameter( QString::fromUtf8( name.data(), name.size() ), value );
        }
        else
        {
            m_parameters->addParameter( QString::fromUtf8( name.data(), name.size() ), QString::fromUtf8( strValue.data(), strValue.size() ) );
        }
    }

    for ( const auto& s : errors )
    {
        RiaLogging::warning( s );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RifCaseRealizationRunspecificationReader::RifCaseRealizationRunspecificationReader( const QString& fileName )
    : RifCaseRealizationReader( fileName )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RifCaseRealizationRunspecificationReader::~RifCaseRealizationRunspecificationReader()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RifCaseRealizationRunspecificationReader::parse()
{
    QFile file( m_fileName );
    if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) ) return;

    QXmlStreamReader xml( &file );

    QStringList errors;

    QString paramName;

    while ( !xml.atEnd() )
    {
        xml.readNext();

        if ( xml.isStartElement() )
        {
            if ( RiaTextStringTools::isTextEqual( xml.name(), QString( "modifier" ) ) )
            {
                paramName = "";
            }

            if ( RiaTextStringTools::isTextEqual( xml.name(), QString( "id" ) ) )
            {
                paramName = xml.readElementText();
            }

            if ( RiaTextStringTools::isTextEqual( xml.name(), QString( "value" ) ) )
            {
                QString paramStrValue = xml.readElementText();

                if ( paramName.isEmpty() ) continue;

                if ( RiaTextStringTools::isNumber( paramStrValue, QLocale::c().decimalPoint() ) )
                {
                    bool   parseOk = true;
                    double value   = QLocale::c().toDouble( paramStrValue, &parseOk );
                    if ( parseOk )
                    {
                        m_parameters->addParameter( paramName, value );
                    }
                    else
                    {
                        errors
                            << QString( "RifCaseRealizationRunspecificationReader: Invalid number format in line %1" ).arg( xml.lineNumber() );
                    }
                }
                else
                {
                    m_parameters->addParameter( paramName, paramStrValue );
                }
            }
        }
        else if ( xml.isEndElement() )
        {
            if ( RiaTextStringTools::isTextEqual( xml.name(), QString( "modifier" ) ) )
            {
                paramName = "";
            }
        }
    }

    for ( const auto& s : errors )
    {
        RiaLogging::warning( s );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RifCaseRealizationParametersFileLocator::locate( const QString& modelPath )
{
    // Chosen to find parameters file for StimPlan ensembles.
    int MAX_LEVELS_UP = 5;
    int dirLevel      = 0;

    QDir qdir( modelPath );

    const QFileInfo dir( modelPath );
    if ( dir.isFile() )
        qdir.cdUp();
    else if ( !dir.isDir() )
        return "";

    do
    {
        QStringList files = qdir.entryList( QDir::Files | QDir::NoDotAndDotDot );
        for ( const QString& file : files )
        {
            if ( QString::compare( file, RifCaseRealizationReader::parametersFileName(), Qt::CaseInsensitive ) == 0 ||
                 QString::compare( file, RifCaseRealizationReader::runSpecificationFileName(), Qt::CaseInsensitive ) == 0 )
            {
                return qdir.absoluteFilePath( file );
            }
        }
        qdir.cdUp();

    } while ( dirLevel++ < MAX_LEVELS_UP );

    return "";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RifCaseRealizationParametersFileLocator::realizationNumber( const QString& modelPath )
{
    QDir    dir( modelPath );
    QString absolutePath = dir.absolutePath();

    return realizationNumberFromFullPath( absolutePath );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RifCaseRealizationParametersFileLocator::realizationNumberFromFullPath( const QString& path )
{
    int resultIndex = -1;

    QRegularExpression      pattern( "realization-(\\d+)", QRegularExpression::CaseInsensitiveOption );
    QRegularExpressionMatch match = pattern.match( path );

    if ( match.hasMatch() )
    {
        resultIndex = match.captured( 1 ).toInt();
    }

    return resultIndex;
}
