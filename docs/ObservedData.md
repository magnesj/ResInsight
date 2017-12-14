---
layout: docs
title: Observed Data
permalink: /docs/observeddata/
published: true
---

Observed data, or *Observed Time History Data*, is data measured in time. Observed data can be plotted along with summary data in **Summary Plots**.

## Import Observed Data

Observed Data files can be imported by using the command: **File->Import->Import Observed Time History Data**, or by using the context command of ![]({{ site.baseurl }}/images/Folder.png)**Observed Time History Data** in the **Plot Object Project Tree**. The user is asked to select _`*.RSM`_, _`*.txt`_, _`*.csv`_, or _`*.inc`_ files for import.

The imported ![]({{ site.baseurl }}/images/Default.png) Observed Data files will be added to ![]({{ site.baseurl }}/images/Folder.png)**Observed Time History Data**. 

Which summaries that has been detected in a Observed data file can be read in an Observed data's **Property Editor**. In the image below, time and year info has been found together with the summary "WBP9L" for the well "OP-1".

![]({{ site.baseurl }}/images/observedDataProperty.png)

### TODO: file format

## Viewing Observed Data

![]({{ site.baseurl }}/images/observedDataCurveCreator.png)

To plot Observed Data, choose **New Summary Plot** in the context menu of **Summary Plots**, in **Plot Object Project Tree**. Observed data will appear in **Sources** together with summary cases. How to use the Plot editor is covered in [Summary Plot Editor]({{site.baseurl}}/docs/summaryploteditor). Observed data points are plotted as crosses by default.
