---
layout: docs
title: Annotations
permalink: /docs/annotations/
published: true
---

ResInsight supports displaying a few types of annotations in 3D views and Contour Map view. Those are:
- Text annotations
- Reach circle annotations
- Polyline annotations
  - User defined polylines
  - Polylines imported from file

![]({{ site.baseurl }}/images/Annotations.png)

## Global vs local annotations
Global annotations may be displayed in all views and are located in the **Annotations** project tree node right below **Grid Models**. Local annotations are associated with a specific view and are located in the **Annotations** project tree node below the view node. All annotation types except text annotations are global only. Text annotation may be either global or local.

All global annotations also have a representation in the local **Annotation** tree node in order to toggle visibilty per view. Those anotations are located in tree nodes starting with **Global**.

![]({{ site.baseurl }}/images/LocalAnnotationsTree.png)
![]({{ site.baseurl }}/images/GlobalAnnotationsTree.png)

Local and global annotations in the project tree



