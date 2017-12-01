---
layout: docs
title: Formations
permalink: /docs/formations/
published: true
---

![]({{ site.baseurl }}/images/formations_legend.PNG)

## Formations for k-layers

Formation information can be utilized in ResInsight as cell colors, used in property filters and are displayed in the **Result info** panel when selecting single cells.

To use this functionality you will need to :

1. Import one or more Formation Names file(s)
2. Select the correct Formation Names file in the Case of interest

<div class="note info">
If only one formation file is imported, the formation will automatically be selected in the active view's case.
</div>

### Import of Formation Names files

Formation Names files can be imported by using the command: **File->Import->Import Formation Names**.
The user is asked to select _`*.lyr`_ files for import.

The imported Formation Names files are then listed in the **Project Tree** in a folder named **Formations**. 

Formation Names files consists of a list of formation names and their k-range. Below is an example of a Formation Names file:

```
-- Any text as comment
'MyFormationName'                 4 - 12
'MySecondFormationName'          15 - 17
'3 k-layer thick 18,19 and 20'    3
'Last Name'                      21 - 21 
```

### Select the Formation file in a Case
To make the Formation information available for viewing, you have to select which of the Formation files to be used for a particular case.

![]({{ site.baseurl }}/images/formations_property_editor.PNG)

This option is available in the **Property Editor** for a case. The formation is selected in the combo box for property **Formation Names File**.

#### Reload of formation data
If the formation file is modified outside ResInsight, the formation data can be imported again by the context menu **Formations->Reload**. This command will import formations for the selected formation files.

### Viewing the Formation Information

#### Formations in 3D view
The formations can be visualized as a result property in **Cell Results**, **Cell Edge Result**, and **Separate Fault Result**. When selected, a special legend displaying formation names is activated.

#### Property filter based on formations
Formation names are available in Property Filters as Result Type **Formation Names**. This makes it easy to filter geometry based on formation specifications.

See [ Cell Filters ]({{ site.baseurl }}/docs/filters) for details.

#### Picking in 3D view
Picking on a cell being part of a formation will display the formation name in the **Result Info** windows, in addition to other pick info for the cell.


## Formations for a Well Path
Formations can be set for a single well path, defined on measured depths of the well path. This formation can be used to annotate the following plot types:
- Well Log Plots
- RFT Plots
- PLT Plots
- Well Allocation Plots

### Import of Well Path Formation Names files

Well Path Formation Names files can be imported by using the command: **File->Import->Import Well Path Formation Names**.
The user is asked to select _`*.csv`_ files for import.

The imported Well Path Formation Names files will be added their associated well path, if a match on well name can be found. If not, new paths will be created, and they can all be found in **Wells** in the **Project Tree**. The file path of the formations can be found in a well path's **Property Editor**.

![]({{ site.baseurl }}/images/wellPathFormationsInPropertyEditor.PNG)

A Well Path Formation Names file is a csv-file, which uses semicolon to separate entries in a table. Below is an example of such a file:

```
Well name; Surface name; Geologic feature; CF; Obs#; Qlf; Kind; MD; TVD; TVDSS; TWT;
B-2H; Formation A Top; UNKNOWN; P;1; ; ;3000;2900;-1850;2000;
B-2H; Formation A1 Top; UNKNOWN; P;1; ; ;3000;2900;-1850;2000; 
B-2H; Formation B Top; UNKNOWN; G;1; ; ;3100;3000;-1950;2000;
B-2H; Formation C Top; UNKNOWN; G;1; ; ;3200;3100;-2050;2000;
B-3H; Fname 2; UNKNOWN; G;1; ; ;3100;3000;-1950;2000;
B-3H; Fname 3.2; UNKNOWN; G;1; ; ;3400;3300;-2050;2000;
B-3H; Fname 4; UNKNOWN; G;1; ; ;3440;3340;-2060;2010;
```

The file must have the columns "Well name", "Surface name" (i.e. formation name) and "MD" (i.e. measured depth) to be regarded as a Well Path Formation Names file. They can be listed in any order, and all other columns will be ignorded.
