---
layout: docs
title: Annotations
permalink: /docs/annotations/
published: true
---

ResInsight supports displaying a few types of annotations in 3D views and Contour Map view.
- Text annotations
- Reach circle annotations
- Polyline annotations
  - User defined polylines
  - Polylines imported from file

![]({{ site.baseurl }}/images/Annotations.png)

## Global scope vs local scope annotations
Global annotations may be displayed in all views and are located in the **Annotations** project tree node right below **Grid Models**. Local annotations are associated with a specific view and are located in the **Annotations** project tree node below the view node. All annotation types except text annotations are global only. Text annotation may be either global or local.

All global annotations also have a representation in the local **Annotation** tree node in order to toggle visibilty per view. Those anotations are located in tree nodes starting with **Global**.

![]({{ site.baseurl }}/images/LocalAnnotationsTree.png)
![]({{ site.baseurl }}/images/GlobalAnnotationsTree.png)

Local and global annotations in the project tree

## Text Annotations
There are two ways of creating a new text annotation.
- Right click **Annotations** or **Text Annotations** tree node. The scope of the annotation depends on which node was clicked, one of the global nodes or one og the local nodes. In this case alle text annotation fields must be entered in the property editor.
- Right click on an object in the view and select **Create Text Annotation**. ResInsight will then create a text annotation at the clicked point. The text must be entered in the property editor. When creating a text annotation this way, it will become a local annotation by default.

![]({{ site.baseurl }}/images/TextAnnotationPropertyEditor.png)

- **Anchor Point** - The interesting point in the view
- **Label Point** - The point where the text label is placed
- **Text** - The text to display. Multiline supprted. The first line will be the name of the annotation in the project tree
- **Text appearance** - Set font size, font color, background color and anchor line color

When a text annotation tree node is selected, target markers in each end of the anchor line are displayed. The targets can be clicked and dragged. Clicking the blue part lets the user drag the target vertically (along Z axis). Clicking the purple part lets the user drag the target in the XY plane.

## Reach Circle Annotations
To create a reach circle annotation, right click **Annotations** or **Reach Circle Annotations** tree node in the global part of the project tree. Then enter values in the property editor.

![]({{ site.baseurl }}/images/CircleAnnotationPropertyEditor.png)

- **Name** - Name of the circle annotation
- **Center Point** - Center point of the circle. Format 'x-pos y-pos depth'
- **Radius** - Circle radius
- **Line Appearance** - Set circle color and line thickness




