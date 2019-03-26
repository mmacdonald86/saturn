`Saturn` sits half way from `Mars` to `Neptune`.

## Building and installation

1. Install correct version of the `Mars` header files.

2. Produce `libsaturn.so`. 

   After this step, the `Saturn` header files and this shared library
   constitute the distribution of `Saturn`. The header files conform to `c++11` syntax.

Later we'll make this process more streamlined.

## Basic usage

1. `#include "saturn/saturn.h"`

2. Create an instance of `FeatureEngine`.

3. Create instances of specific models (which take the `FeatureEngine` object as an argument).
   These specific model objects handle special input ("tags" in the case of a "catalog model";
   additional arguments other than the common features; any other special needs) and output
   of the models.

4. With a bid request, do the following:

   1. Ingest common features to the `FeatureEngine` instance by a series of calls to `update_field`.
   2. For each model that needs to be run, call its `run` method, providing additional arguments, and collecting its results. Note that these models may change the state of the `FeatureEngine` instance by, for example, updating some fields.
   3. For Placed/Auto Optimization adgroups, call its `get_multiplier` method for the multipliers.
   4. For Cold Start and Inflight PSVR Calibration, call its 'get_cpsvr' method for the calibrated psvr.
