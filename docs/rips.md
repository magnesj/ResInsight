# rips package

## Submodules

## rips.App module


#### class rips.App.App(channel)
Bases: `object`

ResInsight application information and control.
Allows retrieving of information and controlling the running instance
Not meant to be constructed manually, but exists as part of the Instance method


#### exit()
Tell ResInsight instance to quit


#### isConsole()
Returns true if the connected ResInsight instance is a console app


#### isGui()
Returns true if the connected ResInsight instance is a GUI app


#### majorVersion()
Get an integer with the major version number


#### minorVersion()
Get an integer with the minor version number


#### patchVersion()
Get an integer with the patch version number


#### versionString()
Get a full version string, i.e. 2019.04.01

## rips.Case module


#### class rips.Case.Case(channel, id)
Bases: `object`

ResInsight case class

Operate on a ResInsight case specified by a Case Id integer.
Not meant to be constructed separately but created by one of the following
methods in Project: loadCase, case, allCases, selectedCases


#### id()
Case Id corresponding to case Id in ResInsight project.


* **Type**

    int



#### name()
Case name


* **Type**

    string



#### groupId()
Case Group id


* **Type**

    int



#### cellCount(porosityModel='MATRIX_MODEL')
Get a cell count object containing number of active cells and
total number of cells


* **Parameters**

    **porosityModel** (*string*) – String representing an enum.
    must be ‘MATRIX_MODEL’ or ‘FRACTURE_MODEL’.



* **Returns**

    active_cell_count: number of active cells
    reservoir_cell_count: total number of reservoir cells



* **Return type**

    Cell Count object with the following integer attributes



#### cellInfoForActiveCells(porosityModel='MATRIX_MODEL')
Get Stream of cell info objects for current case


* **Parameters**

    **porosityModel** (*string*) – String representing an enum.
    must be ‘MATRIX_MODEL’ or ‘FRACTURE_MODEL’.



* **Returns**

    grid_index(int): grid the cell belongs to
    parent_grid_index(int): parent of the grid the cell belongs to
    coarsening_box_index(int): the coarsening box index
    local_ijk(Vec3i: i(int), j(int), k(int)): local cell index in i, j, k directions.
    parent_ijk(Vec3i: i(int), j(int), k(int)): cell index in parent grid in i, j, k.



* **Return type**

    Stream of cell info objects with the following attributes



#### daysSinceStart()
Get a list of decimal values representing days since the start of the simulation


#### grid(index)
Get Grid of a given index. Returns a rips Grid object


* **Parameters**

    **index** (*int*) – The grid index


Returns: Grid object


#### gridCount()
Get number of grids in the case


#### grids()
Get a list of all rips Grid objects in the case


#### timeSteps()
Get a list containing time step strings for all time steps

## rips.Commands module


#### class rips.Commands.Commands(channel)
Bases: `object`

Command executor which can run ResInsight Command File commands nearly verbatim

Documentation Command File Interface:

    [https://resinsight.org/docs/commandfile/](https://resinsight.org/docs/commandfile/)

The differences are:

    1. Enum values have to be provided as strings. I.e. “ALL” instead of ALL.

    1. Booleans have to be specified as correct Python. True instead of true.


#### closeProject()
Close the current project (and reopen empty one)


#### computeCaseGroupStatistics(caseIds)

#### createLgrForCompletions(caseId, timeStep, wellPathNames, refinementI, refinementJ, refinementK, splitType)

#### createMultipleFractures(caseId, templateId, wellPathNames, minDistFromWellTd, maxFracturesPerWell, topLayer, baseLayer, spacing, action)

#### createSaturationPressurePlots(caseIds)

#### exportMsw(caseId, wellPath)

#### exportMultiCaseSnapshots(gridListFile)
Export snapshots for a set of cases


* **Parameters**

    **gridListFile** (*string*) – Path to a file containing a list of grids to export snapshot for



#### exportProperty(caseId, timeStep, property, eclipseKeyword=<class 'property'>, undefinedValue=0.0, exportFile=<class 'property'>)
Export an Eclipse property


* **Parameters**

    * **caseId** (*int*) – case id

    * **timeStep** (*int*) – time step index

    * **property** (*string*) – property to export

    * **eclipseKeyword** (*string*) – Eclipse keyword used as text in export header. Defaults to the value of property parameter.

    * **undefinedValue** (*double*) – Value to use for undefined values. Defaults to 0.0

    * **exportFile** (*string*) – Filename for export. Defaults to the value of property parameter



#### exportPropertyInViews(caseId, viewNames, undefinedValue)

#### exportSimWellFractureCompletions(caseId, viewName, timeStep, simulationWellNames, fileSplit, compdatExport)

#### exportSnapshots(type='ALL', prefix='')
Export snapshots of a given type


* **Parameters**

    * **type** (*string*) – Enum string (‘ALL’, ‘VIEWS’ or ‘PLOTS’)

    * **prefix** (*string*) – Exported file name prefix



#### exportVisibleCells(caseId, viewName, exportKeyword='FLUXNUM', visibleActiveCellsValue=1, hiddenActiveCellsValue=0, inactiveCellsValue=0)

#### exportWellPathCompletions(caseId, timeStep, wellPathNames, fileSplit, compdatExport, includePerforations, includeFishbones, excludeMainBoreForFishbones, combinationMode)

#### exportWellPaths(wellPaths=[], mdStepSize=5.0)

#### loadCase(path)
Load a case


* **Parameters**

    **path** (*string*) – path to EGRID file



* **Returns**

    A Case object



#### openProject(path)
Open a project


* **Parameters**

    **path** (*string*) – path to project file



#### replaceCase(newGridFile, caseId=0)
Replace the given case with a new case loaded from file


* **Parameters**

    * **newGridFile** (*string*) – path to EGRID file

    * **caseId** (*int*) – case Id to replace



#### replaceSourceCases(gridListFile, caseGroupId=0)
Replace all source cases within a case group


* **Parameters**

    * **gridListFile** (*string*) – path to file containing a list of cases

    * **caseGroupId** (*int*) – id of the case group to replace



#### runOctaveScript(path, cases)

#### scaleFractureTemplate(id, halfLength, height, dFactor, conductivity)

#### setExportFolder(type, path, createFolder=False)

#### setFractureContainment(id, topLayer, baseLayer)

#### setMainWindowSize(width, height)

#### setStartDir(path)
Set current start directory


* **Parameters**

    **path** (*string*) – path to directory



#### setTimeStep(caseId, timeStep)
## rips.Grid module


#### class rips.Grid.Grid(index, case)
Bases: `object`

Grid Information. Not meant to be constructed separately

Create Grid objects using mathods on Case: Grid() and Grids()


#### dimensions()
The dimensions in i, j, k direction


* **Returns**

    class with integer attributes i, j, k representing the extent in all three dimensions.



* **Return type**

    Vec3i


## rips.Instance module


#### class rips.Instance.Instance(port=50051)
Bases: `object`

The ResInsight Instance class. Use to launch or find existing ResInsight instances


#### launched()
Tells us whether the application was launched as a new process.
If the application was launched we may need to close it when exiting the script.


* **Type**

    bool



#### app()
Application information object. Set when creating an instance.


* **Type**

    App



#### commands()
Command executor. Set when creating an instance.


* **Type**

    Commands



#### project()
Current project in ResInsight.
Set when creating an instance and updated when opening/closing projects.


* **Type**

    Project



#### static find(startPort=50051, endPort=50071)
Search for an existing Instance of ResInsight by testing ports.

By default we search from port 50051 to 50071 or if the environment
variable RESINSIGHT_GRPC_PORT is set we search
RESINSIGHT_GRPC_PORT to RESINSIGHT_GRPC_PORT+20


* **Parameters**

    * **startPort** (*int*) – start searching from this port

    * **endPort** (*int*) – search up to but not including this port



#### static launch(resInsightExecutable='', console=False)
Launch a new Instance of ResInsight. This requires the environment variable
RESINSIGHT_EXECUTABLE to be set or the parameter resInsightExecutable to be provided.
The RESINSIGHT_GRPC_PORT environment variable can be set to an alternative port number.


* **Parameters**

    * **resInsightExecutable** (*str*) – Path to a valid ResInsight executable. If set
      will take precedence over what is provided in the RESINSIGHT_EXECUTABLE
      environment variable.

    * **console** (*bool*) – If True, launch as console application, without GUI.



* **Returns**

    an instance object if it worked. None if not.



* **Return type**

    Instance


## rips.Project module


#### class rips.Project.Project(channel)
Bases: `object`

ResInsight project. Not intended to be created separately.

Automatically created and assigned to Instance.


#### case(id)
Get a specific case from the provided case Id


* **Parameters**

    **id** (*int*) – case id



* **Returns**

    A rips Case object



#### cases()
Get a list of all cases in the project


* **Returns**

    A list of rips Case objects



#### close()
Close the current project (and open new blank project)


#### loadCase(path)
Load a new case from the given file path


* **Parameters**

    **path** (*string*) – file path to case



* **Returns**

    A rips Case object



#### open(path)
Open a new project from the given path

Argument:

    path(string): path to project file


#### selectedCases()
Get a list of all cases selected in the project tree


* **Returns**

    A list of rips Case objects


## rips.Properties module


#### class rips.Properties.Properties(case)
Bases: `object`

Class for streaming properties to and from ResInsight


#### activeCellProperty(propertyType, propertyName, timeStep, porosityModel='MATRIX_MODEL')
Get a cell property for all active cells. Async, so returns an iterator


* **Parameters**

    * **propertyType** (*string*) – string enum. See available()

    * **propertyName** (*string*) – name of an Eclipse property

    * **timeStep** (*int*) – the time step for which to get the property for

    * **porosityModel** (*string*) – string enum. See available()



* **Returns**

    An iterator to a chunk object containing an array of double values
    You first loop through the chunks and then the values within the chunk to get all values.



#### available(propertyType, porosityModel='MATRIX_MODEL')
Get a list of available properties


* **Parameters**

    * **propertyType** (*string*) – string corresponding to propertyType enum.
      Can be one of the following:

      ’DYNAMIC_NATIVE’

          ’STATIC_NATIVE’
          ‘SOURSIMRL’
          ‘GENERATED’
          ‘INPUT_PROPERTY’
          ‘FORMATION_NAMES’
          ‘FLOW_DIAGNOSTICS’
          ‘INJECTION_FLOODING’


    * **porosityModel** (*string*) – ‘MATRIX_MODEL’ or ‘FRACTURE_MODEL’.



#### gridProperty(propertyType, propertyName, timeStep, gridIndex=0, porosityModel='MATRIX_MODEL')
Get a cell property for all grid cells. Async, so returns an iterator


* **Parameters**

    * **propertyType** (*string*) – string enum. See available()

    * **propertyName** (*string*) – name of an Eclipse property

    * **timeStep** (*int*) – the time step for which to get the property for

    * **gridIndex** (*int*) – index to the grid we’re getting values for

    * **porosityModel** (*string*) – string enum. See available()



* **Returns**

    An iterator to a chunk object containing an array of double values
    You first loop through the chunks and then the values within the chunk to get all values.



#### setActiveCellProperty(values, propertyType, propertyName, timeStep, porosityModel='MATRIX_MODEL')
Set a cell property for all active cells.


* **Parameters**

    * **values** (*list*) – a list of double precision floating point numbers

    * **propertyType** (*string*) – string enum. See available()

    * **propertyName** (*string*) – name of an Eclipse property

    * **timeStep** (*int*) – the time step for which to get the property for

    * **porosityModel** (*string*) – string enum. See available()



#### setActiveCellPropertyAsync(values_iterator, propertyType, propertyName, timeStep, porosityModel='MATRIX_MODEL')
Set a cell property for all active cells. Async, and so takes an iterator to the input values


* **Parameters**

    * **values_iterator** (*iterator*) – an iterator to the properties to be set

    * **propertyType** (*string*) – string enum. See available()

    * **propertyName** (*string*) – name of an Eclipse property

    * **timeStep** (*int*) – the time step for which to get the property for

    * **porosityModel** (*string*) – string enum. See available()



#### setGridProperty(values, propertyType, propertyName, timeStep, gridIndex=0, porosityModel='MATRIX_MODEL')
Set a cell property for all grid cells.


* **Parameters**

    * **values** (*list*) – a list of double precision floating point numbers

    * **propertyType** (*string*) – string enum. See available()

    * **propertyName** (*string*) – name of an Eclipse property

    * **timeStep** (*int*) – the time step for which to get the property for

    * **gridIndex** (*int*) – index to the grid we’re setting values for

    * **porosityModel** (*string*) – string enum. See available()


## Module contents
