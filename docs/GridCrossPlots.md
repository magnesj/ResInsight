---
layout: docs
title: Grid Cross Plots
permalink: /docs/gridcrossplots/
published: true
---

![]({{ site.baseurl }}/images/GridCrossPlot.png)

ResInsight supports the creation of scatter / cross plots of two Eclipse results against each  other, with each cell in the grid
representing one data point in the plot. The data points can be grouped by a third result, by time step or by **Formations**.
giving a separate color and label for each group. The above example shows a classic Porosity vs Permeability plot, grouped by
formations, showing different trends for each formation.

For continuous grouping parameters, the parameter will be divided into a set of equally sized intervals depending on the number of
Color Legend levels.

## Creating Grid Cross Plots
![]({{ site.baseurl }}/images/GridCrossPlot_CreateFromView.png)
![]({{ site.baseurl }}/images/GridCrossPlot_Create.png)

Grid Cross Plots can be created in a couple of ways:
1. Select a 3D view or Cell Result in the main ResInsight window, right-click and select **Create Grid Cross Plot from 3d View**. The resulting cross plot will display the current 3d Result x DEPTH and only contain data points for the visible cells.
2. Right-click on the **Grid Cross Plots** entry under **Plots** in the **Plot Window**. By default the plot will contain the result values PORO x PERMX and will be grouped by formations (if any are loaded in the project). Data for all active cells will be displayed.
