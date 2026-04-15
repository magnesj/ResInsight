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

#include "RicCreateEnsembleParameterCsvReportFeature.h"

#include "RicShowPlotDataFeature.h"

#include "Summary/Ensemble/RimSummaryEnsembleParameterCollection.h"
#include "Summary/RimSummaryCase.h"
#include "Summary/RimSummaryEnsemble.h"

#include "RigCaseRealizationParameters.h"

#include "cafSelectionManagerTools.h"

#include <QAction>

CAF_CMD_SOURCE_INIT( RicCreateEnsembleParameterCsvReportFeature, "RicCreateEnsembleParameterCsvReportFeature" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicCreateEnsembleParameterCsvReportFeature::isCommandEnabled() const
{
    return selectedParameterCollection() != nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicCreateEnsembleParameterCsvReportFeature::onActionTriggered( bool isChecked )
{
    RimSummaryEnsembleParameterCollection* paramCollection = selectedParameterCollection();
    if ( !paramCollection ) return;

    QString title = "Ensemble Parameter Report";
    QString text  = createCsvText( paramCollection );

    RicShowPlotDataFeature::showTextWindow( title, text );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicCreateEnsembleParameterCsvReportFeature::setupActionLook( QAction* actionToSetup )
{
    actionToSetup->setText( "Create Unified CSV Report" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSummaryEnsembleParameterCollection* RicCreateEnsembleParameterCsvReportFeature::selectedParameterCollection()
{
    auto selection = caf::selectedObjectsByType<RimSummaryEnsembleParameterCollection*>();
    if ( !selection.empty() ) return selection.front();
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RicCreateEnsembleParameterCsvReportFeature::createCsvText( RimSummaryEnsembleParameterCollection* paramCollection )
{
    auto ensemble = paramCollection->firstAncestorOrThisOfType<RimSummaryEnsemble>();
    if ( !ensemble ) return {};

    std::vector<RimSummaryCase*> cases = ensemble->allSummaryCases();

    // Collect sorted parameter names from first case that has parameters
    std::vector<QString> paramNames;
    for ( auto summaryCase : cases )
    {
        auto crlParams = summaryCase->caseRealizationParameters();
        if ( !crlParams ) continue;

        for ( const auto& paramPair : crlParams->parameters() )
        {
            paramNames.push_back( paramPair.first );
        }
        break;
    }

    if ( paramNames.empty() ) return {};

    // Header line
    QString text = "REAL";
    for ( const auto& name : paramNames )
    {
        text += " " + name;
    }
    text += "\n";

    // One row per realization
    for ( auto summaryCase : cases )
    {
        auto crlParams = summaryCase->caseRealizationParameters();
        if ( !crlParams ) continue;

        int realizationNumber = crlParams->realizationNumber();
        text += QString( "%1" ).arg( realizationNumber, 5, 10, QChar( '0' ) );

        const auto allParams = crlParams->parameters();
        for ( const auto& name : paramNames )
        {
            auto it = allParams.find( name );
            if ( it != allParams.end() )
            {
                const RigCaseRealizationParameters::Value& val = it->second;
                if ( val.isNumeric() )
                {
                    text += " " + QString::number( val.numericValue() );
                }
                else if ( val.isText() )
                {
                    text += " " + val.textValue();
                }
                else
                {
                    text += " ";
                }
            }
            else
            {
                text += " ";
            }
        }
        text += "\n";
    }

    return text;
}
