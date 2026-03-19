#pragma once

#include "ColorTriplet.h"

#include "cafFilePath.h"
#include "cafPdmChildArrayField.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"
#include "cafPdmProxyValueField.h"

#include <QString>

class SmallDemoPdmObject : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    SmallDemoPdmObject();

    caf::PdmField<double>  m_doubleField;
    caf::PdmField<int>     m_intField;
    caf::PdmField<QString> m_textField;

    caf::PdmField<std::pair<bool, double>>  m_booleanAndDoubleField;
    caf::PdmField<std::pair<bool, QString>> m_booleanAndTextField;

    caf::PdmChildArrayField<ColorTriplet*> m_colorTriplets;

    caf::PdmProxyValueField<double>           m_proxyDoubleField;
    caf::PdmField<caf::FilePath>              m_fileName;
    caf::PdmField<std::vector<caf::FilePath>> m_fileNameList;

    caf::PdmField<std::vector<QString>> m_multiSelectList;

    caf::PdmField<bool>  m_toggleField;
    caf::PdmFieldHandle* objectToggleField() override { return &m_toggleField; }

    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override
    {
        if ( changedField == &m_toggleField )
        {
            std::cout << "Toggle Field changed" << std::endl;
        }
    }

    void setDoubleMember( const double& d )
    {
        m_doubleMember = d;
        std::cout << "setDoubleMember" << std::endl;
    }
    double doubleMember() const
    {
        std::cout << "doubleMember" << std::endl;
        return m_doubleMember;
    }

    //--------------------------------------------------------------------------------------------------
    ///
    //--------------------------------------------------------------------------------------------------
    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;

    //--------------------------------------------------------------------------------------------------
    ///
    //--------------------------------------------------------------------------------------------------
    void defineCustomContextMenu( const caf::PdmFieldHandle* fieldNeedingMenu, QMenu* menu, QWidget* fieldEditorWidget ) override
    {
        menu->addAction( "test" );
        menu->addAction( "other test <<>>" );
    }

private:
    double m_doubleMember;

protected:
    //--------------------------------------------------------------------------------------------------
    ///
    //--------------------------------------------------------------------------------------------------
    void defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;
};
