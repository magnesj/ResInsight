---
layout: docs
title: Summary Cross Plot
permalink: /docs/summarycrossplots/
published: true
---

![]({{site.baseurl}}/images/SummaryCrossPlot.png)

A Summary Cross Plot is a window displaying a graph in the main area of the **Plot Main Window**. It is very similar to [Summary Plots]({{site.baseurl}}/docs/summaryplots), but there are some differences:

- Summary Cross Plot displays one vector against another vector, not one vector against time
- Not possible to paste Excel/CSV data to a summary cross plot
- Summary cross plots have no [Plot Editor]({{site.baseurl}}/docs/summaryploteditor)

To create a new Summary Cross Plot, select the context command ![]({{ site.baseurl }}/images/SummaryPlot16x16.png) **New Summary Cross Plot** on the **Plot Main Window -> Project Tree -> Summary Cross Plots** item. 

## Summary Cross Plot Curves
New Summary Cross Plot curves are created by using the context command **New Summary Cross Plot Curve** on a summary cross plot. To be able to display a Summary Cross Plot curve, ResInsight needs two data vectors. Vector selections are made using the ** Summary Cross Plot Curve** property editor.

![]({{site.baseurl}}/images/SummaryCrossPlotCurvePropertyEditor.png)

The property editor in an ordinary summary plot has one editor group called **Summary Vector** for selecting the Y vector. In the property editor for a summary cross plot, this group has been replaced by two vector selection groups **Summary Vector Y** and **Summary Vector X**. They still works exactly the same way as in the ordinary summary plot. See the [detailed description]({{site.baseurl}}/docs/summaryplots#summary-curves)
