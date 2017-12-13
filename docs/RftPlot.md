---
layout: docs
title: RFT Plot
permalink: /docs/rftplot/
published: true
---

An RFT (_Repeated Formation Tester_) plot is a special well plot for displaying observed formation pressure and simulated formation pressure data, and for comparing those data. The curves are plotted as pressure against true vertical depth (TVD). Simulated pressure data may be a part of the grid model (\*.rft) and observed pressure data are loaded from well log files (\*.las).

![]({{site.baseurl}}/images/RftPlot.png)

## Create New RFT Plot
There are several ways to create new RFT Plots.

**From the Plot Object Project Tree**
- Select context command **New RFT Plot** for _Well Path_ node or _RFT Plots_ node.

**From the Project Tree**
- Select context command **New RFT plot** for a simulation well.

**From the 3D view**
- Right-click a simulation well select **Well Plots -> New RFT Plot**.

## Plotting

.....


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
After a well has been selected in the Well Name field, sources for that well should appear in the sources field. The different sources are grouped in three different groups:
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

Each time step is postfixed by an indication of which source type(s) the time step is belonging to. This indication is displayed as one or more letters within square brackets. Examples: **[ O ]**, **[ R G ]**.
- **O** -- Indicates that the current time step is from observed data
- **R** -- Indicates that the current time step is from RFT data
- **G** -- Indicates that the current time step is from Grid data

More than one letter for one single time step, means that the time steps is from multiple case types.

### Zonation/Formation Names
This property editor lets the user control how formations are handled. This is what it looks like in the RFT plot context.

![]({{site.baseurl}}/images/RftPltFormationNames.png)

Please see the [full documentation]({{site.baseurl}}/docs/formations) on the formations property editor for details.

<div class="note">
  When the formation names property editor is used in the context of RFT plots, the fields <b>Trajectory</b> and <b>Simulation Well</b> are hidden because those values are given by the RFT plot definition.
</div>

### Legend and Axis
![]({{site.baseurl}}/images/RftLegendAndAxis.png)

This property editor lets the user control visual properties for the legend and axis.
- **Show Legends** -- Toggle on/off legends in plot
- Visible X Axis Range
  - **Auto Scale** -- Automatically set X axis range based on data values
  - **Logarithmic Scale** -- Toggle between linear and logarithmic X axis
  - **Min** -- Set X axis minimum value
  - **Max** -- Set X axis maximum value
- Visible Depth Range
  - **Auto Scale** -- Automatically set depth axis range based on data values
  - **Min** -- Set depth axis minimum value
  - **Max** -- Set depth axis maximum value
