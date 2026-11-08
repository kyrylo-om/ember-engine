# platform

Wraps Raylib for the rest of the engine: window, 2D drawing, textures and fonts, audio,
keyboard and mouse input with action mapping. Public headers use Ember types only; Raylib
calls stay in the sources. Apart from the overlay of `debug`, no other module uses Raylib.
