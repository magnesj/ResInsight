---
layout: docs
title: Contour Maps
permalink: /docs/contourmaps/
published: true
---


![]({{ site.baseurl }}/images/ContourMaps.png)

ResInsight can create contour maps based on different forms of aggregation of 3D Eclipse data onto a 2D Plane. Any 3D result value can be aggregated, in addition to specialised results, such as Oil, Gas and Hydrocarbon columns. A Contour Map is a specialised 2D view with many of the same features as the 3D views, including property filters, range filters and display of faults and wells.

## Creating New Contour Maps

Contour Maps can be created in many different ways:

- New Contour Map from the context menu of case or the **Contour Maps** project tree item underneath the case. These will create contour maps with default values.
- New Contour Map from 3d View in the Eclipse View context menu. This will create a contour map based on the existing 3d View with matching filters and result.
- Duplicate Contour Map from the context menu of an existing Contour Map. This will copy the existing map.

![]({{ site.baseurl }}/images/NewContourMapsFromCase.png)
![]({{ site.baseurl }}/images/NewContourMapsFromFolder.png)
![]({{ site.baseurl }}/images/NewContourMapsFromView.png)
![]({{ site.baseurl }}/images/NewContourMapsFromExisting.png)

## Properties of the Contour Maps

A contour Map has many of the same options available as a 3D View, but is always orthographic/parallel projection with no perspective projection or lighting available. Instead of the 3D Grid Box, the Contour Maps uses a 2D Grid simular to the **2d Intersection Views** with optional Axis Lines controlled with the **Show Axis Lines** toggle. The name of the map can be automatically generated from the Case Name, Property Type, Aggregation Type and Sample Spacing (See **Map Projection Properties** for the two latter).

![]({{ site.baseurl }}/images/ContourMapViewProperties.png)

## Map Projection Properties

The Map Projection settings control how the 3D Data is aggregated onto the 2D plane.

![]({{ site.baseurl }}/images/ContourMapProjectionProperties.png)
