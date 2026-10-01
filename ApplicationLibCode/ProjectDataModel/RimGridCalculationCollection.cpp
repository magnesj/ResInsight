/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2022     Equinor ASA
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

#include "RimGridCalculationCollection.h"

#include "RiaLogging.h"
#include "RigEclipseResultAddress.h"
#include "RimEclipseCase.h"
#include "RimEclipseResultDefinition.h"
#include "RimGridCalculation.h"
#include "RimProject.h"

#include "cafPdmUiGroup.h"
#include "cafPdmUiTreeSelectionEditor.h"

CAF_PDM_SOURCE_INIT( RimGridCalculationCollection, "RimGridCalculationCollection" );
//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimGridCalculationCollection::RimGridCalculationCollection()
{
    CAF_PDM_InitObject( "Calculation Collection", ":/chain.png" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimGridCalculation* RimGridCalculationCollection::createCalculation() const
{
    return new RimGridCalculation;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimGridCalculation*> RimGridCalculationCollection::sortedGridCalculations() const
{
    std::vector<RimGridCalculation*> sortedCalculations;
    for ( auto userCalculation : calculations() )
    {
        auto gridCalculation = dynamic_cast<RimGridCalculation*>( userCalculation );
        if ( gridCalculation ) sortedCalculations.emplace_back( gridCalculation );
    }

    // Check if source calculation is depending on other. Will check one level dependency.
    auto isSourceDependingOnOther = []( const RimGridCalculation* source, const RimGridCalculation* other ) -> bool
    {
        auto outputCases = source->outputEclipseCases();
        auto outputAdr   = source->outputAddress();

        for ( auto v : other->allVariables() )
        {
            auto gridVariable = dynamic_cast<RimGridCalculationVariable*>( v );
            if ( std::find( outputCases.begin(), outputCases.end(), gridVariable->eclipseCase() ) != outputCases.end() &&
                 outputAdr.resultCatType() == gridVariable->resultCategoryType() && outputAdr.resultName() == gridVariable->resultVariable() )
            {
                return true;
            }
        }

        return false;
    };

    for ( auto source : sortedCalculations )
    {
        for ( auto other : sortedCalculations )
        {
            if ( source == other ) continue;

            if ( isSourceDependingOnOther( source, other ) && isSourceDependingOnOther( other, source ) )
            {
                QString txt = "Detected circular dependency between " + source->description() + " and " + other->description();
                RiaLogging::error( txt.toStdString() );

                return sortedCalculations;
            }
        }
    }

    std::sort( sortedCalculations.begin(), sortedCalculations.end(), isSourceDependingOnOther );

    return sortedCalculations;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimGridCalculation*> RimGridCalculationCollection::dependentCalculations( RimGridCalculation* sourceCalculation ) const
{
    // Find all dependent grid calculations recursively. The ordering of calculations is least dependent first.

    std::vector<RimGridCalculation*> calculations;

    if ( !dependentCalculationsRecursively( sourceCalculation, calculations ) ) return {};

    std::reverse( calculations.begin(), calculations.end() );

    return calculations;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridCalculationCollection::rebuildCaseMetaData()
{
    ensureValidCalculationIds();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimGridCalculation* RimGridCalculationCollection::findCalculation( const QString& calculationName ) const
{
    for ( auto userCalculation : calculations() )
    {
        auto gridCalculation = dynamic_cast<RimGridCalculation*>( userCalculation );
        if ( gridCalculation && gridCalculation->shortName() == calculationName ) return gridCalculation;
    }

    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimGridCalculationCollection::dependentCalculationsRecursively( RimGridCalculation*               sourceCalculation,
                                                                     std::vector<RimGridCalculation*>& calculations ) const
{
    if ( std::find( calculations.begin(), calculations.end(), sourceCalculation ) != calculations.end() )
    {
        RiaLogging::error( "Detected circular dependency for " + sourceCalculation->description().toStdString() );
        return false;
    }

    calculations.push_back( sourceCalculation );

    for ( auto v : sourceCalculation->allVariables() )
    {
        auto gridVariable = dynamic_cast<RimGridCalculationVariable*>( v );
        if ( gridVariable->resultCategoryType() == RiaDefines::ResultCatType::GENERATED )
        {
            if ( auto other = findCalculation( gridVariable->resultVariable() ) )
            {
                if ( !dependentCalculationsRecursively( other, calculations ) ) return false;
            }
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridCalculationCollection::initAfterRead()
{
    rebuildCaseMetaData();
}

//--------------------------------------------------------------------------------------------------
/// A GENERATED result produced by a grid calculation only exists in memory for the realization it was
/// computed for. Ensemble realizations are opened, used and closed on demand (see RimGridCalculation::
/// casesToCalculate), so the result may not have been computed for this particular case yet. Recompute it
/// here for the given case if a matching calculation is found, so the result is available whenever a view
/// or contour map is created/opened for an ensemble realization.
//--------------------------------------------------------------------------------------------------
void RimGridCalculationCollection::ensureGeneratedResultIsComputed( const RimEclipseResultDefinition* resultDefinition,
                                                                    RimEclipseCase*                   eclipseCase )
{
    if ( !resultDefinition || resultDefinition->resultType() != RiaDefines::ResultCatType::GENERATED ) return;
    if ( !eclipseCase ) return;

    auto project = RimProject::current();
    if ( !project ) return;

    RimGridCalculation* calculation = project->gridCalculationCollection()->findCalculation( resultDefinition->resultVariable() );
    if ( !calculation ) return;

    const bool evaluateDependentCalculations = true;
    calculation->calculateForCases( { eclipseCase }, nullptr, std::nullopt, evaluateDependentCalculations );
}

//--------------------------------------------------------------------------------------------------
/// The cell-result selection dropdown only lists GENERATED result names already present in the given
/// case's result catalog (see RigCaseCellResultsData::resultNames). For an ensemble realization that has
/// not been opened before, that catalog is empty, so no calculated result can even be selected. Recompute
/// every grid calculation whose destination includes this case, so its result becomes selectable and its
/// data available as soon as the case is opened (e.g. when stepping through realizations in a view).
//--------------------------------------------------------------------------------------------------
void RimGridCalculationCollection::ensureGeneratedResultsAreComputed( RimEclipseCase* eclipseCase )
{
    if ( !eclipseCase ) return;

    auto project = RimProject::current();
    if ( !project ) return;

    for ( auto gridCalculation : project->gridCalculationCollection()->sortedGridCalculations() )
    {
        auto outputCases = gridCalculation->outputEclipseCases();
        if ( std::find( outputCases.begin(), outputCases.end(), eclipseCase ) == outputCases.end() ) continue;

        const bool evaluateDependentCalculations = true;
        gridCalculation->calculateForCases( { eclipseCase }, nullptr, std::nullopt, evaluateDependentCalculations );
    }
}
