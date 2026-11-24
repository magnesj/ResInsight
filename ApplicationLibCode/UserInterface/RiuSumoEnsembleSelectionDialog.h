/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025- Equinor ASA
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

#include "RiuSumoCaseTableModel.h"

#include <QDialog>

#include <map>
#include <set>
#include <vector>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QTableView;
class RiaSumoExplore;

//==================================================================================================
/// Pick ensembles from the cases of an asset. The selection is kept per case, so ensembles of several
/// cases can be picked before Apply. Ensembles already in the project start out selected.
//==================================================================================================
class RiuSumoEnsembleSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    struct Entry
    {
        QString assetName;
        QString caseId;
        QString caseName;
        QString ensembleName;
    };

    RiuSumoEnsembleSelectionDialog( RiaSumoExplore&                                      explore,
                                    const std::set<RiuSumoCaseTableModel::CaseEnsemble>& existing,
                                    QWidget*                                             parent = nullptr );

    std::vector<Entry> entriesToAdd() const;
    std::vector<Entry> entriesToRemove() const;

private slots:
    void assetChanged();
    void currentCaseChanged();
    void ensembleItemChanged( QListWidgetItem* item );
    void setAllEnsembles( bool selected );
    void applyFilters();

private:
    QString            currentCaseId() const;
    std::vector<Entry> toEntries( const std::vector<RiuSumoCaseTableModel::CaseEnsemble>& keys ) const;

private:
    RiaSumoExplore& m_explore;

    RiuSumoCaseTableModel       m_model;
    RiuSumoCaseFilterProxyModel m_proxy;

    QComboBox*   m_assetCombo;
    QCheckBox*   m_officialCheckBox;
    QLineEdit*   m_caseFilter;
    QLineEdit*   m_ensembleFilter;
    QTableView*  m_caseView;
    QListWidget* m_ensembleList;

    // Case id to asset and case name, for every case seen, as the selection outlives the asset being shown
    std::map<QString, std::pair<QString, QString>> m_caseInfo;
    // Ensemble names of cases that did not come with them
    std::map<QString, std::vector<QString>> m_fetchedEnsembleNames;
    bool                                    m_updatingList;
};
