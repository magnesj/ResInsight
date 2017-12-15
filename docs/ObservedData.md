---
layout: docs
title: Observed Data
permalink: /docs/observeddata/
published: true
---

Observed data, or *Observed Time History Data*, is data measured in time. On import of observed data, ResInsight translates the data to make it similar to summary data. Observed data can be plotted along with summary data in **Summary Plots**.

## Import Observed Data

Observed Data files can be imported by using the command: **File->Import->Import Observed Time History Data**, or by using the context command of ![]({{ site.baseurl }}/images/Folder.png)**Observed Time History Data** in the **Plot Object Project Tree**. The user is asked to select _`*.RSM`_, _`*.txt`_, or _`*.csv`_ files for import.

The imported ![]({{ site.baseurl }}/images/Default.png) Observed Data files will be added to ![]({{ site.baseurl }}/images/Folder.png)**Observed Time History Data**. 

Which summaries that has been detected in a Observed data file can be read in an Observed data's **Property Editor**. In the image below, time and year info has been found together with the summary "WBP9L" for the well "OP-1".

![]({{ site.baseurl }}/images/observedDataProperty.png)

## File Formats

# TODO: Move observed time history data under export and interfaces?

### Column Based File Format

If a column based file is presented, ResInsight first tries to identify if its header has fixed witdh or not. Further, the header is interpreted by looking for specific lines.

The first line must have one or more vector mnemonics. The initial letter(s) in a mnenomic specify which summary data type the column represents. For instance, *FVPT* and *FWPT* are of type *Field*, as they both have an initial *F*. *WWCTH* and *WGORH* are well data types. See *Vector naming convention* in *Eclipse: File Formats Reference Manual* for a full overview of supported mnenomics.

The next lines can define units, well/group names, region names, LGR names and block numbers and the local cell number. They do not have to appear in any particular order. Scale factors can also be included, but will be ignored by ResInsight. All lines starting with _*--*_ will also be ignored.

#### Column Based with Fixed Header Width

When interpreting column based files with fixed header width, ResInsight looks for left aligned column entries. These type of files are interpreted as we natuarlly read them. More than one table can be present in each file.

```
1                                                                                                      
 -------------------------------------------------------------------------
 SUMMARY
 -------------------------------------------------------------------------
 DATE         FGIRH        FVPT         FWPT         FOPT         FLPT  
              SM3/DAY      RM3          SM3          SM3          SM3    
                           *10**3       *10**3       *10**3       *10**3 
                                                                         
                                                                         
 -------------------------------------------------------------------------
  6-NOV-1997         0            0            0            0            0 
  7-NOV-1997         0     5.749954     0.004548     4.379801     4.384350  
  8-NOV-1997         0     13.76883     0.010841     10.48852     10.49936
  9-NOV-1997         0     19.38692     0.015243     14.76847     14.78372  
 10-NOV-1997         0     24.07161     0.018914     18.33751     18.35642
 11-NOV-1997         0     28.79427     0.022613     21.93558     21.95819
```

#### Column Based with Random Header Width

Column Based with Random Header Width will try to be parsed in the same way as fixed width, but it might fail in situations like the one below. We can see that SM3/SM3 *probably* belongs to WGORH, but it is parsed to WWCTH, as it is the second entry on that line.

```
TIME      WWCTH      WGORH
DAYS               SM3/SM3          

          A-5HP          A-5HP
     1     0.000   0.000
     2     0.000   0.000
     3     0.000   0.000
```

### Keyword Based File Format

If the non-comment line inlcudes the word "VECTOR", the file is interpreted as a keyword based file. In keyword based files, the content of the one-column tables is described in each header. Tables should be assosiated with a table containing time stamps. In the example below, *S-1AH-GOR* is assosiated with *YEARX*, since their origin is equal. ResInsight always interpret *ORIGIN* as well name, and look for a table with the line "VECTOR YEARX" to assosiate with it.

```
----------------------------------------------
-- GOR data 
----------------------------------------------
VECTOR S-122AH-GOR
UNITS SM3/SM3
ORIGIN GORS-122AH
330.6601
257.7500
335.9894
301.4388
260.4193
306.0298
280.2883

VECTOR YEARX
ORIGIN GORS-112AH
UNITS YEAR
1999.7902
1999.8446
1999.9285
2000.0391
2000.0800
2000.0862
2000.1285
---comment


----------------------------------------------
-- GOR data
----------------------------------------------
VECTOR S-211H-GOR
UNITS SM3/SM3
ORIGIN GORS-211H
293.8103
293.1634
304.0000
334.5932
306.4610
293.7571

VECTOR YEARX
ORIGIN GORS-22H
UNITS YEAR
1999.8255
2000.1274
2000.2075
2000.2367
2000.4033
2000.4966
```

Please seek "User data file formats" in **Eclipse: File Formats Reference Manual** for details.

## Viewing Observed Data

![]({{ site.baseurl }}/images/observedDataCurveCreator.png)

To plot Observed Data, choose **New Summary Plot** in the context menu of **Summary Plots**, in **Plot Object Project Tree**. Observed data will appear in **Sources** together with summary cases. How to use the Plot Editor is covered in [Summary Plot Editor]({{site.baseurl}}/docs/summaryploteditor). Observed data points are plotted without lines by default.
