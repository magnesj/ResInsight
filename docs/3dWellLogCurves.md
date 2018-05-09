---
layout: docs
title: 3D Well Log Plots
permalink: /docs/3dwelllogcurves/
published: true
---

ResInsight can display well logs curves directly in the 3d view with a subset of the functionality of the full [2D Well Log Plot Editor]({{ site.baseurl }}/docs/welllogsandplots). The curves will be drawn in a plane next to or centered on the well trajectory they belong to and can display well log data from a simulation model and from imported LAS-files.

![]({{ site.baseurl }}/images/3dWellLogCurves.png)

# Creating new 3D Well Log Curves
Add a new 3D well log curve by selecting a Well Path in the Project Tree, right-clicking and selecting "3D Well Log Curves" and "Add 3D Well Log Curve".

![]({{ site.baseurl }}/images/3dWellLogCurvesCreate.png)

# Deleting 3D Well Log Curves
Delete one or more 3D well log curves by selecting them in the project tree, right-clicking on one of the selected items and choosing "Delete 3D Well Log Curve(s)". The curves will be deleted with no further confirmation.

![]({{ site.baseurl }}/images/3dWellLogCurvesDelete.png)

# Configurating the 3D Well Log Curves

## 3D Track and Draw Plane Appearance
Each 3D well log curve belongs to a 3D Track which in turn is attached to a well path. The 3D track itself contains some settings related to the display of the curves:

| Parameter      | Description                                                                | Range        |
|----------------|----------------------------------------------------------------------------|--------------|
| Width Scaling  | A scaling factor applied to the width of the draw surfaces                 | [0.25 , 2.5] | 
| Show Grid      | Show axis markers for the value-axis of the curves along the draw surface  | True / False |
| Show Background| Draw a white background on the drawing surfaces of the curves              | True / False |


![]({{ site.baseurl }}/images/3dWellLogCurves_DrawPlaneAppearance.png)

## Curve Data Selection
The Curve Data configuration allows you to select a case, result type, property and time step to plot.
![]({{ site.baseurl }}/images/3dWellLogCurves_CurveData.png)

## Curve Configuration
![]({{ site.baseurl }}/images/3dWellLogCurves_CurveConfiguration.png)

## Selecting a Drawing Plane for the Curve
![]({{ site.baseurl }}/images/3dWellLogCurves_DrawPlaneSelection.png)
