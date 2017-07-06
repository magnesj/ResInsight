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

- **StartMD** – the start position for the fishbones. This will be set to the highest possible value automatically, but can be set lower by the user. Gives the point along the well from which the transmissibility from the matrix to the main bore will be calculated.  
- **Main Bore Diameter** -- The hole diameter for the main bore will be used in the calculation of the transmissibility (connection factor) into the main bore. 




#### Export Well Segments
For multisegment wells there are additional parameters which should be set. These are used in the export of WELSEGS data. 


![]({{ site.baseurl }}/images/Fishbones_PropEdit_MSW.png)

For the Fishbone group the following parameters can be set for Multi Segment Wells
- **Liner Inner Diameter** -- The liner inner diameter for the fishbones. 
- **Roughness Factor** -- The roughness factor of ...
- **Pressure Drop** -- can be either *Hydrostatic*, *Hydrostatic + Friction* or *Hydrostatic + Friction + Acceleration*. 
- **Length and Depth** can be *Incremental* or *Absolute*. Used in WELSEGS export - when specifyig the lenght and depth change for each segment, these will be incremental (length / depth of given segment) or abosolute (the length down the tube or depth of the last nodal point). 


![]({{ site.baseurl }}/images/Fishbones_ExportWellSegments.png)


**Tubing Roughness Factor** is exported as Roughness in Welsegs

Notice that there are additional MSW parameters in the property edit for the fishbones subs definition. 
![]({{ site.baseurl }}/images/Fishbones_LateralsMSWprop.png)

- **Tubing Diameter** -- Diameter of 
- **Open Hole Roughness Factor** -- The 
- **Tubing Roughness Factor** -- the... 
- **ICDs per Sub** -- The number of ICD (valves) per Sub, used for calculation of total ICD area for WSEGVALV export. 
- **ICD Orifice Diameter** -- The Diamater of the ICD, used for calculation of ICD area for WSEGVALV export. 
- **ICD Flow Coefficient** -- The flow coefficient, exported directly as a part of WSEGVALV.


In the output file there are data for three Eclipse keyword specified: 
- WELSEGS -- Defines multi-segment well 
The first WELSEGS entry contains information about the well: 
- *Name* - Name of well
- *Dep 1* - TODO
- *Tlen 1* - TODO
- *Len&Dep* - incremental or abosulute, as specified by the user in the Fishbones property editor. 
- *PresDrop* - specifies what is included in the pressure drop calculation, hydrostatic, friction or acceleration. Specified by user in the Fishbones property editor.

The folowing entries contains information about each segment: 
- *First Seg*, *Last Seg* -- Values are being exported pr segment, so both first and last segment number is the number of the segment being exported. 
- *Branch Num* -- Branch number for segment being exported 
- *Outlet Seg* - The segment the exported segment is connected to. For the main bore segments, this is the segment before them, for ICDs the segment number being exported and for fishbone laterals the segment on the main broe where the laterals are connected.  
- *Length* - Length of segment
- *Depth Change* - Depth of segment. 
- *Diam* - Liner inner diamater for the main bore and ICD entries. TODO: check for Laterals. 
- *Rough* - The roughness factor as entered by the user. Notice that a different value can be specified for the main bore and the laterals, as described above.       

The list of entries contains information on the main stem, the ICDs at the fishbone subs and the fishbone laterals. A commet above each entry detals which element (main bore / ICD / lateral) the entry is for. 


    -- Name            Dep 1          Tlen 1       Vol 1     Len&Dep     PresDrop     
       Well Path A     4137.09154     87.00000     1*        ABS         H--           /
    -- First Seg     Last Seg     Branch Num     Outlet Seg     Length        Depth Change     Diam        Rough       
    -- Main stem
    -- Segment for sub 0
       2             2            1              1              13.00000      0.53667          0.15200     0.00001      /
    -- Laterals
    -- Diam: MSW - Tubing Radius
    -- Rough: MSW - Open Hole Roughness Factor
    -- ICD
       3             3            2              2              0.10000       0                0.15200     0.00001      /
    -- Fishbone 0 : Sub index 0 - Lateral 0
       52            52           27             3              1.70326       -0.57276         0.00960     0.00100      /
       53            53           27             52             2.34748       -0.81635         0.00960     0.00100      /


- COMPSEGS -- Along a multisegment well, the COMPSEGS keyword defines the location of the completions 
The first COMPSEGS entry is a line with the well path name. Each following entry is for the segments in the well, and contaning the following field: 
- *I*, *J* and *K* -- The Eclipde cell index
- *Branch no* -- Branch number for the segment
- *Start Length*, *End Length* -- Start and end lenght along the well for the relevent segment. 

- **WSEGVALV** -- Defining segments representing a sub-critical valve. 
The parameters exported in the WEGVALV keword are
- *Well Name* -- The name of the well
- *Seg No* -- Segment number along the well
- *Cv* -- The ICD Flow Coefficient, as entered by the user
- *Ac* -- the total ICD area per sub, calculated as the area per ICD (given by the orifice radius) multiplied with the number of icds per Sub.       




## Perforation Intervals

### Modelling of Perforation Interval Completions
![]({{ site.baseurl }}/images/PerforationIntervals_propEditor.png)

When creating a new perforation interval, the following properties of the perforation can be set in the property editor: 
- **Start MD**, **End MD** -- Measured depth along the well path for the perforation to start/stop. 
- **Diameter** -- Diameter of the perforation, used in calculation of transmissibility (see below). 
- **Skin Factor** -- Skin factor for the perforation, used in calculation of transmissibility (see below). 
- **Start of History** -- Turned on if the perforation should be present for all time steps
- ** Start Date** -- The perforation will be included in the model for al time steps after this date. If "Start of History" is turned on, this option is not available and the perforation is included for all time steps. 

The perforation intervals will be indicated by different colour along the well path. 

### Export of Perforation Interval Completion Data




## Exporting Completion Data

![]({{ site.baseurl }}/images/Completions_ExportCompletionData.png)

- **Export Folder** -- Folder for the exported COMPDAT file. If it does not already exist, it will be created when performing the export. The exported file will get a fixed name based on what is included in the export. 
- **Case to Apply** -- Select which case to use for the export. Matrix transmissibilities will be read from this case.  
- **Export**  -- Can be *Calculated Transmissibilities* or *Default Connection Factors and WPIMULT*. If *Caclulated Transmissibilities* is chosen, the transmissibilities calculated based on the case and completion data are exported directly. If the *Default Connection Factors and WPIMULT* is chosen, the information about the connections for Eclipse to be able to make the transmissbility calculaton is exported for the COMPDAT keyword. In addition, the same transmissibility calculation is performed by ResInshight, and the factor between the actual transmissibility for the connection and the Eclipse calculation is exported in the WPIMULT keyword. 
- **Well Selsction** -- *All Wells* or *Checked wells* if exporting from a well path collection. *Selected wells* if exporting wells. 
- **File Split** -- *Unified File*, *Split On Well* or *Split on Well and Completion Type*. If there are completions of multiple types or along multiple wells, this parameter determines if the entries for the different wels / completion types should be in the same file or split in different files. 
- **Include Fishbones** -- Option to inclulde or exclude fishbone completions from the export. 
- **Exclude Main Bore Transmissibility For Fishbones** -- If this options is checked on, only the transmissibilities for the fishbone laterals will be included in the export, and transmissibility along the main bore will not contribute. 
- **Include Perforations** -- Option to include or exclude perforation invervals in the export. 
- **Time step** -- Which timestep to export. This option is included since perforation intervals have a start time, and thus not all perforations need be present at all time steps. 
