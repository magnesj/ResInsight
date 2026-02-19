/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017-     Statoil ASA
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

#include "ExpressionParserImpl.h"

#include <QRegularExpression>

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
ExpressionParserImpl::ExpressionParserImpl()
{

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<QString> ExpressionParserImpl::detectReferencedVariables(const QString& expression)
{
    std::vector<QString> referencedVariables;
    {
        std::vector<std::string> variable_list;
        exprtk::collect_variables(expression.toStdString(), variable_list);

        std::vector<std::pair<int, QString>> indexAndNamePairs;

        for (const auto& s : variable_list)
        {
            QString variableNameLowerCase = QString::fromStdString(s);

            // ExprTk reports always in lower case

            int index = expression.indexOf(variableNameLowerCase, 0, Qt::CaseInsensitive);
            if (index > -1)
            {
                indexAndNamePairs.push_back(std::make_pair(index, expression.mid(index, variableNameLowerCase.size())));
            }
        }

        // Sort the variable names in the order they first appear in the expression
        std::sort(indexAndNamePairs.begin(), indexAndNamePairs.end());

        for (const auto& indexAndName : indexAndNamePairs)
        {
            referencedVariables.push_back(indexAndName.second);
        }
    }

    return referencedVariables;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void ExpressionParserImpl::assignVector(const QString& variableName, std::vector<double>& vector)
{
    m_symbol_table.add_vector(variableName.toStdString(), vector);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool ExpressionParserImpl::evaluate(const QString& expressionText, QString* errorText)
{
    expression_t expression;

    expression.register_symbol_table(m_symbol_table);

    parser_t parser;
    if (!parser.compile(expressionText.toStdString(), expression))
    {
        if (errorText)
        {
            *errorText = parserErrorText(parser);
        }

        return false;
    }

    // Trigger evaluation
    expression.value();

    if (errorText)
    {
        *errorText = parserErrorText(parser);
    }

    return (parser.error_count() == 0);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString ExpressionParserImpl::parserErrorText(parser_t& parser)
{
    QString txt;

    for (size_t i = 0; i < parser.error_count(); i++)
    {
        auto error = parser.get_error(i);

        QString errorMsg = QString("Position: %1   Type: [%2]   Msg: %3\n")
            .arg(static_cast<int>(error.token.position))
            .arg(exprtk::parser_error::to_str(error.mode).c_str())
            .arg(error.diagnostic.c_str());
        
        txt += errorMsg;
    }

    return txt;
}

//--------------------------------------------------------------------------------------------------
// 
// The parser do not support single-line if statements on a vector.
// Expand a single line if-statement to a for loop over all items in the referenced 
// vectors. The script will add '[i]' as postfix to all variables, assuming all variables
// to be vectors. This statement will be put in a for loop over all elements counting
// up to size of the vector with minimum items.
//
// Single line statement :
//   c := if((a > 13), a, b)
//
// Intended parser script text
//
//    for (var i : = 0; i < min(c[], a[], b[]); i += 1)
//    {
//        c[i] : = if ((a[i] > 13), a[i], b[i]);
//    }
//
// When aggregating functions (min, max, sum, avg) are applied to a single vector variable,
// they compute a scalar result over the whole vector. These must be pre-computed before the
// element-wise for loop, otherwise the variable gets replaced with its indexed version (e.g.
// min(b) -> min(b[i])) which changes the semantics: min(b[i]) returns b[i], not min of all b.
//
// Example with pre-computation:
//   c := if(a > min(b), a, b)
//
// Expanded to:
//   var ri_agg_0 := min(b);
//   for (var i := 0; i < min(c[], a[], b[]); i += 1)
//   {
//       c[i] := if(a[i] > ri_agg_0, a[i], b[i]);
//   }
//
//--------------------------------------------------------------------------------------------------
QString ExpressionParserImpl::expandIfStatements(const QString& expressionText)
{
    auto allVectorVariables = detectReferencedVariables(expressionText);

    QString textWithVectorBrackets = expressionText;
    QString precomputedStatements;

    // Pre-compute aggregating function calls with a single vector variable argument.
    // For example, min(b) should be computed as a scalar before the element-wise loop.
    // Without pre-computation, min(b) would expand to min(b[i]) which just returns b[i].
    static const QStringList aggregatingFunctions = { "min", "max", "sum", "avg" };

    int precomputedIndex = 0;
    for ( const QString& func : aggregatingFunctions )
    {
        for ( const QString& var : allVectorVariables )
        {
            // Match func(var) where var is the sole argument (bounded by word boundaries)
            QString patternStr = QString( "\\b%1\\(\\s*\\b%2\\b\\s*\\)" ).arg( func, var );
            QRegularExpression pattern( patternStr, QRegularExpression::CaseInsensitiveOption );

            if ( pattern.match( textWithVectorBrackets ).hasMatch() )
            {
                // Create a unique scalar variable name for the pre-computed value.
                // Must start with a letter (exprtk requirement). Use ri_agg_N prefix.
                QString preVarName = QString( "ri_agg_%1" ).arg( precomputedIndex++ );
                precomputedStatements += QString( "var %1 := %2(%3);\n" ).arg( preVarName, func, var );
                textWithVectorBrackets.replace( pattern, preVarName );
            }
        }
    }

    QString listOfVars;
    for ( const QString& var : allVectorVariables )
    {
        listOfVars += QString( "%1[]," ).arg( var );

        QString            regexpText = QString( "\\b%1\\b" ).arg( var );
        QRegularExpression regexp( regexpText );

        QString varWithBrackets = var + "[i]";
        textWithVectorBrackets  = textWithVectorBrackets.replace( regexp, varWithBrackets );
    }

    listOfVars = listOfVars.left( listOfVars.size() - 1 );

    QString expandedText = precomputedStatements;
    expandedText += QString( "for (var i := 0; i < min(%1); i += 1)\n" ).arg( listOfVars );
    expandedText += "{\n";
    expandedText += QString( "    %1;\n" ).arg( textWithVectorBrackets );
    expandedText += "}\n";

    return expandedText;
}
