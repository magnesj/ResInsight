/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2019-     Equinor ASA
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
#include "RiuQwtPlotTools.h"

#include "RiaApplication.h"
#include "RiaColorTools.h"
#include "RiaPreferences.h"
#include "RiaQDateTimeTools.h"
#include "RiuPlotAxis.h"

#include "RimPlotCurve.h"

#include "Summary/RiaSummaryAddressAnalyzer.h"

#include "RiuGuiTheme.h"
#include "RiuPlotCurveSymbolImageCreator.h"
#include "RiuQwtPlotCurve.h"
#include "RiuQwtPlotLegend.h"

#include "qwt_axis.h"
#include "qwt_date_scale_draw.h"
#include "qwt_date_scale_engine.h"
#include "qwt_graphic.h"
#include "qwt_painter.h"
#include "qwt_plot.h"
#include "qwt_plot_grid.h"
#include "qwt_plot_item.h"
#include "qwt_plot_layout.h"
#include "qwt_plot_shapeitem.h"
#include "qwt_scale_widget.h"
#include "qwt_text.h"

#include <QCryptographicHash>
#include <QImage>
#include <QSet>

#include <map>
#include <optional>
#include <set>
#include <vector>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuQwtPlotTools::setCommonPlotBehaviour( QwtPlot* plot )
{
    // Plot background and frame look

    QPalette newPalette( plot->palette() );
    newPalette.setColor( QPalette::Window, Qt::white );
    plot->setPalette( newPalette );

    plot->setAutoFillBackground( true );
    plot->setCanvasBackground( Qt::white );
    plot->plotLayout()->setCanvasMargin( 0, -1 );

    QFrame* canvasFrame = dynamic_cast<QFrame*>( plot->canvas() );
    canvasFrame->setFrameShape( QFrame::Box );

    // Grid
    QwtPlotGrid* grid = new QwtPlotGrid;
    grid->attach( plot );
    QPen gridPen( Qt::SolidLine );
    grid->setPen( gridPen );
    RiuGuiTheme::styleQwtItem( grid );

    // Axis number font
    int axisFontSize =
        caf::FontTools::absolutePointSize( RiaPreferences::current()->defaultPlotFontSize(), caf::FontTools::RelativeSize::Medium );
    QFont axisFont = plot->axisFont( QwtAxis::XBottom );
    axisFont.setPointSize( axisFontSize );

    plot->setAxisFont( QwtAxis::XBottom, axisFont );
    plot->setAxisFont( QwtAxis::XTop, axisFont );
    plot->setAxisFont( QwtAxis::YLeft, axisFont );
    plot->setAxisFont( QwtAxis::YRight, axisFont );

    // Axis title font
    std::vector<QwtAxis::Position> axes = { QwtAxis::XBottom, QwtAxis::XTop, QwtAxis::YLeft, QwtAxis::YRight };

    for ( QwtAxis::Position axis : axes )
    {
        QwtText axisTitle     = plot->axisTitle( axis );
        QFont   axisTitleFont = axisTitle.font();
        axisTitleFont.setPointSize( axisFontSize );
        axisTitleFont.setBold( false );
        axisTitle.setFont( axisTitleFont );
        axisTitle.setRenderFlags( Qt::AlignRight );

        plot->setAxisTitle( axis, axisTitle );
    }

    // Set a focus policy to allow it taking key press events.
    // This is not strictly necessary since this widget inherit QwtPlot
    // which already has a focus policy.
    // However, for completeness we still do it here.
    plot->setFocusPolicy( Qt::WheelFocus );

    // Enable mousetracking and event filter
    plot->canvas()->setMouseTracking( true );
    plot->plotLayout()->setAlignCanvasToScales( true );

    plot->setContentsMargins( 1, 1, 1, 1 );

    // Store the pointer address as an object name. This way each plot can be identified uniquely for CSS-stylesheets
    QString objectName = QString( "%1" ).arg( reinterpret_cast<uint64_t>( plot ) );
    plot->setObjectName( objectName );

    QString canvasName = QString( "%1" ).arg( reinterpret_cast<uint64_t>( plot->canvas() ) );
    plot->canvas()->setObjectName( canvasName );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuQwtPlotTools::setDefaultAxes( QwtPlot* plot )
{
    plot->setAxesCount( QwtAxis::XBottom, 1 );
    plot->setAxesCount( QwtAxis::YLeft, 1 );

    plot->axisWidget( QwtAxis::XBottom )->setMargin( 0 );
    plot->axisWidget( QwtAxis::YLeft )->setMargin( 0 );
    plot->axisWidget( QwtAxis::XTop )->setMargin( 0 );
    plot->axisWidget( QwtAxis::YRight )->setMargin( 0 );

    plot->setAxisMaxMinor( QwtAxis::XBottom, 2 );
    plot->setAxisMaxMinor( QwtAxis::YLeft, 3 );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuQwtPlotTools::enableDateBasedBottomXAxis( QwtPlot*                         plot,
                                                  const QString&                   dateFormat,
                                                  const QString&                   timeFormat,
                                                  RiaDefines::DateFormatComponents dateComponents,
                                                  RiaDefines::TimeFormatComponents timeComponents )
{
    QwtDateScaleDraw* scaleDraw = new QwtDateScaleDraw( Qt::UTC );

    std::set<QwtDate::IntervalType> intervals =
        { QwtDate::Year, QwtDate::Month, QwtDate::Week, QwtDate::Day, QwtDate::Hour, QwtDate::Minute, QwtDate::Second, QwtDate::Millisecond };

    for ( QwtDate::IntervalType interval : intervals )
    {
        scaleDraw->setDateFormat( interval, dateTimeFormatForInterval( interval, dateFormat, timeFormat, dateComponents, timeComponents ) );
    }

    QwtDateScaleEngine* scaleEngine = new QwtDateScaleEngine( Qt::UTC );
    plot->setAxisScaleEngine( QwtAxis::XBottom, scaleEngine );
    plot->setAxisScaleDraw( QwtAxis::XBottom, scaleDraw );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiuQwtPlotTools::dateTimeFormatForInterval( QwtDate::IntervalType            interval,
                                                    const QString&                   dateFormat,
                                                    const QString&                   timeFormat,
                                                    RiaDefines::DateFormatComponents dateComponents,
                                                    RiaDefines::TimeFormatComponents timeComponents )
{
    if ( dateComponents != RiaDefines::DateFormatComponents::DATE_FORMAT_UNSPECIFIED &&
         timeComponents != RiaDefines::TimeFormatComponents::TIME_FORMAT_UNSPECIFIED )
    {
        return RiaQDateTimeTools::timeFormatString( timeFormat, timeComponents ) + "\n" +
               RiaQDateTimeTools::dateFormatString( dateFormat, dateComponents );
    }
    else
    {
        switch ( interval )
        {
            case QwtDate::Millisecond:
                return RiaQDateTimeTools::timeFormatString( timeFormat,
                                                            RiaDefines::TimeFormatComponents::TIME_FORMAT_HOUR_MINUTE_SECOND_MILLISECOND );
            case QwtDate::Second:
                return RiaQDateTimeTools::timeFormatString( timeFormat, RiaDefines::TimeFormatComponents::TIME_FORMAT_HOUR_MINUTE_SECOND );
            case QwtDate::Minute:
            {
                QString fullFormat =
                    RiaQDateTimeTools::timeFormatString( timeFormat, RiaDefines::TimeFormatComponents::TIME_FORMAT_HOUR_MINUTE );
                fullFormat += "\n";
                fullFormat += RiaQDateTimeTools::dateFormatString( dateFormat, RiaDefines::DateFormatComponents::DATE_FORMAT_YEAR_MONTH_DAY );
                return fullFormat;
            }
            case QwtDate::Hour:
            {
                QString fullFormat = RiaQDateTimeTools::timeFormatString( timeFormat, RiaDefines::TimeFormatComponents::TIME_FORMAT_HOUR );
                if ( !fullFormat.endsWith( "AP" ) )
                {
                    fullFormat += ":00";
                }
                fullFormat += "\n";
                fullFormat += RiaQDateTimeTools::dateFormatString( dateFormat, RiaDefines::DateFormatComponents::DATE_FORMAT_YEAR_MONTH_DAY );
                return fullFormat;
            }
            case QwtDate::Day:
                return RiaQDateTimeTools::dateFormatString( dateFormat, RiaDefines::DateFormatComponents::DATE_FORMAT_YEAR_MONTH_DAY );
            case QwtDate::Week:
                return RiaQDateTimeTools::dateFormatString( dateFormat, RiaDefines::DateFormatComponents::DATE_FORMAT_YEAR_MONTH );
            case QwtDate::Month:
                return RiaQDateTimeTools::dateFormatString( dateFormat, RiaDefines::DateFormatComponents::DATE_FORMAT_YEAR_MONTH );
            case QwtDate::Year:
                return RiaQDateTimeTools::dateFormatString( dateFormat, RiaDefines::DateFormatComponents::DATE_FORMAT_YEAR );
            default:
                return RiaQDateTimeTools::dateFormatString( dateFormat, RiaDefines::DateFormatComponents::DATE_FORMAT_YEAR_MONTH_DAY );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QwtPlotShapeItem*
    RiuQwtPlotTools::createBoxShape( const QString& label, double startX, double endX, double startY, double endY, QColor color, Qt::BrushStyle brushStyle )
{
    return createBoxShapeT<QwtPlotShapeItem>( label, startX, endX, startY, endY, color, brushStyle );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QwtAxis::Position RiuQwtPlotTools::toQwtPlotAxisEnum( RiaDefines::PlotAxis riaPlotAxis )
{
    if ( riaPlotAxis == RiaDefines::PlotAxis::PLOT_AXIS_LEFT )
        return QwtAxis::YLeft;
    else if ( riaPlotAxis == RiaDefines::PlotAxis::PLOT_AXIS_RIGHT )
        return QwtAxis::YRight;
    else if ( riaPlotAxis == RiaDefines::PlotAxis::PLOT_AXIS_BOTTOM )
        return QwtAxis::XBottom;
    else if ( riaPlotAxis == RiaDefines::PlotAxis::PLOT_AXIS_TOP )
        return QwtAxis::XTop;

    return QwtAxis::YLeft;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaDefines::PlotAxis RiuQwtPlotTools::fromQwtPlotAxis( QwtAxis::Position axis )
{
    if ( axis == QwtAxis::YLeft )
        return RiaDefines::PlotAxis::PLOT_AXIS_LEFT;
    else if ( axis == QwtAxis::YRight )
        return RiaDefines::PlotAxis::PLOT_AXIS_RIGHT;
    else if ( axis == QwtAxis::XBottom )
        return RiaDefines::PlotAxis::PLOT_AXIS_BOTTOM;
    else if ( axis == QwtAxis::XTop )
        return RiaDefines::PlotAxis::PLOT_AXIS_TOP;

    return RiaDefines::PlotAxis::PLOT_AXIS_LEFT;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuQwtPlotTools::updateLegendData( RiuQwtPlotLegend* legend, const std::vector<RimPlotCurve*>& curves )
{
    QList<QwtLegendData> legendDataList = createLegendData( curves );

    legend->updateLegend( QVariant(), legendDataList );
}

//--------------------------------------------------------------------------------------------------
/// Collect legend data, one entry per legend item, from the plots. Items with identical title and icon are merged.
//--------------------------------------------------------------------------------------------------
QList<QList<QwtLegendData>> RiuQwtPlotTools::uniqueLegendData( const std::vector<QwtPlot*>& plots )
{
    QList<QList<QwtLegendData>> uniqueData;
    QSet<QString>               keys;

    for ( QwtPlot* plot : plots )
    {
        if ( !plot ) continue;

        for ( QwtPlotItem* item : plot->itemList() )
        {
            if ( !item->testItemAttribute( QwtPlotItem::Legend ) ) continue;

            QList<QwtLegendData> legendData = item->legendData();
            if ( legendData.isEmpty() ) continue;

            QString title = legendData.first().value( QwtLegendData::TitleRole ).value<QwtText>().text();

            // Include the icon in the key, so entries with the same title but different color or style are all kept
            QString key  = title;
            auto    icon = legendData.first().value( QwtLegendData::IconRole ).value<QwtGraphic>();
            if ( !icon.isNull() )
            {
                QImage     image = icon.toImage( QSize( 32, 16 ).expandedTo( icon.defaultSize().toSize() ) );
                QByteArray bytes( reinterpret_cast<const char*>( image.constBits() ), image.sizeInBytes() );
                key += "|" + QString::fromLatin1( QCryptographicHash::hash( bytes, QCryptographicHash::Md5 ).toHex() );
            }
            if ( keys.contains( key ) ) continue;

            keys.insert( key );
            uniqueData.append( legendData );
        }
    }

    return uniqueData;
}

//--------------------------------------------------------------------------------------------------
/// Draw a legend icon consisting of an optional horizontal line and/or an optional symbol.
//--------------------------------------------------------------------------------------------------
static QwtGraphic createCurveLegendIcon( const QSize&                        size,
                                         bool                                drawLine,
                                         const QColor&                       lineColor,
                                         RiuPlotCurveSymbol::PointSymbolEnum symbol,
                                         const QColor&                       symbolColor )
{
    QwtGraphic graphic;
    graphic.setDefaultSize( size );
    graphic.setRenderHint( QwtGraphic::RenderPensUnscaled, true );

    QPainter painter( &graphic );
    painter.setRenderHint( QPainter::Antialiasing );

    if ( drawLine )
    {
        QPen pn;
        pn.setCapStyle( Qt::FlatCap );
        pn.setColor( lineColor );

        painter.setPen( pn );

        const double y = 0.5 * size.height();
        QwtPainter::drawLine( &painter, 0.0, y, size.width(), y );
    }

    if ( symbol != RiuPlotCurveSymbol::SYMBOL_NONE )
    {
        auto image = RiuPlotCurveSymbolImageCreator::createSymbolImage( symbol, QSize( size.width() / 2, size.height() / 2 ), symbolColor );

        QPoint p( size.width() / 4, size.height() / 4 );
        painter.drawImage( p, image );
    }

    return graphic;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<QwtLegendData> RiuQwtPlotTools::createLegendData( const std::vector<RimPlotCurve*>& curves )
{
    QList<QwtLegendData> legendDataList;

    for ( auto c : curves )
    {
        QwtLegendData test;
        test.setValue( QwtLegendData::Role::TitleRole, c->curveName() );

        c->updateUiIconFromPlotSymbol();
        auto icon = c->uiIcon();
        auto size = icon->availableSizes().first();
        // see QwtPlotCurve::legendIcon

        QColor     curveColor = RiaColorTools::toQColor( c->color() );
        QwtGraphic graphic    = createCurveLegendIcon( size, /*drawLine=*/true, curveColor, c->symbol(), curveColor );

        QVariant v = QVariant::fromValue( graphic );
        test.setValue( QwtLegendData::Role::IconRole, v );

        legendDataList.push_back( test );
    }

    return legendDataList;
}

namespace
{
// A curve can be grouped/labeled by any of these attributes. VECTOR is the summary vector name (e.g.
// WOPR, WBHP), as curves are often colored by vector and symbolized by case, or vice versa.
enum class SummaryLegendAttribute
{
    CASE,
    WELL,
    VECTOR
};

struct SummaryLegendCurveInfo
{
    cvf::Color3f                        color;
    RiuPlotCurveSymbol::PointSymbolEnum symbol;
    QString                             caseName;
    RifEclipseSummaryAddress            address;
};

//--------------------------------------------------------------------------------------------------
/// Returns the common value of the given attribute across all curves in the group, or an empty optional
/// if the curves do not all share the same value (i.e. the attribute is not constant within the group).
//--------------------------------------------------------------------------------------------------
std::optional<QString> groupLabelForAttribute( SummaryLegendAttribute attribute, const std::vector<SummaryLegendCurveInfo>& group )
{
    if ( attribute == SummaryLegendAttribute::CASE )
    {
        std::set<QString> names;
        for ( const auto& curveInfo : group )
            names.insert( curveInfo.caseName );
        if ( names.size() != 1 ) return std::nullopt;
        return *names.begin();
    }

    std::vector<RifEclipseSummaryAddress> addresses;
    addresses.reserve( group.size() );
    for ( const auto& curveInfo : group )
        addresses.push_back( curveInfo.address );

    RiaSummaryAddressAnalyzer analyzer;
    analyzer.appendAddresses( addresses );

    if ( attribute == SummaryLegendAttribute::WELL )
    {
        auto names = analyzer.wellNames();
        if ( names.size() != 1 ) return std::nullopt;
        return QString::fromStdString( *names.begin() );
    }

    auto quantities = analyzer.quantities();
    if ( quantities.size() != 1 ) return std::nullopt;
    return QString::fromStdString( quantities.front() );
}

//--------------------------------------------------------------------------------------------------
/// Returns a label per group for the given attribute, if that attribute is constant within every group
/// (see groupLabelForAttribute()) and also discriminates between groups (different groups get different
/// labels). Without the latter requirement, an attribute that happens to be constant across ALL curves
/// (e.g. a single well shown in one sub plot) would be accepted and produce a useless legend with the
/// same label repeated for every color/symbol.
//--------------------------------------------------------------------------------------------------
template <typename GroupKey>
std::optional<std::map<GroupKey, QString>>
    labelsIfDiscriminating( SummaryLegendAttribute attribute, const std::map<GroupKey, std::vector<SummaryLegendCurveInfo>>& groups )
{
    std::map<GroupKey, QString> labels;
    std::set<QString>           distinctValues;
    for ( const auto& [group, curveInfos] : groups )
    {
        auto label = groupLabelForAttribute( attribute, curveInfos );
        if ( !label ) return std::nullopt;

        labels[group] = *label;
        distinctValues.insert( *label );
    }

    if ( distinctValues.size() != groups.size() ) return std::nullopt;
    return labels;
}
} // namespace

//--------------------------------------------------------------------------------------------------
/// Try to split the combined (color + symbol) legend into two separate, smaller legends: one showing
/// the distinct curve colors and one showing the distinct curve symbols.
///
/// This is useful when curves vary along two independent dimensions (e.g. one color per source case
/// and one symbol per well), as a single merged legend would otherwise require one entry per
/// combination (i.e. case count x well count) instead of (case count + well count).
///
/// Returns false, leaving the output lists untouched, if the curves do not have summary address
/// metadata attached (see RiuPlotCurve::setSummaryAddressMetaData()), or if the colors and symbols
/// cannot be clearly attributed to two independent dimensions (summary case, well or vector name). The
/// caller should fall back to a single merged legend in that case.
//--------------------------------------------------------------------------------------------------
bool RiuQwtPlotTools::splitLegendByColorAndSymbol( const std::vector<QwtPlot*>& plots,
                                                   QList<QwtLegendData>&        colorLegendData,
                                                   QList<QwtLegendData>&        symbolLegendData )
{
    std::map<cvf::Color3f, std::vector<SummaryLegendCurveInfo>>                        curvesByColor;
    std::map<RiuPlotCurveSymbol::PointSymbolEnum, std::vector<SummaryLegendCurveInfo>> curvesBySymbol;

    for ( QwtPlot* plot : plots )
    {
        if ( !plot ) continue;

        for ( QwtPlotItem* item : plot->itemList() )
        {
            if ( !item->testItemAttribute( QwtPlotItem::Legend ) ) continue;

            auto plotCurve = dynamic_cast<RiuPlotCurve*>( item );
            if ( !plotCurve || !plotCurve->hasSummaryAddressMetaData() ) return false;

            auto ownerCurve = plotCurve->ownerRimCurve();
            if ( !ownerCurve ) return false;

            SummaryLegendCurveInfo curveInfo;
            curveInfo.color    = ownerCurve->color();
            curveInfo.symbol   = ownerCurve->symbol();
            curveInfo.caseName = plotCurve->summaryCaseNameMetaData();
            curveInfo.address  = plotCurve->summaryAddressMetaData();

            curvesByColor[curveInfo.color].push_back( curveInfo );
            curvesBySymbol[curveInfo.symbol].push_back( curveInfo );
        }
    }

    if ( curvesByColor.size() < 2 || curvesBySymbol.size() < 2 ) return false;

    const std::vector<SummaryLegendAttribute> allAttributes = { SummaryLegendAttribute::CASE,
                                                                SummaryLegendAttribute::WELL,
                                                                SummaryLegendAttribute::VECTOR };

    std::map<SummaryLegendAttribute, std::optional<std::map<cvf::Color3f, QString>>>                        colorLabelsByAttribute;
    std::map<SummaryLegendAttribute, std::optional<std::map<RiuPlotCurveSymbol::PointSymbolEnum, QString>>> symbolLabelsByAttribute;
    for ( auto attribute : allAttributes )
    {
        colorLabelsByAttribute[attribute]  = labelsIfDiscriminating( attribute, curvesByColor );
        symbolLabelsByAttribute[attribute] = labelsIfDiscriminating( attribute, curvesBySymbol );
    }

    // Try the most common combinations first: one color per case with one symbol per well, falling back
    // to other combinations (e.g. one color per vector with one symbol per case) if that is what the
    // curves actually use.
    const std::vector<std::pair<SummaryLegendAttribute, SummaryLegendAttribute>> preferredAttributePairs = {
        { SummaryLegendAttribute::CASE, SummaryLegendAttribute::WELL },
        { SummaryLegendAttribute::WELL, SummaryLegendAttribute::CASE },
        { SummaryLegendAttribute::VECTOR, SummaryLegendAttribute::CASE },
        { SummaryLegendAttribute::CASE, SummaryLegendAttribute::VECTOR },
        { SummaryLegendAttribute::VECTOR, SummaryLegendAttribute::WELL },
        { SummaryLegendAttribute::WELL, SummaryLegendAttribute::VECTOR },
    };

    std::map<cvf::Color3f, QString>                        colorToLabel;
    std::map<RiuPlotCurveSymbol::PointSymbolEnum, QString> symbolToLabel;
    bool                                                   foundAttributes = false;
    for ( const auto& [colorAttribute, symbolAttribute] : preferredAttributePairs )
    {
        if ( colorLabelsByAttribute[colorAttribute] && symbolLabelsByAttribute[symbolAttribute] )
        {
            colorToLabel    = *colorLabelsByAttribute[colorAttribute];
            symbolToLabel   = *symbolLabelsByAttribute[symbolAttribute];
            foundAttributes = true;
            break;
        }
    }

    if ( !foundAttributes ) return false;

    // Use the same square icon size as the regular per-curve legend icons (see RimPlotCurve::updateUiIconFromPlotSymbol()),
    // as symbols are drawn at half this size (see createCurveLegendIcon()) and become an unrecognizable blob if the icon
    // is too small or not square.
    const QSize iconSize( 24, 24 );

    for ( const auto& [color, label] : colorToLabel )
    {
        QColor        curveColor = RiaColorTools::toQColor( color );
        QwtLegendData entry;
        entry.setValue( QwtLegendData::Role::TitleRole, label );
        entry.setValue( QwtLegendData::Role::IconRole,
                        QVariant::fromValue(
                            createCurveLegendIcon( iconSize, /*drawLine=*/true, curveColor, RiuPlotCurveSymbol::SYMBOL_NONE, curveColor ) ) );
        colorLegendData.push_back( entry );
    }

    // Use a neutral color for the symbol-only legend, as color is already covered by the color legend.
    const QColor neutralSymbolColor( 60, 60, 60 );

    for ( const auto& [symbol, label] : symbolToLabel )
    {
        QwtLegendData entry;
        entry.setValue( QwtLegendData::Role::TitleRole, label );
        entry.setValue( QwtLegendData::Role::IconRole,
                        QVariant::fromValue(
                            createCurveLegendIcon( iconSize, /*drawLine=*/false, neutralSymbolColor, symbol, neutralSymbolColor ) ) );
        symbolLegendData.push_back( entry );
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuQwtPlotTools::enableGridLines( QwtPlot* plot, QwtAxis::Position axis, bool enableMajor, bool enableMinor )
{
    // NB: Make a copy, not reference to avoid iterator invalidation
    QwtPlotItemList plotItems = plot->itemList( QwtPlotItem::Rtti_PlotGrid );
    for ( QwtPlotItem* plotItem : plotItems )
    {
        auto* grid = static_cast<QwtPlotGrid*>( plotItem );
        if ( axis == QwtAxis::XTop || axis == QwtAxis::XBottom )
        {
            grid->setXAxis( axis );
            grid->enableX( enableMajor );
            grid->enableXMin( enableMinor );
        }
        else
        {
            grid->setYAxis( axis );
            grid->enableY( enableMajor );
            grid->enableYMin( enableMinor );
        }
        grid->setMajorPen( Qt::lightGray, 1.0, Qt::SolidLine );
        grid->setMinorPen( Qt::lightGray, 1.0, Qt::DashLine );
    }
}
