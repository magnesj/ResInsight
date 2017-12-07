---
layout: docs
title: RFT Plot
permalink: /docs/rftplot/
published: true
---

An RFT plot is a special well plot displaying pressure data against true vertical depth (TVD). RFT data may be a part of the grid model or may be loaded from well log files (\*.LAS) and well path files (\*.dev, \*.json etc.).

![]({{site.baseurl}}/images/Dummy.png)

## Create New RFT Plot
There are several ways to create new RFT Plots
- Right-click a Well Path under Wells in the project tree and then select **New RFT Plot**.
- Right-click a simulation well in the 3D view and select **Well Plots -> New RFT Plot**.
- Right-click the RFT Plots node in the project tree and select **New RFT Plot**.

## Constraints
If an imported well log file does not contain pressure data, that data source (_Observed data_) will not be visible in the sources field in the property editor. Pressure data in a well log file is expected to have a column named _PRESSURE_ or _PRES_FORM_. If the well log contains pressure data, but has no infomation about TVD, a warning will appear when a curve is to be displayed. TVD may be available either as an explicit TVD column named _TVDMSL_ in the well log file or a separately imported well path file.

<p align="center">
  <img src="{{site.baseurl}}/images/NoTvdWarningDialog.png"/><br/>
  No TVD warning dialog
</p>

## Property Editor
The property editor lets the user select which curves to display in the RFT plot.

<p align="center">
  <img src="{{site.baseurl}}/images/RftPlotPropertyEditor.png"/><br/>
  RFT plot property editor
</p>

### Well Name
An RFT plot is always related to a well. The **Well Name** field contains the selected well. If the RFT plot was created from a simulation well or a well path, the correct well name is initially selected. If no well is selected, the user must select a well to be able to proceed.

There are two types of wells in the well name field:
- Imported physical well paths. These are postfixed by '(Well Path)'.
- Simulation wells without a connected physical well path

### Sources
After a well is selected in the Well Name field, sources for that well should appear in the sources field. The different sources are grouped in three different groups:
- **RFT File Cases** -- Cases imported from \*.rft file(s) included in the simulation output case. (The keyword _WRFTPLT_ was used in the schedule file during simulation output generation)
- **Grid Cases** -- Simulation cases
- **Observed Cases** -- Observed data imported from well log files and well path files

When the user selects a source, time steps for that source appears in the **Time Steps** field.

### Time Steps
The **Time Steps** field contains available time steps for the selected source(s). Some combinations of selected sources may display a filtered list of time steps instead of the union of all time steps for all selected sources. The policy is as follows:
1. **Exclusively grid cases selected**. All available time steps for the selected grid cases are displayed.
2. **Grid case(s) and observed data case selected**. Time steps shown are
  - The time steps from the observed case
  - The first time step from the merged time step list from all grid cases
  - If no time steps from grid cases match the observed time step, display the two adjacent grid case time steps.
3. **Grid case(s) and RFT File case(s) selected**. Same display logic as point 2
4. **All types of cases selected**. Same display logic as point 2 with the exception that RFT File case time steps are treated as grid time steps.

### Zonation/Formation Names
![]({{site.baseurl}}/images/RftPltFormationNames.png)

This field lets the user control whether to display formation names or not, and where to load the names from.
If **Show Formations** is checked, formations are loaded from the specified formation source. The options are as follows.
- **Case** -- Load formations from the case specified in the **Formation Case** field
- **Well Pick** -- Load formations from the selected well in the **Well Name** field.

### Legend and Axis
![]({{site.baseurl}}/images/RftPltLegendAndAxis.png)


