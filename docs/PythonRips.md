---
layout: docs
title: rips - gRPC Python API
permalink: /docs/python/
published: true
---

ResInsight has a [gRPC Remote Procedure Call](https://www.grpc.io/) interface with a Python Client interface. This interface allows you to interact with a running ResInsight instance from a Python script.

The Python client package is available for install via the Python PIP package system with `pip install rips` as admin user, or `pip install --user rips` as a regular user.

On some systems the `pip` command may have to be replaced by `python -m pip`.

# Instance Module


#### class rips.Instance(port=50051)
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

    * **startPort** (*int*) -- start searching from this port

    * **endPort** (*int*) -- search up to but not including this port



#### static launch(resInsightExecutable='', console=False)
Launch a new Instance of ResInsight. This requires the environment variable
RESINSIGHT_EXECUTABLE to be set or the parameter resInsightExecutable to be provided.
The RESINSIGHT_GRPC_PORT environment variable can be set to an alternative port number.


* **Parameters**

    * **resInsightExecutable** (*str*) -- Path to a valid ResInsight executable. If set
      will take precedence over what is provided in the RESINSIGHT_EXECUTABLE
      environment variable.

    * **console** (*bool*) -- If True, launch as console application, without GUI.



* **Returns**

    an instance object if it worked. None if not.



* **Return type**

    Instance


## Example

```
import rips

resInsight  = rips.Instance.find()

if resInsight is None:
    print('ERROR: could not find ResInsight')
```

# App Module


#### rips.App()
alias of `rips.App`

## Example

```
import rips

resInsight  = rips.Instance.find()
if resInsight is not None:
    print(resInsight.app.versionString())
    print("Is this a console run?", resInsight.app.isConsole())
```

# Case Module


#### class rips.Case(channel, id)
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

    **porosityModel** (*string*) -- String representing an enum.
    must be 'MATRIX_MODEL' or 'FRACTURE_MODEL'.



* **Returns**

    active_cell_count: number of active cells
    reservoir_cell_count: total number of reservoir cells



* **Return type**

    Cell Count object with the following integer attributes



#### cellInfoForActiveCells(porosityModel='MATRIX_MODEL')
Get Stream of cell info objects for current case


* **Parameters**

    **porosityModel** (*string*) -- String representing an enum.
    must be 'MATRIX_MODEL' or 'FRACTURE_MODEL'.



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

    **index** (*int*) -- The grid index


Returns: Grid object


#### gridCount()
Get number of grids in the case


#### grids()
Get a list of all rips Grid objects in the case


#### timeSteps()
Get a list containing time step strings for all time steps

```
import rips

resInsight  = rips.Instance.find()
if resInsight is not None:
    cases = resInsight.project.cases()

    print ("Got " + str(len(cases)) + " cases: ")
    for case in cases:
        print(case.name)
```

# Commands Module


#### rips.Commands()
alias of `rips.Commands`

# Grid Module


#### class rips.Grid(index, case)
Grid Information. Not meant to be constructed separately

Create Grid objects using mathods on Case: Grid() and Grids()


#### dimensions()
The dimensions in i, j, k direction


* **Returns**

    class with integer attributes i, j, k representing the extent in all three dimensions.



* **Return type**

    Vec3i


# Project Module


#### rips.Project()
alias of `rips.Project`

# Properties Module


#### class rips.Properties(case)
Class for streaming properties to and from ResInsight


#### activeCellProperty(propertyType, propertyName, timeStep, porosityModel='MATRIX_MODEL')
Get a cell property for all active cells. Async, so returns an iterator


* **Parameters**

    * **propertyType** (*string*) -- string enum. See available()

    * **propertyName** (*string*) -- name of an Eclipse property

    * **timeStep** (*int*) -- the time step for which to get the property for

    * **porosityModel** (*string*) -- string enum. See available()



* **Returns**

    An iterator to a chunk object containing an array of double values
    You first loop through the chunks and then the values within the chunk to get all values.



#### available(propertyType, porosityModel='MATRIX_MODEL')
Get a list of available properties


* **Parameters**

    * **propertyType** (*string*) -- string corresponding to propertyType enum.
      Can be one of the following:

      'DYNAMIC_NATIVE'

          'STATIC_NATIVE'
          'SOURSIMRL'
          'GENERATED'
          'INPUT_PROPERTY'
          'FORMATION_NAMES'
          'FLOW_DIAGNOSTICS'
          'INJECTION_FLOODING'


    * **porosityModel** (*string*) -- 'MATRIX_MODEL' or 'FRACTURE_MODEL'.



#### gridProperty(propertyType, propertyName, timeStep, gridIndex=0, porosityModel='MATRIX_MODEL')
Get a cell property for all grid cells. Async, so returns an iterator


* **Parameters**

    * **propertyType** (*string*) -- string enum. See available()

    * **propertyName** (*string*) -- name of an Eclipse property

    * **timeStep** (*int*) -- the time step for which to get the property for

    * **gridIndex** (*int*) -- index to the grid we're getting values for

    * **porosityModel** (*string*) -- string enum. See available()



* **Returns**

    An iterator to a chunk object containing an array of double values
    You first loop through the chunks and then the values within the chunk to get all values.



#### setActiveCellProperty(values, propertyType, propertyName, timeStep, porosityModel='MATRIX_MODEL')
Set a cell property for all active cells.


* **Parameters**

    * **values** (*list*) -- a list of double precision floating point numbers

    * **propertyType** (*string*) -- string enum. See available()

    * **propertyName** (*string*) -- name of an Eclipse property

    * **timeStep** (*int*) -- the time step for which to get the property for

    * **porosityModel** (*string*) -- string enum. See available()



#### setActiveCellPropertyAsync(values_iterator, propertyType, propertyName, timeStep, porosityModel='MATRIX_MODEL')
Set a cell property for all active cells. Async, and so takes an iterator to the input values


* **Parameters**

    * **values_iterator** (*iterator*) -- an iterator to the properties to be set

    * **propertyType** (*string*) -- string enum. See available()

    * **propertyName** (*string*) -- name of an Eclipse property

    * **timeStep** (*int*) -- the time step for which to get the property for

    * **porosityModel** (*string*) -- string enum. See available()



#### setGridProperty(values, propertyType, propertyName, timeStep, gridIndex=0, porosityModel='MATRIX_MODEL')
Set a cell property for all grid cells.


* **Parameters**

    * **values** (*list*) -- a list of double precision floating point numbers

    * **propertyType** (*string*) -- string enum. See available()

    * **propertyName** (*string*) -- name of an Eclipse property

    * **timeStep** (*int*) -- the time step for which to get the property for

    * **gridIndex** (*int*) -- index to the grid we're setting values for

    * **porosityModel** (*string*) -- string enum. See available()
