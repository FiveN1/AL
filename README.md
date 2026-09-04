# AL - Tom's Application Library

This is a codebase that I use for my projects. Most of my open-source project will reference this library.
Use this library for learning and feel free to use any of this code in your own projects! Just credit me somewhere! :)

## Content:

This codebase consists of many 'sub-libraries' mostly independent on each other.

| File | About |
| --- | --- |
| [`al_alloc.h`](al_alloc.h) | Allocator implementations for easy memory management |
| [`al_input.h`](al_input.h) | Polling-based input handler & custom key types. Compatible with sokol |
| [`al_imgui.h`](al_imgui.h) | Debug ImGui windows, profiler, console |
| [`al_sdf.h`](al_sdf.h) | Library for managing SDF primitives with incremental BVH and incremental update stack. Compatible with CPU and GPU compute. |

## How to use?

Include any of the libraries that you want to use in **only one** C or C++ translation unit with the `AL_IMPL` definition.
Then simply include any of these headers in your project anywhere needed.

## Dependencies:
Some of these libraries depend hevily on [Sokol](https://github.com/floooh/sokol), cimgui, vecmath.
