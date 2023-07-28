Hacking notes
=============

Intersection info
-----------------
Each of the full ray/csg intersection functions, fill a `struct csghit` with
information. `struct csghit` contains a `struct interval ivlist[]` array, and an
`ivcount`, for the number of ray intervals passing through the object. Convex
objects should only ever have a single entry, concave can have multiple due to
the ray exiting and re-entering the object. Each `struct interval` has two
`struct rayhit` fields named `a` and `b`, corresponding to the entry hit and
exit hit respectively.

There's also a simplified ray/object function for convenience, which calls the
full CSG intersection function and then returns the information for the first
hit of the first interval encountered by the ray, which may not be the entry of
the first interval in the list, if the ray starts inside the bounds of the
object.

Partial redrawing
-----------------
When a widget needs redrawing, `rtk_invalidate` is called, which marks it for
redraw. On the "modern PC" ports which use OpenGL for display, this also
makes sure to request a redisplay, otherwise the redraw will never be called.
Under DOS redraw is called continuously.

When the widget has been redrawn into the back buffer, `rtk_invalfb` is
called which in turn calls `app_redisplay` to add the widget rect to the union
of the dirty rectangles. On the "modern PC" ports this is a no-op as the whole
screen is always updated with `glutSwapBuffers`.
