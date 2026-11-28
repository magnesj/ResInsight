#pragma once

#include "cafCmdFeature.h"

//==================================================================================================
/// Opens the dialog for adding, renaming and removing SUMO ensembles.
//==================================================================================================
class RicSumoEnsembleManagementFeature : public caf::CmdFeature
{
    CAF_CMD_HEADER_INIT;

protected:
    bool isCommandEnabled() const override;
    void onActionTriggered( bool isChecked ) override;
    void setupActionLook( QAction* actionToSetup ) override;
};
