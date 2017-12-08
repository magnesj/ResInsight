---
layout: docs
title: PLT Plot
permalink: /docs/pltplot/
published: true
---

An PLT plot is a special well plot displaying PLT (Production Log Tool) data against measured depth (TVD). PLT data may be a part of the grid model or may be loaded from well log files (\*.LAS) and well path files (\*.dev, \*.json etc.).

![]({{site.baseurl}}/images/PltPlot.png)

## Create New PLT Plot
There are several ways to create new PLT Plots
- Right-click a Well Path under Wells in the project tree and then select **New PLT Plot**.
- Right-click the PLT Plots node in the project tree and select **New PLT Plot**.
- Right-click a simulation well in the 3D view that has an associated well path and select **Well Plots -> New PLT Plot**.
- Right-click a simulation well in the project tree that has an associated well path and select **New PLT Plot**.

## Constraints
If an imported well log file does not contain PLT data, that data source (_Observed data_) will not be visible in the sources field in the property editor. PLT data in a well log file is expected to have column names:

- Oil: _QOZT_, _QOIL_, _xxxx_QOIL_
- Gas: _QOZT_, _QGAS_, _xxxx_QGAS_
- Water: _QGZT_, _QWAT_, _xxxx_QWAT_
- Total: _QTZT_, _QTOT_, _xxxx_QTOT_

## Property Editor
The property editor lets the user select which curves to display in the PLT plot.

<p align="center">
  <img src="{{site.baseurl}}/images/PltPlotPropertyEditor.png"/><br/>
  PLT plot property editor
</p>

### Well Name
A PLT plot is always related to a well path. The **Well Name** field contains the selected well path. Only observed well paths are displayed.

### Sources
The **Sources** field shows the sources for the selected well path should appear in the sources field. The different sources are grouped in three different groups:
- **RFT File Cases** -- Cases imported from \*.rft file(s) included in the simulation output case. (The keyword _WRFTPLT_ was used in the schedule file during simulation output generation)
- **Grid Cases** -- Simulation cases
- **Observed Cases** -- Observed data imported from well log files and well path files

When the user selects a source, time steps for that source appears in the **Time Steps** field.

### Time Steps
The **Time Steps** field contains available time steps for the selected source(s). Some combinations of selected sources may display a filtered list of time steps instead of the union of all time steps for all selected sources. The policy is as follows:
1. **Exclusively grid cases selected**. All available time steps for the selected grid cases are displayed.
2. **Grid case(s) and observed data case selected**. Time steps shown are
  - The first time step from the merged time step list from all grid cases
  - If no time steps from grid cases match the observed time step, display the two adjacent grid case time steps.
3. **Grid case(s) and RFT File case(s) selected**. Same display logic as point 2
4. **All types of cases selected**. Same display logic as point 2 with the exception that RFT File case time steps are treated as grid time steps.

Each time step is postfixed by an indication of which source type(s) the time step is belonging to. This indication is displayed as one or more letters within square brackets. Examples: **[ O ]**, **[ R G ]**.
- **O** -- Indicates that the current time step is from observed data
- **R** -- Indicates that the current time step is from RFT data
- **G** -- Indicates that the current time step is from Grid data

More than one letter for one single time step, means that the time steps is from multiple case types.

### Curve Selection
The curve selection group lets the user control what to display.
- **Standard Volume** --
- **Reservoir Volume** --
- **Oil** -- Check to display the oil component
- **Gas** -- Check to display the gas component
- **Water** -- Check to display the water component
- **Total** -- Check to display the total component

### Zonation/Formation Names
This property editor lets the user control how formations are handled. This is what it looks like in the PLT plot context.

![]({{site.baseurl}}/images/RftPltFormationNames.png)

Please see the [full documentation]({{site.baseurl}}/docs/formations) on the formations property editor for details.

<div class="note">
  When the formation names property editor is used in the context of PLT plots, the fields <b>Trajectory</b> and <b>Simulation Well</b> are hidden because those values are given by the PLT plot definition.
</div>

### Legend and Axis
![]({{site.baseurl}}/images/PltLegendAndAxis.png)

This property editor lets the user control visual properties for the legend and axis.
- **Depth Type** -- Select depth type, _Measured Depth_ or _Pseudo Length_
- **Depth Unit** -- Select depth unit, _Meter_ or _Feet_
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
  
  ### Plot
  The PLT plot displays groups of curves. A group consists of the components oil, gas and water. The curves within a group are stacked.
