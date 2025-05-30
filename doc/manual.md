RetroRay
========

This version of the manual is for the incomplete, under-development version of
RetroRay. Hopefully as missing features are gradually filled in, any awkward
interactions documented here will change, and the this manual will be updated.

Saving and Loading scenes
-------------------------
File dialogs are not implemented yet. Saving and loading uses a fixed filename,
which by default is `foo.rry` (in the current directory). Pressing the open or
save toolbar buttons always operate on that fixed filename.

You can open a different file if you pass a filename as a command-line argument
to retroray. In that case, the specified file will be opened, and the "default"
filename becomes whatever the command-line argument specified. So further
invocations of save and open will use that filename instead.

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

To display a previous render and also at the same time save it as "render.png"
to the current directory, press the "view last render" button, or hit F7.

Hitting ESC cancels any current operation, including rendering, and returns the
current tool to "select". Double-tapping ESC, quits the program.

File format
-----------
The retroray file format is a simple text-based hierarchical scene description.
Nodes are of the form: `nodetype { <attributes> ... <child nodes> }`. Attributes
can be strings (in double quotes), numbers, or vectors in square brackets. The
root node is always `rrscene`, and it can contain any number of the following
elements: `material`, `object`, `light`, `view`.

### Material node

Mandatory attributes: 
  - `name` (string): must be unique and it's used to refer to a material

Optional attributes:
  - `kd` (vec3): diffuse color
  - `ks` (vec3): specular color
  - `ke` (vec3): emissive color
  - `shin` (num): shininess / specular exponent, range [1, 128]
  - `reflect` (num): reflectivity, range [0, 1]
  - `transmit` (num): transmissivity, range [0, 1]
  - `ior` (num): index of refraction, range [1, 2]

Child nodes: none

### Object node

Mandatory attributes: none

Optional attributes:
  - `name` (string): must be unique
  - `type` (string): must be one of: `sphere`, `box`
  - `material` (string): must be the name of a material node
  - `pos` (vec3): world position
  - `rot` (vec4): orientation quaternion (x, y, z, w)
  - `scale` (vec3): scaling
  - `pivot` (vec3): center of rotation/scaling in local coordinates

Child nodes: none

### Light node

Mandatory attributes: none

Optional attributes:
  - `name` (string): must be unique
  - `pos` (vec3): light position in world space
  - `color` (vec3): light color
  - `energy` (num): color multiplier
  - `shadows` (int): cast shadows option, boolean 0 or 1

Child nodes: none

### View node

Only one is expected, the rest will be ignored.

Mandatory attributes: none

Optional attributes:
  - `theta` (num): horizontal spherical coordinates angle in degrees [0, 360)
  - `phi` (num): vertical spherical coordinates angle in degrees [0, 180]
  - `dist` (num): zoom, or distance from the target position
  - `pos` (vec3) target position

Child nodes: none
