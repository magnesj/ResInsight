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
class QPushButton;
class QTableView;
class QToolButton;
class RiaSumoConnector;
class RiaSumoExploreCache;

//==================================================================================================
/// Pick ensembles from the cases of an asset. The selection is kept per case, so ensembles of several
/// cases can be picked before Apply. Ensembles already in the project start out selected.
///
/// What Sumo holds is requested asynchronously, so the dialog stays responsive while the lists load.
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

    RiuSumoEnsembleSelectionDialog( RiaSumoConnector&                                    connector,
                                    RiaSumoExploreCache&                                 cache,
                                    const std::set<RiuSumoCaseTableModel::CaseEnsemble>& existing,
                                    QWidget*                                             parent = nullptr );
    ~RiuSumoEnsembleSelectionDialog() override;

    bool eventFilter( QObject* watched, QEvent* event ) override;

    std::vector<Entry> entriesToAdd() const;
    std::vector<Entry> entriesToRemove() const;

private slots:
    void assetChanged();
    void currentCaseChanged();
    void ensembleItemChanged( QListWidgetItem* item );
    void setAllEnsembles( bool selected );
    void applyFilters();
    void refresh();

private:
    void               loadAssets();
    void               loadCases( bool forceRefresh );
    void               showCases( const QString& assetName );
    void               updateStatus();
    void               createFilterRow();
    void               layoutFilterRow();
    void               updateValueFilters();
    void               applyColumnFilters();
    QString            currentCaseId() const;
    std::vector<Entry> toEntries( const std::vector<RiuSumoCaseTableModel::CaseEnsemble>& keys ) const;

private:
    RiaSumoConnector&    m_connector;
    RiaSumoExploreCache& m_cache;

    RiuSumoCaseTableModel       m_model;
    RiuSumoCaseFilterProxyModel m_proxy;

    QComboBox*   m_assetCombo;
    QCheckBox*   m_myCasesCheckBox;
    QCheckBox*   m_officialCheckBox;
    QPushButton* m_refreshButton;
    QLabel*      m_statusLabel;
    QLineEdit*   m_ensembleFilter;
    QWidget*     m_filterRow;
    QTableView*  m_caseView;
    QListWidget* m_ensembleList;

    // Case id to asset and case name, for every case seen, as the selection outlives the asset being shown
    std::map<QString, std::pair<QString, QString>> m_caseInfo;

    // One filter widget per column below the header: line edits for text, menu buttons for values
    std::map<int, QLineEdit*>        m_textFilters;
    std::map<int, QToolButton*>      m_valueFilters;
    std::map<int, std::set<QString>> m_checkedValues;

    int  m_pendingRequests;
    bool m_updatingList;
};
