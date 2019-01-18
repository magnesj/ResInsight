---
layout: docs
title: Measurement
permalink: /docs/measurement/
published: true
---

![]({{ site.baseurl }}/images/Measurement.png)

ResInsight supports measurements in the 3D views. To enter measurement mode, press the ruler toolbar button ![]({{site.baseurl}}/images/MeasurementButton.png). This mode can also be activated from the context menu in a 3D view.

When ResInsight is in measurement mode, clicking on an surface in the 3D view will set the first measurement point. Clicking on a different surface will set the second measurement point, and display a label with measurements. Additional clicking will start a new measurement between two points.

The measurement label contains the following:
- **Length** - The length of the measurement segment
- **Horizontal Length** - The length of the measurement segment projected onto the XY plane

If **CTRL** button is pressed during clicking, multiple points can be added to create a polyline. The measurement label will now contain additional measurements.

The measurement label contains several lengths.
- **Segment Length** - The length of the last segment
- **Segment Horizontal Length** - The length of the last segment projected onto the XY plane
- **Total Length** - The total length of the measurement polyline
- **Total Horizontal Length** - The total length of the measurement polyline projected onto the XY plane
- **Horizontal area** - The area of the polyline projected onto the XY plane

To leave measurement mode, press the toolbar button or press the **Esc** button.
