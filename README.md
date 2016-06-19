Multithreaded tiled pathtracer
==============================

This pathtracer was primarily made for fun and for educational purposes. In very low resolutions with very simple scenes, it runs interactively and you can move around the camera. There is no acculumation between frames - everything starts over each frame.

During initialization, the pathtracer sets up a list of tiles to be rendered. Each frames these tiles are dispatched to a number of worker threads that will process each tile in parallel. Thread synchronization is implemented via C++11's condition variables.

Demo: https://twitter.com/polyras86/status/742723920038531072


OSX instructions
----------------

For compilation I use good old `make`. Before you can call `make` you must create the file `osx_project/MakefileSettings` with something like the following:

```
BUILD_DIR = ~/Build/pathtracer
```

Use this file to specify where you want the products to be built.

Then you can build all the targets via the command `make [TARGET_NAME]`. Your current working dir must be `osx_project`.

* debug: A debug version.
* release: A fast release version.
* rd: Compile and run debug version.
* rr: Compile and run release version.


Other platforms
---------------

Since this is just a project I made for fun I have not added support for other platforms. But the code is arranged so that it should be easy to do. Have a look at `osx_main.mm`. This file only implements the platform "wrapper" and everything else is delegated to platform agnostic logic.


Sampling
--------

For now the pathtracer simply casts rays in random directions in the hemisphere above each surface normal. This is suboptimal and could be improved by using a more "intelligent" distribution to get better samples.
Besides random sampling, it also samples the sun and each "sphere light" (which are treated as point lights for simplicity).


Casting rays
------------

`rendering.cpp` defines the ray casting algorithms. It is split into three functions:

* `TraceObject()`: This is the basic tracing algorithm. It finds out what object was intersected if any.
* `TraceID()`: Uses the result of `TraceObject()` to find the ID of the hit object.
* `TraceDetails()`: Uses the result of `TraceObject()` to resolve intersection details, such as normal, albedo, etc..

This design means we can use a faster/simpler tracing algorithm when appropriate and only compute intersection normals etc. when required.

The triangle intersection code (`triangle::Intersect()`) is based on [the well-known Möller–Trumbore algorithm](https://en.wikipedia.org/wiki/Möller–Trumbore_intersection_algorithm).


More information
----------------

For more information about path tracing I recommend these links:

* http://www.pbrt.org
* http://www.scratchapixel.com/lessons/3d-basic-rendering/global-illumination-path-tracing
