---
layout: docs
title: Curve Calculator
permalink: /docs/curvecalculator/
published: true
---

The summary curve calculator is a tool to do relative simple vector calculations on a set of curves. The created curves can be stored for later use in the project.

The calculator can be run by pressing the calculator icon in en menu bar, or by right-clicking on either a summary case or the summary plot collection.

## Calculation Settings
To make a new calculated curve, click on "New Calculation". This will add a new calculation to "Calculated Summaries". Before choosing which curves to do calculations on, a calculation expression must be made. The default expression *Calculation_1 := a + b* will do a vector addition on the curves which *a* and *b* are placeholdes for, and assign it to the calculation *Calculation_1*. How to assign curves to assign to *a* and *b* will be covered in section **Summary Address Selection**. 

### Operators and Functions
Possible operations and functions are found by right-clikcing in the expression window. The following tables show all the options available.

#### Assignment Operators

| OPERATOR | DEFINITION            |
|----------|-----------------------|
|  :=      | Assign                |
|  +=      | Increment             |
|  -=      | Decrement             |
|  `*`=    | Assign multiplication |
|  %=      | Assign modulo         |
|  /=      | Assign division       |

#### Arithmetic Operators

| OPERATOR | DEFINITION      |
|----------|-----------------|
|  +       | Addition        |
|  -       | Subtraction     |
|  *       | Multiplication  |
|  /       | Division        |
|  %       | Modulus         |
|  ^       | Power           |


#### Functions

| FUNCTION | DEFINITION                                              |
|----------|---------------------------------------------------------|
| abs      | Absolute value                                          |
| avg      | Average                                                 |
| ceil     | Rounding up                                             |
| clamp    | Clamp                                                   |
| floor    | Rounding down                                           |
| frac     | Fractional portion of input                             |
| log      | Natural logarithm                                       |
| log10    | Base 10 logarithm                                       |
| max      | Maximum                                                 |
| min      | Minimum                                                 |
| mul      | Product of all the inputs.                              |
| pow      | Power                                                   |
| root     | Nth-Root                                                |
| round    | Round x to the nearest integer                          |
| roundn   | Round x to n decimal places                             |
| sgn      | Sign of x, -1 where x < 0, +1 where x > 0, else zero    |
| sqrt     | Square root                                             |
| sum      | Sum                                                     |
| trunc    | Integer portion of input                                |

#### Trigonometry Functions

| FUNCTION | DEFINITION                              |
|----------|-----------------------------------------|
| acos     | Arc cosine (in radians)                 |
| acosh    | Inverse hyperbolic cosine (in radians)  |
| asin     | Arc sine (in radians)                   |
| asinh    | Inverse hyperbolic sine (in radians)    |
| atan     | Arc tangent (in radians)                |
| atanh    | Inverse hyperbolic tangent (in radians) |
| cos      | Cosine                                  |
| cosh     | Hyperbolic cosine                       |
| cot      | Cotangent                               |
| csc      | Cosecant                                |
| deg2rad  | Convert x from degrees to radians       |
| deg2grad | Convert x from degrees to gradians      |
| rad2deg  | Convert x from radians to degrees       |
| grad2deg | Convert x from gradians to degrees      |
| sec      | Secant                                  |
| sin      | Sine                                    |
| sinc     | Sine cardinal                           |
| sinh     | Hyperbolic sine                         |
| tan      | Tangent                                 |
| tanh     | Hyperbolic tangent                      |

### Unit

## Summary Address Selection
An expression consists of placeholders (variables) for curves (summary address). By clicking "Parse Expession", the variables will appear in the table below the settings. To assign a summary address to a variable, click *Edit*. This action will create a *Summary Address Selection* dialog. Use the dialog to select a summary address and press OK. Repeat the procedure for all the variables.

## Using Generated Curves
