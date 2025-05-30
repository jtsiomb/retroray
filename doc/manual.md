RetroRay
========

This version of the manual is for the incomplete, under-development version of
RetroRay. Hopefully as missing features are gradually filled in, any awkward
interactions documented here will change, and the this manual will be updated.

Saving and Loading scenes
-------------------------
Currently the only way to load a previously saved scene, is to pass it as a
command-line argument. When you hit the save toolbar button, scenes are always
saved as `foo.rry` in the current directory.

Navigating
----------
To navigate in 3D space, hold down either *Alt* or *Ctrl*, and drag with one of
the mouse buttons pressed:
  - Left button: rotate
  - Middle button: pan
  - Right button: zoom

Additionally the mouse-wheel can be used to zoom on some systems.

Constructing a scene
--------------------
Objects can be added by opening the object drop-down menu by clicking the "+"
toolbar button. Geometrical objects are initially placed at the origin; light
sources are placed at the current view position.

Left-click objects to select them, and either use the toolbar buttons to
move, rotate, scale them, or use the hotkeys 'g', 'r', and 's' respectively.

To constrain manipulations along a single axis hold 'x', 'y', or 'z' while
manipulating the object, and similarly to constrain across a single plane, hole
down 'shift-x', 'shift-y' or 'shift-z'. Alternatively click on the corresponding
constraint option from the 'xyz' toolbar drop-down menu.

Materials
---------
Open the material editor by pressing the corresponding "black sphere" button on
the toolbar. Hit the "+" button to create a new material, modify its properties,
and assign it to the currently selected object by pressing the "check-mark"
button.

Rendering
---------
Click on the "area" render button on the toolbar, or hit F6 to enter selective
rendering mode, then drag the rubber-band across a part of the screen to render
that area. Doing it again will add more render regions.

To do a full-screen render click the "render" button on the toolbar, or hit F5.

To display a previous render and also at the same time save it as "render.png" to the current
directory, press the "view last render" button, or hit F7.

Hitting ESC cancels any current operation, including rendering, and returns the
current tool to "select". Double-tapping ESC, quits the program.
