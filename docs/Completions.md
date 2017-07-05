---
layout: docs
title: Completions
permalink: /docs/completions/
published: true
---
![]({{ site.baseurl }}/images/CompletionsIllustration.png)

## Fishbones

### Modelling of Fishbone Completions

To add new fishbones completions, select the «New Fishbones Subs Definition”. This menu item is available by right clicking on **Wells** in the Porject Tree or right clicking on the well in the view. 

![]({{ site.baseurl }}/images/Completions_AddNew.png)

<div class="note info">
In the property editor, the settings for the Fishbones group becomes available when a fishbone group is created. These properties are only used for the export, see below. 
</div>

### Fishbone Subs Definition
A Fishbones Subs Definition (a group of fishbones) is created in the Project tree from the command descibed above. New subs definitions can be created to give more flexibility in the placing of the fishbones. 

In the property editor one can set the parameters for the fishbones subs

![]({{ site.baseurl }}/images/Fishbones_SubDefPropEdit.png)

**Location defined by** -- This parameter has three options, “Start/End/Number of Subs”, “Start/End/Spacing” and “User Specified”. The setting will control which of the **location** parameters the user can control and which are calculated automatically. 
- **Start MD / End MD** -- Position, in Measured depth along the well, of the first / last fishbone Sub. The unit will be dependent on unit system for the well (m for metric units and ft for field units). 
- **Measured Depths** -- The measured depth of the fishbone subs. If the “location defined by” is set to the “User specified” option, only this property will be available to the user. 

Laterals configurations sets up the configuration of the laterals at each sub position.  
- **Laterals Per Sub** -- Number of laterals for each sub position
- **Lenght(s)** -- Length of each lateral, in m or ft. 
- **Exit Angle** -- Exit angle for fishbone lateral, in degree. 
- **Build Angle** -- Build angle for fishbone lateral, in degree pr meter. 
- **Orientation** -- Can be “Fixed Angle”, in which case the user can specify the angle for the first lateral, or “Random angle” where each sub will have a random orientation. Notice that the angle between each of the laterals will be constant, with the laterals equally spaced. 
- **Install Success Rate** -- Gives the probability of success for installation of each of the fishbones laterals. If 1, all laterals are installed.  

<div class="note info">
**Well Properties** and properties for **Multi Segment Wells** are only used for export. See description of these parameters below. 
</div>

### Imported Laterals

By selecting **Import Completions From File** fishbone laterals which have been previously exported can be imported. For imported laterals there are no possibility to change the modelling of the completions, and all parameters are related to export (see below).  

### Fishbones Export 

#### Export Laterals
The **Export Laterals** command will export the fishbones laterals as a *.dev*-file, which can be imported as a completion. Notice that well properties needed for export of well segment data or connection factors is not saved, and must be entered manually after import of the laterals. 

#### Export of Fishbone Completion Data

![]({{ site.baseurl }}/images/Fishbones_PropEdit.png)

- **Hole Diameter** -- The hole diameter for the main bore will be used in the calculation of the transmissibility (connection factor) into the main bore. 
- **StartMD** – the start position for the fishbones. This will be set to the highest possible value automatically, but can be set lower by the user. Gives the point along the well from which the transmissibility from the matrix to the main bore will be calculated.  



#### Export Well Segments
For multisegment wells there are additional parameters which should be set. These are used in the export of WELSEGS data. 


Pressure drop can be either *Hydrostatic*, *Hydrostatic + Friction* or *Hydrostatic + Friction + Acceleration*. 
Length and Depth can be *Incremental* or *Absolute*. 






## Perforation Intervals

### Modelling of Perforation Interval Completions
### Export of Perforation Interval Completion Data


## Exporting Completion Data

![]({{ site.baseurl }}/images/Completions_ExportCompletionData.png)


