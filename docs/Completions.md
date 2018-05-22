---
layout: docs
title: Completions
permalink: /docs/completions/
published: true
---
![]({{ site.baseurl }}/images/CompletionsIllustration.png)

Three types of completions are available for modeling and export: Perforation intervals, fishbone wells and fractures. The purpose of modeling these completions, is that it enables ResInsight to export Eclipse keywords for doing more refined simulations.

Completions can be modeled by adding new perforation intervals, fishbones subs or fractures. Details about the completions must then be specified, such as the length of the perforation interval or number of fishbone subs and laterals. After modeling the completions, the transmissibility (cell connection factors) can be calculated and exported to the Eclipse COMPDAT Keyword. See [ Export Completions]({{ site.baseurl }}/docs/completionexport)

For fishbones completions, multi-segment well information can even be exported to the Eclipse Keywords WELSEGS, COMPSEGS and WSEGVALV. See [ Export Well Segments]({{ site.baseurl }}/docs/completionexport#export-well-segments)

<div class="note">
  The derived cell property <b>Completion Type</b> displays wich cells are intersected by the completions.
  See <a href="{{ site.baseurl }}/docs/derivedresults#completion-type">Derived Results&rarr;Completion Type</a>
</div>


## Visualization and Inspection of Well Connection Factors

![]({{ site.baseurl }}/images/Completions_WellConnectionFactors.png)

Connection factors both from the Simulation Wells and the ones calculated from the modeled completions can be visualized in the 3D view by enabling **Well Connection Factors** in the **Project Tree**. The connection factors are displayed with a symbol as in the example shown above. The color of the symbol displays the magnitude of the connection factor and can be controlled by the legend settings.  Additional details of the connection factor are displayed in the **Result Info** window when clicking on the connection factor symbol.

The **Property Panel** of the **Well Connection Factors** item has a few options:

- **Geometry Scale Factor** -- Scales the connection factor symbols relative to the displayed well path radius
- **Show on Closed Connections** -- Show the connection factors calculated for closed simulation well cells in addition to the ones at open cells.

A combined view of well connection factors and the derived result **Completion Type** enables the user to see the completion type contributing to connection factor in a cell 
( [Derived Results, Completion Type]({{ site.baseurl }}/docs/derivedresults#completion-type) )


## Perforation Intervals

A new perforation interval on a well can be set up by right-clicking on the well in the **3D View** or in the **Project Tree**, and choosing the command **New Perforation interval**. 

![]({{ site.baseurl }}/images/CreateCompletionOnWellPath.png)

![]({{ site.baseurl }}/images/PerforationIntervals_propEditor.png)

After creating the perforation interval, the following properties of the perforation can be set in the property editor: 
- **Start MD**, **End MD** -- Measured depth along the well path for the perforation to start/stop. 
- **Diameter** -- Diameter of the perforation, used in calculation of transmissibility ( For details on the transmissibility calculation, see [Transmissibility Calculations]({{ site.baseurl }}/docs/completionexport#transmissibility-calculations)). 
- **Skin Factor** -- Skin factor for the perforation, used in calculation of transmissibility. 
- **All Timesteps** -- If on, the perforation interval will be present for all time steps
- **Start Date** -- The perforation will be included in the model for all time steps after this date. If "All TimeSteps" is turned on, this option is not available and the perforation is included for all time steps. 

The perforation intervals will be indicated by different color along the well path. 

![]({{ site.baseurl }}/images/WellPerforationIntervalColor.png)

### Import Perforation Intervals

Perforation intervals can be imported into ResInsight from _`*.ev`_ files. These files consist of a list of wells, and their corresponding measured depth values for perforation start and end. 

#### Perforation Interval File Format

"--" is interpreted as the start of a comment. The comment ends at the end of the line. 
The files can start with a unit definition line:

    UNITS <unitname>

<div class="note info">
This line is ignored for now. The numbers are interpreted to be in the units present in the case. 
</div>

In the following any number of :

    WELLNAME <well-name>
    <date>  <well completion>  <top mMD>  <base mMD>   <bore hole diameter>  <skin factor>
    <date>  <well completion>  <top mMD>  <base mMD>   <bore hole diameter>  <skin factor>

- _date_ -- Start date of the completion in the format "dd mmm yyyy". Eg `01 SEP 2006`. A special `"SOH"`date is also allowed meaning Start Of History.
- _well completion_ -- For now, only `"perforation"` is supported

Here is an example:

    UNITS METRIC

    -- R-2 AH sidetrack into Ile/Tilje
    WELLNAME R-2AH
    "SOH"   perforation 6200 6350 0.212 0   -- taget Ile 2 and Ile 3
    "SOH"   perforation 7050 7133 0.212 0   -- target Tilje 3, 83 m prodint

    -- S-2 AH
    WELLNAME S-2AH
    "SOH"   perforation 4340 4369 0.212 0   -- target Garn 2, 29 m prodint (update 290915)
    01 SEP 2006   perforation 5060 6185 0.212 0   -- target Tilje 3, 1125 m prodint

## Fishbones

Fishbones are completions created by drilling or etching a set of small holes nearly perpendicular to the main bore.
Each set of holes is created in the same operation and is callea a _sub_ while each individual hole is called a _lateral_.

For each well path there is a top level folder in the **Project Tree** containing all the fish bone definitions containing settings that applies to all the fishbones for this well path. 

![]({{ site.baseurl }}/images/Fishbones_PropEdit.png)

- **Fishbone Well Properties** -- Settings used when exporting well connection factors
  - **StartMD** -- the start position for the fishbones. This will be set to the highest possible value automatically, but can be set lower by the user. Gives the point along the well from which the transmissibility from the matrix to the main bore will be calculated.  
  - **Main Bore Diameter** -- The hole diameter for the main bore will be used in the calculation of the transmissibility (connection factor) into the main bore. 
  - **Main Bore Skin Factor** -- The skin factor for the main bore, used in calculation of the transmissibility (connection factor) into the main bore. 
For multi-segment wells there are additional parameters which should be set. These are used in the export of WELSEGS data. 
- **Multi Segment Wells** - Options used by the Well Segments Export
  - **Liner Inner  eter** -- The liner inner diameter for the fishbones. 
  - **Roughness Factor** -- The roughness factor used in export of main bore segments. 
  - **Pressure Drop** -- can be either *Hydrostatic*, *Hydrostatic + Friction* or *Hydrostatic + Friction + Acceleration*. 
  - **Length and Depth** -- Used in WELSEGS export - when specifying the length and depth change for each segment
    - **Incremental** -- length / depth of given segment
    - **Absolute** -- the length down the tube or depth of the last nodal point

### Fishbones Subs Definition

To add new fishbones completions, select the **New Fishbones Subs Definition** command. This menu item is available by right clicking on **Wells** in the Project Tree or right clicking on the well trajectory in the 3D View. 

![]({{ site.baseurl }}/images/CreateCompletionOnWellPath.png)

The new **Fishbones Subs Definition** (a group of fishbone subs) is created in the Project tree. Several subs definitions can be created on the same well trajectory to give more flexibility in placing of the fishbones. 

![]({{ site.baseurl }}/images/Fishbones_SubDefPropEdit.png)
- **Appearance**
  - **Fishbones Color** -- The 3D View color of the fishbones created by this definition
- **Location** -- Options to control the position and number of fishbone subs created by this definition
  - **Location Defined By** -- This setting will select how to define the location of the subs in this group. 
  - **Start MD** -- Position, in Measured depth along the well, of the first fishbone Sub. 
  - **End MD** -- Position of the last fishbone Sub. 
  - **Number of Subs** -- Defines the number of subs to create in the defined interval
  - **Spacing** -- Spacing between the subs within the interval
  - **Measured Depths** -- The measured depth of each of the fishbone subs. If the **Location Defined By** is set to the **User Specified** this will be directly editable by the user. 
- **Laterals Configuration** -- Configures the laterals at each sub position.  
  - **Laterals Per Sub** -- Number of laterals for each sub position
  - **Lenght(s)** -- Length of each lateral, in m or ft. 
  - **Exit Angle** -- Exit angle for fishbone lateral, in degree. 
  - **Build Angle** -- Build angle for fishbone lateral, in degree pr meter. 
  - **Orientation**
    - **Fixed Angle** -- The user can specify the angle for the first lateral
    - **Random angle** -- Each sub will have a random orientation. Notice that the angle between each of the laterals will be constant, with the laterals equally spaced. 
  - **Install Success Rate** -- Gives the probability of success for installation of each of the fishbones laterals. If 1, all laterals are installed.  
- **Well Properties** -- Settings to control the connection factor calculation used in [Completion Export]({{ site.baseurl }}/docs/completionexport))
  - **Hole Diameter** -- The hole diameter of the lateral
  - **Skin Factor** -- The skin factor used in the transmissibility calculation for the lateral. 
- **Multi Segment Wells** -- Settings used for Well Segment Export
  - **Tubing Diameter** -- The diameter used in the *WELSEGS* export of the laterals. 
  - **Open Hole Roughness Factor** -- Exported directly to the *WELSEGS* keyword.
  - **Tubing Roughness Factor** -- Exported directly to the *WELSEGS* keyword.
  - **ICDs per Sub** -- The number of ICD (valves) per Sub, used for calculation of total ICD area for *WSEGVALV* export. 
  - **ICD Orifice Diameter** -- The Diameter of the ICD, used for calculation of ICD area for *WSEGVALV* export. 
  - **ICD Flow Coefficient** -- The flow coefficient, exported directly as a part of *WSEGVALV*.

### Import Well Trajectories as Fishbones Laterals

The command **Import Completions From File** can be used to import well trajectories in the _`*.dev`_ format as fishbone laterals.  The imported laterals are listed under the folder **Imported Laterals** in the **Project Tree**. These laterals behave as completions, and will be exported when exporting completion data using the settings in the property panel of the **Imported Laterals** folder.

![]({{ site.baseurl }}/images/Fishbones_LateralsMSWprop.png)

### Export Fishbones as Well Trajectories

The **Export Laterals** command will export the fishbone laterals as a well trajectory into a  _`*.dev*`_ -file. 

<div class="note info">
Notice that only the trajectory data is exported. Properties related to well segment data or Completion Data export can not be written to <code>*.dev*</code> files. 
</div>

## Fractures

Hydraulic fractures are completions created by pressurizing the reservoir at a certain point in the well, and thereby creating a crack in the formation. A substance is then injected into the crack to keep it open when relaxing the pressure.

Fractures in ResInsight can be added both to simulation wells and Well Trajectories, and have two main types: Elliptical fractures and StimPlan fractures. 

### Visualization control

The **Fractures** item in the **Project Tree** controls whether to show the modeled fractures or not. For fractures along the well path the **Perforation Length** is shown as an olive green cylinder around the well path as shown in the image below.

![]({{ site.baseurl }}/images/FractureVizControl.png) 

- **Colors** -- Control the color settings for the fractures in the View
  - **Result Variable** -- Selects the fracture result to use for coloring the fracture. This is most interesting when you have imported a StimPlan fracture with varying width, conductivity and other parameters. The legend item in the **Project Tree** controls how the legend is set up.
  - **Default Color** -- The color to use if the requested result is not available for some fracture.
- **StimPlan**
  - **Show Mesh** -- Toggles the visualization of the StimPlan mesh lines, showing the StimPlan cell borders
  - **Color Interpolation** -- Toggles interpolation of the StimPlan results. When Off, one color per StimplanCell is shown. When on, ResInsight interpolates the color between the cells.
  
### Fracture Templates

To create a fracture you first need a **Fracture Template**. The template collects settings that are likely to be shared among several fractures, most importantly the fracture geometry. A fracture at a particular place refers to the template, and it is thereby possible to share fracture geometry and other settings between fracture instances. 

Fracture Templates are listed in a folder with the same name in the **Project Tree**

![]({{ site.baseurl }}/images/FractureTemplates.png)

To create a new fracture template, right-click the **Fracture Template** item in the tree, and select either **New Ellipse Fracture Template** or **New StimPlan Fracture Template**. 

#### Common Fracture Template Options

![]({{ site.baseurl }}/images/CommonFractureTemplateProperties.png)
- **Name** -- User defined name for the template. Used when selecting which template to use in the Fracture
- **ID** -- Assigned ID to be used when addressing this template form a Command file script
- **Geometry**
  - **Fracture Orientation** -- The fractures orientation
    - **Transverse(normal) to Well Path** -- The fracture plane is vertical and normal to the well path at the intersection point.
    - **Along Well Path** -- Fracture is vertical and along the well path. This option enables options to control the perforation length and the efficiency of the well in the fracture. See below.
    - **Azimuth** -- The fracture is vertical and in line with the Azimuth Angle (measured from North) supplied.
- **Fracture Truncation**
  - **Fracture Containment** -- Enable this option to limit what K layers you want the fracture to influence. K-Layers outside the range will not be drained by the fracture.
  - **Top Layer** -- Topmost K-layer that the fracture will drain.
  - **Base Layer** -- Lowest K-layer that the fracture will drain.
- **Properties** -- The availability of these options depend on the **Fracture Orientation** and the **Conductivity in Fracture** setting.
  - **Conductivity in Fracture** 
    - **Finite Conductivity** -- Use a calculated conductivity for flow in the fracture. Either the StimPlan conductivity, or a constant conductivity in Ellipse fractures.
    - **Infinite Conductivity** -- Assume infinite conductivity in the fracture itself. For StimPlan fractures this will ignore the conductivity in the StimPlan data.
  - **Skin Factor** -- Used when exporting to Eclipse.
  - **Perforation Length** -- The length of the intersection between the well bore and the fracture when the fracture is along the well path ( Fractures **Along Well Path** only ).
  - **Perforation Efficiency** -- The efficiency of the wellbore-fracture perforation ( Fractures **Along Well Path** only ).
  - **Well Diameter** -- Used when exporting to Eclipse.
- **Sensitivity Scale Factors** -- Each of the properties are scaled by the scaling factor. When used interactively, the **Apply** button must be pressed to apply the scaling factors. These scaling factors are also available from command file scripting 
  - **Height, Width** -- Scales the height or the _*Halflength*_ of the fracture template
  - **D-factor** -- Scales the none-darcy flow factor
  - **Conductivity** -- Scales the conductivity in the fracture directly

#### Non-Darcy Flow

![]({{ site.baseurl }}/images/FractureNonDarcyFlow.png)

Non-Darcy Flow is used to improve the computation of connection factors for cells penetrated by well pipes. A pre-computed D-factor can be set using **User Defined D-factor**. Selecting the option **Compute D-factor** displays the following set of properties:
  - **Inertial Coefficient** -- Beta-factor in Forcheimer units
  - **Effective Permeability**
    - **User Defined** -- Defines the permeability *Ke* in milliDarcy in the property **Effective Permeability** 
    - **Use Fracture Conductivity** -- Extract conductivity from the fracture template at the well intersection location and scale with **Relative Permeability**
  - **Width**
    - **User Defined** -- Defines the width of the fracture
    - **Use Fracture Width** -- Extract width from the fracture template at the well intersection location
  - **Relative Gas Density**
  - **Gas Viscosity**
  - **D Factor** -- Displays the computed value of the D factor
  - **D Factor Details** -- Displays the value of variables used to compute the D factor

#### Ellipse Fracture Template

![]({{ site.baseurl }}/images/EllipseFractureTemplateProperties.png)

- **Name** -- User name of this particular fracture template
- **Geometry** 
  - **Halflength X<sub>r</sub>** -- Half the width of the ellipse
  - **Height** -- The height of the elliptical fracture
  - **Fracture Orientation** -- See above
- **Fracture Truncation** -- See above
- **Properties** -- The availability of these options depend on the **Fracture Orientation** and the **Conductivity in Fracture** setting
  - **Permeability** -- A constant permeability inside the fracture (Used to calculate conductivity in the fracture)
  - **Width** -- Crack width (Used to calculate conductivity in the fracture)
- **Sensivity Scale Factors** -- See above
- **Non-Darcy Flow** -- See above

#### Stimplan Fracture Template

StimPlan fracture templates imports XML output from the StimPlan software. These XML files contains results from a simulated hydraulic fracture, describing both geometry, varying crack width, resulting conductivity etc. as time varying quantities. Both symmetric and asymmetric StimPlan fracture templates are supported.

![]({{ site.baseurl }}/images/StimplanFractureTemplateProperties.png)

- **Name** -- User name of this particular fracture template
- **Show StimPlan Mesh** -- Show or hide the mesh lines on the fracture in the 3D View
- **Input**
  - **File Name** -- Path to the imported StimPlan XML-file
  - **Active Time step Index** -- Time step in the StimPlan simulation to use for transmissibility calculations and visualization
  - **Well/fracture Intersection Depth** -- The position of the fracture along the well path as MD.
- **Geometry**
  - **Fracture Orientation** -- See above
- **Fracture Truncation** -- See above
- **Properties**
  - **Conductivity Scaling Factor** -- Scale the overall conductivity to do sensitivity studies.
- **Sensitivity Scale Factors** -- See above
- **Non-Darcy Flow** -- See above

### Fracture Instances

Instances of fractures can be created along well paths and simulation wells by right clicking the well pipe in the 3D view, or the corresponding items in the **Project Tree**. 

![]({{ site.baseurl }}/images/FractureInstancePropertyPanel.png)

- **Name** -- User editable name
- **Location/Orientation**
  - **Measured depth Location** -- The measured depth
  - **Azimuth** -- Azimuth orientation of fracture. 
  - **Dip** -- Dip of fracture plane
  - **Tilt** -- Rotation of fracture about its plane normal
- **Properties**
  - **Fracture Template** -- Select the fracture template to use for this fracture.
  - **StimPlan Time Step** -- Displays the time step used by the template 
  - **Perforation Length** / **Perforation Efficiency** / **Well Diameter** -- These values are copied from the new template when selecting a different one. See [Common Fracture Template Options]({{ site.baseurl }}/docs/completions#common-fracture-template-options)  
- **Fracture Center Info** -- This group displays info on where the center of the fracture is positioned. The center is defined to be where the well path intersects the fracture.

