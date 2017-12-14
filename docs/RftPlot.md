---
layout: docs
title: RFT Plot
permalink: /docs/rftplot/
published: true
---

An RFT (_Repeated Formation Tester_) plot is a special well plot for displaying observed formation pressure and simulated formation pressure, and for comparing those. The curves are plotted as pressure against true vertical depth (TVD). Simulated pressure data may be a part of the grid model (\*.rft) and observed pressure data are loaded from well log files (\*.las).

![]({{site.baseurl}}/images/RftPlot.png)

## Create New RFT Plot
There are several ways to create new RFT Plots.

**From the Plot Object Project Tree**
- Select context command **New RFT Plot** for _Well Path_ node or _RFT Plots_ node.

**From the Project Tree**
- Select context command **New RFT plot** for a simulation well.

**From the 3D view**
- Right-click a simulation well select **Well Plots -> New RFT Plot**.

## Plot Observed Data
To be able to plot observed pressure data for a well in an RFT plot, at least one well log file from that well (e.g. \*.las) have to be imported to ResInsight. This file must contain a pressure column, which must have the name _PRESSURE_ or _PRES_FORM_. If the well log file itself does not contain a TVD column (named _TVDMSL_), a well path file (See [Well Trajectories]({{site.baseurl}}/docs/wellpaths)) for the same well must also be imported to ResInsight.

If no TVD data for a well is found when the user tries to plot a curve, ResInsight will present a warning dialog to the user.

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
Select the wanted well to display in the plot. Wells postfixed by _'(Well Path)'_ have associated observed data in addition to simulated data.

### Sources
After a well has been selected in the Well Name field, sources for that well should appear in the sources field. The sources are placed in one of three different groups:
- **RFT File Cases** -- Simulation cases may have associated formation pressure data in _\*.rft_ file(s). If the simulation case contains such files, those are imported together with the simulation case (See the keyword `WRFTPLT` in the Eclipse manual for information)
- **Grid Cases** -- Simulation cases
- **Observed Cases** -- Observed data imported from well log files

When the user selects a source, time steps for that source appears in the **Time Steps** field.

### Time Steps
The **Time Steps** field contains available time steps for the selected source(s). Some combinations of selected sources may display a filtered list of time steps instead of the union of all time steps for all selected sources. The policy is as follows:
1. **Exclusively grid cases selected**. All available time steps for the selected grid cases are displayed.
2. **Grid case(s) and observed data selected**. Time steps shown are
  - The time steps from the observed case
  - The first time step from the grid case(s)
  - If no time steps from grid cases match the observed time step, display the two adjacent grid case time steps.
3. **Grid case(s) and RFT File case(s) selected**. Same display logic as point 2, except from RFT cases behaving as observed data.
4. **All types of cases selected**. Same display logic as point 2, except from RFT File case time steps are treated as grid time steps.

Each time step is postfixed by an indication of which source type(s) the time step is belonging to. This indication is displayed as one or more letters within square brackets. Examples: **[ O ]**, **[ R G ]**.
- **O** -- Time step is from observed data
- **R** -- Time step is from RFT data
- **G** -- Time step is from Grid data

More than one letter for one single time step, means that the time steps comes from multiple case types.

### Zonation/Formation Names
This property editor lets the user control the visibility of formations lines. This is what it looks like in the RFT plot context.

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
