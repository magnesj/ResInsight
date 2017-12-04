---
layout: docs
title: Import Observed Time History Data
permalink: /docs/importobservedtimehistorydata
published: true
---

Importing observed time history data to ResInsight may be performed in two different ways:
- By selecting the main menu item *File -> Import -> Import Observed Time History Data*
- By right clicking the project tree item *Observed Time History Data and* and selecting *Import Observed Time History Data*

The following file types are supported:
- RSM observed time history data file (\*.rsm)
- Column based (Comma Separated Values, CSV) time history data file (\*.csv/\*.txt)

## Import RSM observed time history data
To import RSM files, the only action needed from the user is to select one or more RSM files. When the import is finished, one tree node for each file will appear under the *Observed Time History Data* node in the project tree.

## Import CSV/txt observed time history data
CSV files may have slightly different formatting




Under construction
---------------------------------

ResInsight has several commands to create snapshots conveniently. 3 commands to take snapshots of existing Plot and 3D Views directly, and a more advanced export command that can automatically modify Eclipse 3D Views before snapshotting them. 

## Snapshots of Existing Views

The commands to snapshot existing views and plots are available from the toolbar and the **Edit** and **File**->**Export** menus in the main windows

![]({{ site.baseurl }}/images/SnapShotToolBar.png)

### Snapshot to Clipboard ![]({{ site.baseurl }}/images/SnapShot.png)

A snapshot of the active view is copied to the clipboard using **Edit -> Copy Snapshot To Clipboard**.

### Snapshot to File ![]({{ site.baseurl }}/images/SnapShotSave.png)

Image export of the currently active 3D View or Plot Window can be launched from **File -> Export -> Snapshot To File**. 

### Snapshot All Views/Plots to File ![]({{ site.baseurl }}/images/SnapShotSaveViews.png)

If a project contains multiple 3D Views or Plot Windows, all of them can be exported in one go using **File -> Export -> Snapshot All Views To File**. This will either export all the 3D Views or all the Plot Windows, depending on whether you invoke the command in the 3D Main Window or the Plot Main Window.

The files generated are stored in a folder named _`snapshots`_ within the folder where the Project File resides. 

<div class="note">
 Snapshots of existing views can also be created and saved from the command line. 
 ( See <a href="{{ site.baseurl }}/docs/commandlineparameters">Command Line Arguments</a> )
</div>
