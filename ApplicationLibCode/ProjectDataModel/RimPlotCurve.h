/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2016-     Statoil ASA
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

#include "RimPlotCurveAppearance.h"

#include "RiaCurveDataTools.h"
#include "RiaDefines.h"

#include "RiuQwtPlotCurveDefines.h"
#include "RiuQwtSymbol.h"

#include "cafPdmChildField.h"
#include "cafPdmField.h"
#include "cafPdmFieldCvfColor.h"
#include "cafPdmObject.h"

#include <QPointer>
#include <Qt>

class QwtPlot;
class QwtPlotCurve;
class QwtPlotIntervalCurve;
class RiuQwtPlotCurve;

//==================================================================================================
///
///
//==================================================================================================
class RimPlotCurve : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    caf::Signal<>        appearanceChanged;
    caf::Signal<bool>    visibilityChanged;
    caf::Signal<>        dataChanged;
    caf::Signal<QString> nameChanged;

public:
    RimPlotCurve();
    ~RimPlotCurve() override;

    void loadDataAndUpdate( bool updateParentPlot );

    virtual bool xValueRangeInQwt( double* minimumValue, double* maximumValue ) const;
    virtual bool yValueRangeInQwt( double* minimumValue, double* maximumValue ) const;

    void          setParentQwtPlotAndReplot( QwtPlot* plot );
    void          setParentQwtPlotNoReplot( QwtPlot* plot );
    void          detachQwtCurve();
    void          reattachQwtCurve();
    QwtPlotCurve* qwtPlotCurve() const;

    void                          setColor( const cvf::Color3f& color );
    cvf::Color3f                  color() const;
    void                          setLineStyle( RiuQwtPlotCurveDefines::LineStyleEnum lineStyle );
    void                          setSymbol( RiuQwtSymbol::PointSymbolEnum symbolStyle );
    void                          setInterpolation( RiuQwtPlotCurveDefines::CurveInterpolationEnum );
    RiuQwtSymbol::PointSymbolEnum symbol();
    int                           symbolSize() const;
    cvf::Color3f                  symbolEdgeColor() const;
    void                          setSymbolEdgeColor( const cvf::Color3f& edgeColor );
    void                          setSymbolSkipDistance( float distance );
    void                          setSymbolLabel( const QString& label );
    void                          setSymbolLabelPosition( RiuQwtSymbol::LabelPosition labelPosition );
    void                          setSymbolSize( int sizeInPixels );
    void                          setLineThickness( int thickness );
    void                          resetAppearance();
    Qt::BrushStyle                fillStyle() const;
    void                          setFillStyle( Qt::BrushStyle brushStyle );
    void                          setFillColor( const cvf::Color3f& fillColor );

    bool isCurveVisible() const;
    void setCurveVisibility( bool visible );

    void updateCurveName();
    void updateCurveNameAndUpdatePlotLegendAndTitle();
    void updateCurveNameNoLegendUpdate();

    QString         curveName() const { return m_curveName; }
    virtual QString curveExportDescription( const RifEclipseSummaryAddress& address = RifEclipseSummaryAddress() ) const
    {
        return m_curveName;
    }
    void    setCustomName( const QString& customName );
    QString legendEntryText() const;
    void    setLegendEntryText( const QString& legendEntryText );

    void updateCurveVisibility();
    void updateLegendEntryVisibilityAndPlotLegend();
    void updateLegendEntryVisibilityNoPlotUpdate();

    bool showInLegend() const;
    bool errorBarsVisible() const;

    void setShowInLegend( bool show );
    void setZOrder( double z );
    void setErrorBarsVisible( bool isVisible );

    virtual void updateCurveAppearance();
    bool         isCrossPlotCurve() const;
    void         updateUiIconFromPlotSymbol();

    void updateCurveAppearanceForFilesOlderThan_2021_06();

protected:
    virtual QString createCurveAutoName()                        = 0;
    virtual void    updateZoomInParentPlot()                     = 0;
    virtual void    onLoadDataAndUpdate( bool updateParentPlot ) = 0;
    void            initAfterRead() override;
    void            updateCurvePresentation( bool updatePlotLegendAndTitle );

    void         updateOptionSensitivity();
    void         updatePlotTitle();
    virtual void updateLegendsInPlot();

    void setSamplesFromXYErrorValues( const std::vector<double>& xValues,
                                      const std::vector<double>& yValues,
                                      const std::vector<double>& errorValues,
                                      bool                       keepOnlyPositiveValues,
                                      RiaCurveDataTools::ErrorAxis errorAxis = RiaCurveDataTools::ErrorAxis::ERROR_ALONG_Y_AXIS );
    void setSamplesFromXYValues( const std::vector<double>& xValues,
                                 const std::vector<double>& yValues,
                                 bool                       keepOnlyPositiveValues );
    void setSamplesFromDatesAndYValues( const std::vector<QDateTime>& dateTimes,
                                        const std::vector<double>&    yValues,
                                        bool                          keepOnlyPositiveValues );

    void setSamplesFromTimeTAndYValues( const std::vector<time_t>& dateTimes,
                                        const std::vector<double>& yValues,
                                        bool                       keepOnlyPositiveValues );

protected:
    // Overridden PDM methods
    void                 fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;
    caf::PdmFieldHandle* objectToggleField() override;
    caf::PdmFieldHandle* userDescriptionField() override;
    void                 appearanceUiOrdering( caf::PdmUiOrdering& uiOrdering );
    void                 curveNameUiOrdering( caf::PdmUiOrdering& uiOrdering );

    virtual void onCurveAppearanceChanged( const caf::SignalEmitter* emitter );
    virtual void onFillColorChanged( const caf::SignalEmitter* emitter );

private:
    bool canCurveBeAttached() const;
    void attachCurveAndErrorBars();
    void checkAndApplyDefaultFillColor();

protected:
    QPointer<QwtPlot> m_parentQwtPlot;

    RiuQwtPlotCurve*      m_qwtPlotCurve;
    QwtPlotIntervalCurve* m_qwtCurveErrorBars;

    caf::PdmField<bool>    m_showCurve;
    caf::PdmField<QString> m_curveName;
    caf::PdmField<QString> m_customCurveName;
    caf::PdmField<bool>    m_showLegend;
    caf::PdmField<QString> m_legendEntryText;
    caf::PdmField<bool>    m_showErrorBars;
    caf::PdmField<bool>    m_isUsingAutoName;

    caf::PdmChildField<RimPlotCurveAppearance*> m_curveAppearance;

    caf::PdmField<QString>                                    m_symbolLabel_OBSOLETE;
    caf::PdmField<int>                                        m_symbolSize_OBSOLETE;
    caf::PdmField<cvf::Color3f>                               m_curveColor_OBSOLETE;
    caf::PdmField<int>                                        m_curveThickness_OBSOLETE;
    caf::PdmField<float>                                      m_symbolSkipPixelDistance_OBSOLETE;
    caf::PdmField<RimPlotCurveAppearance::PointSymbol>        m_pointSymbol_OBSOLETE;
    caf::PdmField<RimPlotCurveAppearance::LineStyle>          m_lineStyle_OBSOLETE;
    caf::PdmField<RimPlotCurveAppearance::FillStyle>          m_fillStyle_OBSOLETE;
    caf::PdmField<cvf::Color3f>                               m_fillColor_OBSOLETE;
    caf::PdmField<RimPlotCurveAppearance::CurveInterpolation> m_curveInterpolation_OBSOLETE;
    caf::PdmField<RimPlotCurveAppearance::LabelPosition>      m_symbolLabelPosition_OBSOLETE;
    caf::PdmField<cvf::Color3f>                               m_symbolEdgeColor_OBSOLETE;
};