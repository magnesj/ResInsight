---
layout: docs
title: Completions
permalink: /docs/completions/
published: true
---
![]({{ site.baseurl }}/images/CompletionsIllustration.png)

Three types of completions are available for modelling and export - perforation invervals, fishbone wells and fractures. The purpose of modelling these completions, is that it enables ResInsight to export Eclipse keywords for doing more refined simulations.

Completions can be modelled by adding new perforation intervals, fishbones subs or fractures. Details about the completions must then be specified, such as the length of the perforation interval or number of fishbone subs and laterals. After modelling the completions, the transmissibility (cell connection factors) can be calculated and exported to the Eclipse COMPDAT Keyword. 

For fishbones completions, multisegment well information can even be exported to the Eclipse Keywords WELSEGS, COMPSEGS and WSEGVALV. 

## Perforation Intervals

A new perforation interval on a well can be set up by right-clicking on the well in the **3D View** or in the **Project Tree**, and choosing the command **New Perforation interval**. 

![]({{ site.baseurl }}/images/CreateCompletionOnWellPath.png)

![]({{ site.baseurl }}/images/PerforationIntervals_propEditor.png)

After creating the perforation interval, the following properties of the perforation can be set in the property editor: 
- **Start MD**, **End MD** -- Measured depth along the well path for the perforation to start/stop. 
- **Diameter** -- Diameter of the perforation, used in calculation of transmissibility ( For details on the transmissibility calculation, see [Export of Perforation Interval Completion Data]({{ site.baseurl }}/docs/completions#export-of-perforation-interval-completion-data)). 
- **Skin Factor** -- Skin factor for the perforation, used in calculation of transmissibility. 
- **All Timesteps** -- If on, the perforation interval will be present for all time steps
- **Start Date** -- The perforation will be included in the model for al time steps after this date. If "All TimeSteps" is turned on, this option is not available and the perforation is included for all time steps. 

The perforation intervals will be indicated by different colour along the well path. 

![]({{ site.baseurl }}/images/WellPerforationIntervalColor.png)


## Fishbones

Fishbones are completions created by drilling or etching a set of small holes nearly perpendicular to the main bore.
Each set of holes is created in the same operation and is callea a _sub_ while each individual hole is called a _lateral_.

For each wellpath there is a top level folder in the **Project Tree** containing all the fish bone definitions containing settings that applies to all the fishbones for this well path. 

![]({{ site.baseurl }}/images/Fishbones_PropEdit.png)

- **Fishbone Well Properties** -- Settings used when exporting well connection factors
  - **StartMD** –- the start position for the fishbones. This will be set to the highest possible value automatically, but can be set lower by the user. Gives the point along the well from which the transmissibility from the matrix to the main bore will be calculated.  
  - **Main Bore Diameter** -- The hole diameter for the main bore will be used in the calculation of the transmissibility (connection factor) into the main bore. 
  - **Main Bore Skin Factor** -- The skin factor for the main bore, used in calculation of the transmissibility (connection factor) into the main bore. 
For multisegment wells there are additional parameters which should be set. These are used in the export of WELSEGS data. 
- **Multi Segment Wells** - Options used by the Well Segments Export
  - **Liner Inner  eter** -- The liner inner diameter for the fishbones. 
  - **Roughness Factor** -- The roughness factor used in export of main bore segments. 
  - **Pressure Drop** -- can be either *Hydrostatic*, *Hydrostatic + Friction* or *Hydrostatic + Friction + Acceleration*. 
  - **Length and Depth** -- Used in WELSEGS export - when specifyig the lenght and depth change for each segment
    - **Incremental** -- length / depth of given segment
    - **Absolute** -- the length down the tube or depth of the last nodal point

### Fishbones Subs Definition

To add new fishbones completions, select the **New Fishbones Subs Definition** command. This menu item is available by right clicking on **Wells** in the Porject Tree or right clicking on the well trajectory in the 3D View. 

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
- **Well Properties** -- Settings to control the connection factor calculation used in [Completion Data Export]({{ site.baseurl }}/docs/completions#exporting-completion-data))
  - **Hole Diameter** -- The hole diameter of the lateral
  - **Skin Factor** -- The skin factor used in the transmissibility calculation for the lateral. 
- **Multi Segment Wells** -- Settings used for Well Segment Export
  - **Tubing Diameter** -- The diameter used in the *WELSEGS* export of the laterals. 
  - **Open Hole Roughness Factor** -- The rougness factor used in the *WELSEGS* export of the laterals. 
  - **Tubing Roughness Factor** --  
  - **ICDs per Sub** -- The number of ICD (valves) per Sub, used for calculation of total ICD area for *WSEGVALV* export. 
  - **ICD Orifice Diameter** -- The Diamater of the ICD, used for calculation of ICD area for *WSEGVALV* export. 
  - **ICD Flow Coefficient** -- The flow coefficient, exported directly as a part of *WSEGVALV*.

### Import Well Trajectories as Laterals

The command **Import Completions From File** can be used to import well trajectories in the `*.dev`format as fishbone laterals.  The imported laterals are listed under the folder **Imported Laterals** in the **Project Tree**. These laterals behave as completions, and will be exported when exporting completion data using the settings in the property panel of the **Imported Laterals** folder.

![]({{ site.baseurl }}/images/Fishbones_LateralsMSWprop.png)

### Export Fishbones as Well Trajectories

The **Export Laterals** command will export the fishbone laterals as a well trajectory into a  *.dev*-file. 

<div class="note info">
Notice that only the trajectory data is exported. Properties related to well segment data or needed to calculate connection factors or are not exported. 
</div>


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

### Transmissibility Calculations

The transmissibility calculation is performed for each direction, X, Y and Z, in an orthogonal coordinate system local to the cell. 

Taking the X direction as an example, we first calculate the relevant permeability *K* from the Eclipse properties *PERMY* (K<sub></sub>) and PERMZ (K<sub>z</sub>): 

![]({{ site.baseurl }}/images/Equation_PerfInterval_K.png)

The Peacman radius (pressure equvivalent radius) for the cell is then calculated, using permeabilites and cell sizes (D<sub>y</sub> and D<sub>z</sub>): 

![]({{ site.baseurl }}/images/Equation_PerfInterval_Peaceman.png)

The x-component of the transmissibility vector is calculated, using the length of the perforation in the x direction (l<sub>x</sub>), the well radius (r<sub>w</sub>) and skin factor (S):

![]({{ site.baseurl }}/images/Equation_PerfInterval_Trans.png)

The y and z component of the transmissibility are calculated in the same manner, and the total transmissibility is then calculated as: 

![]({{ site.baseurl }}/images/Equation_PerfInterval_TotalT.png)

If the *Export Calculated Transmissibilities* is chosen in the export setting (see TODO), this value is exported in the COMPDAT keyword directly. If the *Export Default Connection Factors and WPIMULT* the transmissibility is chosen, the transmissibility is calculated as above, and in addition the transmissibility is calculated as Eclipse would do it using values other than transmissibility in the COMPDAT keyword (perforation length, well radius etc). The ratio between these trasmissibilities is then exported as the WPIMULT value. 

For an example of *COMPDAT* files exported with calculated transmissibilities and with defaults and WPIMULT values, see export of fishbones completion data below.  

### Export of Fishbone Completion Data

The transmissibility calculation for the fishbones is done following the above description except that when calculating the transmissibility for the laterals, the full cell volume is split among the laterals for calculation of the transmissibility. This is done by finding the direction of the main bore, and then dividing the cell size in this direction by the number of laterals in the cell when calculating the Peaceman radius. 

An example of the exported COMPDAT file is shown below. The calculated transmissibility contribution to the cell connection factor from each lateral or main bore part is included as a comment. 

    COMPDAT
    -- Well            I      J      K1     K2     Status     SAT     TR               DIAM     KH     S     Df     DIR     r0     
    -- Well Path B main bore : 0.0569986
       Well Path B     26     45     29     29     OPEN       1*      5.699858E-02      /
    -- Fishbone 0: Sub: 0 Lateral: 0 : 0.0021382
    -- Fishbone 0: Sub: 0 Lateral: 1 : 0.00228575
    -- Fishbone 0: Sub: 0 Lateral: 2 : 0.0126269
    -- Fishbone 0: Sub: 1 Lateral: 1 : 0.0112929
    -- Fishbone 0: Sub: 2 Lateral: 0 : 0.00566964
    -- Well Path B main bore : 0.230572
       Well Path B     27     41     15     15     OPEN       1*      2.645858E-01      /
    /
    

For export with WPIMULT factors, the main bore diameter and direction are given in the export for cells which have both main bore and lateral contribuitons, while diamater and main direction of the first lateral is used for cells with no main bore contribuiton. Other parameters exported as part of COMPDAT are set to default. 

The *WPIMULT* parameters are calculated, as for the perforation intervals, by ResInsight calculating both the transmissibility of the completion as described above, and in addition calculating the transmissibility based on the information exported in the COMPDAT keyword. The ratio between these two numbers is then exported as the *WPIMUT* keyword. 

    COMPDAT
    -- Well            I      J      K1     K2     Status     SAT     TR     DIAM        KH     S           Df     DIR     r0     
    -- Well Path B main bore : 0.0569986
       Well Path B     26     45     29     29     OPEN       1*      1*     0.21600     1*     0.00000     1*     'Z'      /
    -- Fishbone 0: Sub: 0 Lateral: 0 : 0.0021382
    -- Fishbone 0: Sub: 0 Lateral: 1 : 0.00228575
    -- Fishbone 0: Sub: 0 Lateral: 2 : 0.0126269
    -- Fishbone 0: Sub: 1 Lateral: 1 : 0.0112929
    -- Fishbone 0: Sub: 2 Lateral: 0 : 0.00566964
    -- Well Path B main bore : 0.230572
       Well Path B     27     41     15     15     OPEN       1*      1*     0.21600     1*     0.00000     1*     'Z'      /
    /
    WPIMULT
    -- Well            Mult         I      J      K      
       Well Path B     0.70133      25     45     29      /
       Well Path B     25.11396     27     41     15      /
    /



## Export Well Segments

It is possible to export all the Fishbone Subs Definitions to a text file containing the Eclipse input data 
keywords needed to represent the fishbone part of the well as an MSW.

This can be done by the command **Export Well Segments** available as a context command on the **Fishbones** folder. Invoking the command will show a dialog prompting you to enter a target directory and which case to use in the calculations.

![]({{ site.baseurl }}/images/Fishbones_ExportWellSegments.png)



### Exported MSW Data

In the output file there are data for three Eclipse keyword specified.

##### WELSEGS
WELSEGS defines multi-segment well. The list of entries contains information on the main stem, the ICDs at the fishbone subs and the fishbone laterals. A commet above each entry detals which element (main bore / ICD / lateral) the entry is for.  Example: 

    WELSEGS
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
    /
   

- The first *WELSEGS* entry contains information about the well: 
  - **Name** - Name of well
  - **Dep 1** - TVD of start MD point, as given by the user in the Fishbones **Start MD** field.  
  - **Tlen 1** - Point given by the user in the Fishbones **Start MD** field.  
  - **Len&Dep** - incremental or abosulute, as specified by the user in the Fishbones property editor. 
  - **PresDrop** - specifies what is included in the pressure drop calculation, hydrostatic, friction or acceleration. Specified by user in the Fishbones property editor.

- The folowing *WELSEGS* entries contains information about each segment: 
  - **First Seg**, **Last Seg** -- Values are being exported pr segment, so both first and last segment number is the number of the segment being exported. 
  - **Branch Num** -- Branch number for segment being exported 
  - **Outlet Seg** -- The segment the exported segment is connected to. For the main bore segments, this is the segment before them, for ICDs the segment number being exported and for fishbone laterals the segment on the main broe where the laterals are connected.  
  - **Length** -- Length of segment (if incremental Len&Dep above) or length of segments including this along well (if abosulte Len&Dep above). For ICDs length is set to 0.1. 
  - **Depth Change** -- Depth of segment, incremental or abosolute as for Length. For ICDs depth is set to 0. 
  - **Diam** -- Diameter of segment. For main bore and ICD entries, the liner inner diamater for the Fishbones collection is used. For laterals, an effecive diamaeter is calculated so that the diameter exported is the diamaneter which, assuming a circle, would give the same area as the area betweeen the hole diamaeter and the tubing diameter.  
  - **Rough** -- The roughness factor as entered by the user. Notice that a different value can be specified for the main bore and the laterals, as described above.       

    
##### COMPSEGS
An example of the COMPSEGS keyword as exported is shown below.  

    COMPSEGS
    -- Name            
       Well Path A      /
    -- I      J      K      Branch no     Start Length     End Length     Dir Pen     End Range     Connection Depth     
       28     40     6      27            0.00000          1.70326         /
       28     40     7      27            1.70326          2.34748         /
       28     40     8      27            2.34748          2.96577         /
    /

The first COMPSEGS entry is a line with the well path name. Each following entry is for the segments in the well, and contaning the following field: 
- **I**, **J** and **K** -- The Eclipde cell index
- **Branch no** -- Branch number for the segment
- **Start Length**, **End Length** -- Start and end lenght along the well for the relevent segment. 

##### WSEGVALV
An example of the WSEGVALV keyword as exported is shown below.  

    WSEGVALV
    -- Well Name       Seg No     Cv          Ac          
       Well Path A     3          1.50000     0.00008      /
       Well Path A     5          1.50000     0.00008      /
       Well Path A     7          1.50000     0.00008      /
    /
    
The parameters exported in the WEGVALV keword are
- **Well Name** -- The name of the well
- **Seg No** -- Segment number along the well
- **Cv** -- The ICD Flow Coefficient, as entered by the user
- **Ac** -- the total ICD area per sub, calculated as the area per ICD (given by the orifice radius) multiplied with the number of icds per Sub.  
    
