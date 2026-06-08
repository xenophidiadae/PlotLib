Plot DLL

This library is a small C plotting DLL for WPF applications.
It draws directly into `WriteableBitmap.BackBuffer` memory in `Bgra32` format.

What it can do:
- clear bitmap and fill rectangles
- draw lines, frames, grid, and axes
- draw polyline graphs and scatter plots
- draw functions through callbacks
- draw parameterized functions with variables like `a`
- draw bar charts and histograms
- compute simple statistics: min, max, mean, variance, stddev

Main files:
- `plot_dll.h` - public API
- `plot_dll.c` - implementation

Basic usage from WPF:
1. Create a `WriteableBitmap` with `PixelFormats.Bgra32`.
2. Call `Lock()`.
3. Pass `BackBuffer`, width, height, and stride into `plot_init_bitmap(...)`.
4. Call drawing functions from the DLL.
5. Call `AddDirtyRect(...)`.
6. Call `Unlock()`.

Parameters:
- Use `PlotParameter` and `PlotParameterSet` for variables like `a`, `b`, `c`.
- Update parameter values from WPF controls such as `Slider`.
- Redraw the bitmap on each value change for live animation.

Important notes:
- colors are passed as `0xAARRGGBB`
- the DLL itself does not create WPF controls
- UI elements like sliders stay on the C# side

Build example:
- `cl /LD plot_dll.c /Fe:plot_dll.dll`
