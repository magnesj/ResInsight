---
layout: docs
title: Create Well Paths
permalink: /docs/createwellpaths/
published: true
---

ResInsight lets the user create new/custom well paths by clicking in the 3D view. A self created well path will behave the same way as an ordinary imported well path.

To create a well path:
1. Right click **Wells** in the project tree
2. Select **Create Well Path** in the context menu. A new well node and a well targets node are created.
3. Click in the 3D view on locations where the well path will pass (targets). Note. A 3D object must be hit when clicking. Clicking in thin air will not work.
4. When finished placing targets, click on "Stop Picking Targets" in the property editor

![]({{ site.baseurl }}/images/WellTargetsPropertyEditor.png)

A self created well path may be edited by either editing coordinates in the property editor or clicking and dragging targets in the 3D view. 

![]({{ site.baseurl }}/images/WellTargets.png)

Clicking and dragging the blue part of a target, it can be moved along the Z axis only. Clicking and dragging the magenta part of a target, it can be moved freely around.
