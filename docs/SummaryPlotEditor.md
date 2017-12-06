---
layout: docs
title: Summary Plot Editor
permalink: /docs/summaryploteditor/
published: true
---

The plot editor is a separate ResInsight dialog window where the user can select which vectors to display in a summary plot. It is also possible to edit an existing plot in this editor.

The upper part of the editor contains editors for selecting which vectors/summaries to display in the plot. The number of fields vary from 3 to 6 depending on the summary type currently highlighted. Dynamic field are hidden if the highlighted summary category does not have any sub-fields.

<div class="note">
In this context <b>highlight</b> means a marked field item that has a light blue background (only one at a time), while <b>selected</b> means an item or items that have a ticked check box.
</div>

## Selection Fields
This section describes the different selection fields in the selection part of the plot editor. A complete/valid vector selection consists of a selected source, a selected summary category, a selected item in each dynamic field (if any) and a selected vector/summary.

### Sources
This field contains all imported cases. Select the case(s) to display in the plot.

### Summary Types
This field contains all fixed summary types and two special ones, *Calculated* and *Imported*. 

- **Field** -- Select Field related vectors only
- **Aquifer** -- Select Aquifer category vectors only 
- **Network** -- Select Network category vectors only  
- **Misc** -- Select vectors in the Misc category only 
- **Region** -- Select Region related vectors only  
   - **Region number** -- Select the Region number
- **Region-Region** -- Select Region to Region related vectors only  
   - **Region numbers** -- Select the Region to Region numbers
- **Group** - Select Group related vectors only
   - **Group name** --  Select Group name
- **Well** -- Select Well related vectors only
   - **Well name** --  Select Well name
- **Completion**   -- Select Completion related vectors only
   - **Well name** --  Select Well name
   - **I, J, K** -- Select the I, J, K values of the completion
- **Lgr-Completion** -- Select Completion in LGR related vectors only
   - **Well name** --  Select Well name
   - **Lgr name** -- Select Lgr name
   - **I, J, K** -- Select the I, J, K values of the completion in the Lgr.
- **Lgr-Well** -- Select Well in LGR related vectors only
   - **Well name** -- Select Well name
   - **Lgr name** -- Select Lgr name 
- **Segment** -- Select Segment related vectors only
   - **Well name** -- Select Well name
   - **Segment number** -- Select the segment number
- **Block** -- Select I, J, K -- Block related vectors only 
   - **I, J, K** -- Select the I, J, K values of the Block. 
- **Lgr-Block** -- Select I, J, K - Block in LGR related vectors only
   - **Lgr name** -- Select Lgr name
   - **I, J, K** -- Select the I, J, K values of the Block in the Lgr. 
- **Calculated** -- Select calculated vectors created by the [curve calculator]({{site.baseurl}}/docs/curvecalculator).
- **Imported** -- Select observed data vectors [imported from file]({{site.baseurl}}/docs/importobstimehistdata) (e.g. CSV, RSM files)

### Dynamic Fields
In the list above some of the summary categories have one or more sub fields which is displayed when that category is highlighted. See previous section for a description of those fields.

### Summaries
This field contains the summaries/vectors for the highlighted summary category.

## 
