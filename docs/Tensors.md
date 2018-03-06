---
layout: docs
title: Tensor Results
permalink: /docs/tensors/
published: true
---

![]({{ site.baseurl }}/images/tensorArrows.png)

Tensors are arrows showing the average principal vectors in an element, shown on every visible face of the element.

The tensor results editor is found in a geo mechanical model's **View** in the project tree as seen below.

![]({{ site.baseurl }}/images/tensorProjectTree.png)

## Visualization

Arrows. Pressure/tension

## Properties

![]({{ site.baseurl }}/images/tensorPropEditor.png)

**Value**. Tensor Results of an element can be calculated from one of the three result values *SE, ST* and *E*.

**Visibility**. Choose which of the three principals to be shown. The threshold removes all principals with an absolute value less than or equal to the threshold value.

**Vector Colors**. Choose which color palette to use for the three arrows. The colors appear in "correct" order (first color = principal 1). 

The vector color **Result Colors** is special. By choosing this color type, a new legend will appear. This legend is defined by the values in the Legend definition of the Element Tensor Results. The extremal values of the color mapper are the extremals of the three principals combined. 

**Vector Size**. Scale method **Result** scales the arrows relative to the maximum result value of all components in the model. With scale method **Constant**, the arrows are of equal size. The arrow size can be both minified and magnified by using **Size Scale**.

