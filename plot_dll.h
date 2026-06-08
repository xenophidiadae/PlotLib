#ifndef PLOT_DLL_H
#define PLOT_DLL_H

#include <stddef.h>

#ifdef _WIN32
#define PLOT_API __declspec(dllexport)
#define PLOT_CALL __cdecl
#else
#define PLOT_API
#define PLOT_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PlotBitmap {
    unsigned char* pixels;
    int width;
    int height;
    int stride;
} PlotBitmap;

typedef struct PlotRect {
    int x;
    int y;
    int width;
    int height;
} PlotRect;

typedef struct PlotPointF {
    double x;
    double y;
} PlotPointF;

typedef struct PlotStats {
    double min;
    double max;
    double mean;
    double variance;
    double stddev;
} PlotStats;

typedef struct PlotParameter {
    const char* name;
    double value;
    double min_value;
    double max_value;
    double step;
} PlotParameter;

typedef struct PlotParameterSet {
    PlotParameter* items;
    int count;
} PlotParameterSet;

typedef double (PLOT_CALL *PlotFunction)(double x, void* user_data);
typedef double (PLOT_CALL *PlotParametricFunction)(double x, const PlotParameterSet* params, void* user_data);

PLOT_API int PLOT_CALL plot_init_bitmap(PlotBitmap* bitmap, void* pixels, int width, int height, int stride);
PLOT_API PlotRect PLOT_CALL plot_make_rect(int x, int y, int width, int height);
PLOT_API int PLOT_CALL plot_init_parameter_set(PlotParameterSet* params, PlotParameter* items, int count);
PLOT_API int PLOT_CALL plot_set_parameter_at(PlotParameterSet* params, int index, double value);
PLOT_API double PLOT_CALL plot_get_parameter_at(const PlotParameterSet* params, int index, double default_value);
PLOT_API int PLOT_CALL plot_find_parameter(const PlotParameterSet* params, const char* name);
PLOT_API double PLOT_CALL plot_get_parameter(const PlotParameterSet* params, const char* name, double default_value);
PLOT_API int PLOT_CALL plot_set_parameter(PlotParameterSet* params, const char* name, double value);

PLOT_API void PLOT_CALL plot_clear(PlotBitmap* bitmap, unsigned int color);
PLOT_API void PLOT_CALL plot_fill_rect(PlotBitmap* bitmap, PlotRect rect, unsigned int color);
PLOT_API void PLOT_CALL plot_draw_rect(PlotBitmap* bitmap, PlotRect rect, unsigned int color);
PLOT_API void PLOT_CALL plot_draw_line(PlotBitmap* bitmap, int x0, int y0, int x1, int y1, unsigned int color);
PLOT_API void PLOT_CALL plot_draw_grid(PlotBitmap* bitmap, PlotRect rect, int x_steps, int y_steps, unsigned int color);
PLOT_API void PLOT_CALL plot_draw_axes(
    PlotBitmap* bitmap,
    PlotRect rect,
    double x_min,
    double x_max,
    double y_min,
    double y_max,
    unsigned int color
);

PLOT_API void PLOT_CALL plot_draw_polyline(
    PlotBitmap* bitmap,
    PlotRect rect,
    const PlotPointF* points,
    int count,
    double x_min,
    double x_max,
    double y_min,
    double y_max,
    unsigned int color
);

PLOT_API void PLOT_CALL plot_draw_scatter(
    PlotBitmap* bitmap,
    PlotRect rect,
    const PlotPointF* points,
    int count,
    double x_min,
    double x_max,
    double y_min,
    double y_max,
    int radius,
    unsigned int color
);

PLOT_API void PLOT_CALL plot_draw_function(
    PlotBitmap* bitmap,
    PlotRect rect,
    PlotFunction function,
    void* user_data,
    double x_min,
    double x_max,
    double y_min,
    double y_max,
    int samples,
    unsigned int color
);

PLOT_API void PLOT_CALL plot_draw_parametric_function(
    PlotBitmap* bitmap,
    PlotRect rect,
    PlotParametricFunction function,
    const PlotParameterSet* params,
    void* user_data,
    double x_min,
    double x_max,
    double y_min,
    double y_max,
    int samples,
    unsigned int color
);

PLOT_API void PLOT_CALL plot_draw_bars(
    PlotBitmap* bitmap,
    PlotRect rect,
    const double* values,
    int count,
    double min_value,
    double max_value,
    unsigned int fill_color,
    unsigned int border_color,
    int gap_pixels
);

PLOT_API void PLOT_CALL plot_draw_histogram(
    PlotBitmap* bitmap,
    PlotRect rect,
    const double* values,
    int count,
    int bins,
    double min_value,
    double max_value,
    unsigned int fill_color,
    unsigned int border_color,
    int gap_pixels
);

PLOT_API int PLOT_CALL plot_compute_stats(const double* values, int count, PlotStats* out_stats);

#ifdef __cplusplus
}
#endif

#endif
