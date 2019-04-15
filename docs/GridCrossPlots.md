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
