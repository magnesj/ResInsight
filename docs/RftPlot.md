---
layout: docs
title: RFT Plot
permalink: /docs/rftplot/
published: true
---

An RFT plot is a special well plot displaying pressure data against true vertical depth (TVD). RFT data may be a part of the grid model or may be loaded from well log files (\*.LAS) and well path files (\*.dev, \*.json etc.).

![Screen Dump]({{site.baseurl}}/images/Dummy.png)

## Create New RFT Plot
There are several ways to create new RFT Plots
- Right-click a Well Path under Wells in the project tree and then select **New RFT Plot**.
- Right-click a simulation well in the 3D view and select **Well Plots -> New RFT Plot**.
- Right-click the RFT Plots node in the project tree and select **New RFT Plot**.

## Restrictions
If an imported well log file does not contain pressure data, that data source (_Observed data_) will not be visible in the sources field in the property editor. Pressure data in a well log file is expected to have a column named _PRESSURE_ or _PRES_FORM_. If the well log contains pressure data, but has no infomation about TVD, a warning will appear when a curve is to be displayed. TVD may be available either as an explicit TVD column named _TVDMSL_ in the well log file or a separately imported well path file.


## Property Editor


