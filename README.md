# Instant Meshes

<img width="170" height="166" src="resources/icon.png">

This repository contains the interactive meshing software developed as part of the publication

> **Instant Field-Aligned Meshes**<br/>
> Wenzel Jakob, Marco Tarini, Daniele Panozzo, Olga Sorkine-Hornung<br/>
> In *ACM Transactions on Graphics (Proceedings of SIGGRAPH Asia 2015)*<br/>
> [PDF](http://igl.ethz.ch/projects/instant-meshes/instant-meshes-SA-2015-jakob-et-al.pdf),
> [Video](https://www.youtube.com/watch?v=U6wtw6W4x3I),
> [Project page](http://igl.ethz.ch/projects/instant-meshes/)

With modifications by Michael Shields:

- Modernized the build for C++23 with pinned Eigen, GLFW, and oneTBB
  dependencies, native Apple Silicon support, and oneTBB bundled in the macOS app.
- Added ASCII and binary STL input in the GUI and batch mode, with vertex
  deduplication and validation of malformed files and degenerate facets.
- Fixed point-cloud disk radii and bounding boxes, and validated command-line
  thread counts.
- Added core, batch conversion, and STL regression tests, plus enforced STL
  importer coverage.

## Screenshot

![Instant Meshes screenshot](resources/screenshot.jpg)

## Compiling

See the [compiling instructions](docs/compiling.md) for build requirements,
platform-specific commands, and testing.

## Usage

To get started, launch the binary and select a dataset using the "Open mesh" button on the top left (the application must be located in the same directory as the 'datasets' folder, otherwise the panel will be empty).

The standard workflow is to solve for an orientation field (first blue button) and a position field (second blue button) in sequence, after which the 'Export mesh' button becomes active. Many user interface elements display a descriptive message when hovering the mouse cursor above for a second.

A range of additional information about the input mesh, the computed fields,
and the output mesh can be visualized using the check boxes accessible via the
'Advanced' panel.

Clicking the left mouse button and dragging rotates the object; right-dragging
(or shift+left-dragging) translates, and the mouse wheel zooms. The fields can also be manipulated using brush tools that are accessible by clicking the first icon in each 'Tool' row.

On Linux, Instant Meshes relies on the program ``zenity``, which must be installed.
