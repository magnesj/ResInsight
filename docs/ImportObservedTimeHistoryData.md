---
layout: docs
title: Import Observed Time History Data
permalink: /docs/importobservedtimehistorydata
published: true
---

Importing observed time history data to ResInsight may be performed in two different ways:
- By selecting the main menu item **File -> Import -> Import Observed Time History Data**
- By right clicking the project tree item **Observed Time History Data and** and selecting **Import Observed Time History Data**

The following file types are supported:
- RSM observed time history data file (\*.rsm)
- Column based (Comma Separated Values, CSV) time history data file (\*.csv/\*.txt)

## Import RSM observed time history data
To import RSM files, the only action needed from the user is to select one or more RSM files. When the import is finished, one tree node for each file will appear under the *Observed Time History Data* node in the project tree.

## Import CSV/txt observed time history data
CSV/txt files are generic ascii files which may have slightly different formatting. When importing these types of files the user is presented a dialog, where the user may tell ResInsight how to import the selected file(s). The dialog appears once for each imported file.

### CSV/txt import dialog
![]({{ site.baseurl }}/images/ImportObservedTimeHistoryDataDialog.png)

Dialog fields description:
- **Cell Separator** -- Select the correct cell separator character. ResInsight will try to set the correct value as default.
- **Decimal Separator** -- Select the correct decimal separator. ResInsight will try to set the correct value as default.
- **Selected Time Column** -- Select the column that contains the time/date information. The first column is default.
- **Use Custom Date Time Format** -- Check this box if the Date Format and/or Time Format in the file do not match any of the predefined formats.
- **Custom Date Time Format** -- Enter date time format to match the time information in the file. This field is visible only when the above check bos checked.
- **Date Format** -- Select the date format matching the date information in the file.
- **Time Format** -- Select the time format matching the time information in the file.
- **Preview** -- Preview the first 30 lines of the file contents. The view will reflect the currently selected Cell Separator and the selected time column is marked in yellow.


-------
The files generated are stored in a folder named _`snapshots`_ within the folder where the Project File resides. 

<div class="note">
 Snapshots of existing views can also be created and saved from the command line. 
 ( See <a href="{{ site.baseurl }}/docs/commandlineparameters">Command Line Arguments</a> )
</div>
