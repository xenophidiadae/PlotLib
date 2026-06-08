#include "plot_dll.h"

#ifdef _MSC_VER
#include <float.h>
#endif
#include <math.h>
#include <string.h>
#include <stdlib.h>

#ifdef _MSC_VER
#define plot_isfinite _finite
#else
#define plot_isfinite isfinite
#endif

static int plot_bitmap_valid(const PlotBitmap* bitmap) {
    return bitmap != NULL && bitmap->pixels != NULL && bitmap->width > 0 && bitmap->height > 0 && bitmap->stride >= bitmap->width * 4;
}

static int plot_rect_valid(PlotRect rect) {
    return rect.width > 0 && rect.height > 0;
}

static int plot_params_valid(const PlotParameterSet* params) {
    return params != NULL && params->items != NULL && params->count >= 0;
}

static int plot_round_to_int(double value) {
    return value >= 0.0 ? (int)(value + 0.5) : (int)(value - 0.5);
}

static int plot_clamp_int(int value, int min_value, int max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static double plot_clamp_double(double value, double min_value, double max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static double plot_normalize_parameter_value(const PlotParameter* parameter, double value) {
    if (parameter->max_value > parameter->min_value) {
        value = plot_clamp_double(value, parameter->min_value, parameter->max_value);
    }
    return value;
}

static int plot_rect_contains(const PlotRect* rect, int x, int y) {
    return x >= rect->x && y >= rect->y && x < rect->x + rect->width && y < rect->y + rect->height;
}

static void plot_put_pixel(PlotBitmap* bitmap, int x, int y, unsigned int color) {
    unsigned char* pixel;
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;

    if (!plot_bitmap_valid(bitmap)) {
        return;
    }

    if (x < 0 || y < 0 || x >= bitmap->width || y >= bitmap->height) {
        return;
    }

    pixel = bitmap->pixels + (size_t)y * (size_t)bitmap->stride + (size_t)x * 4u;

    b = (unsigned char)(color & 0xFFu);
    g = (unsigned char)((color >> 8) & 0xFFu);
    r = (unsigned char)((color >> 16) & 0xFFu);
    a = (unsigned char)((color >> 24) & 0xFFu);

    if (a == 255u) {
        pixel[0] = b;
        pixel[1] = g;
        pixel[2] = r;
        pixel[3] = a;
        return;
    }

    {
        unsigned int inv_a = 255u - a;
        pixel[0] = (unsigned char)((b * a + pixel[0] * inv_a) / 255u);
        pixel[1] = (unsigned char)((g * a + pixel[1] * inv_a) / 255u);
        pixel[2] = (unsigned char)((r * a + pixel[2] * inv_a) / 255u);
        pixel[3] = (unsigned char)(a + (pixel[3] * inv_a) / 255u);
    }
}

static void plot_put_pixel_clipped(PlotBitmap* bitmap, const PlotRect* clip, int x, int y, unsigned int color) {
    if (!plot_rect_contains(clip, x, y)) {
        return;
    }
    plot_put_pixel(bitmap, x, y, color);
}

static int plot_map_x(PlotRect rect, double x, double x_min, double x_max) {
    double t;
    if (x_max == x_min) {
        return rect.x;
    }
    t = (x - x_min) / (x_max - x_min);
    return rect.x + plot_round_to_int(t * (double)(rect.width - 1));
}

static int plot_map_y(PlotRect rect, double y, double y_min, double y_max) {
    double t;
    if (y_max == y_min) {
        return rect.y + rect.height - 1;
    }
    t = (y - y_min) / (y_max - y_min);
    return rect.y + rect.height - 1 - plot_round_to_int(t * (double)(rect.height - 1));
}

static void plot_draw_line_clipped(PlotBitmap* bitmap, const PlotRect* clip, int x0, int y0, int x1, int y1, unsigned int color) {
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1) {
        plot_put_pixel_clipped(bitmap, clip, x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        {
            int e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
    }
}

static void plot_draw_filled_circle(PlotBitmap* bitmap, const PlotRect* clip, int cx, int cy, int radius, unsigned int color) {
    int x;
    int y;

    if (radius <= 0) {
        plot_put_pixel_clipped(bitmap, clip, cx, cy, color);
        return;
    }

    for (y = -radius; y <= radius; ++y) {
        for (x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radius * radius) {
                plot_put_pixel_clipped(bitmap, clip, cx + x, cy + y, color);
            }
        }
    }
}

PLOT_API int PLOT_CALL plot_init_bitmap(PlotBitmap* bitmap, void* pixels, int width, int height, int stride) {
    if (bitmap == NULL || pixels == NULL || width <= 0 || height <= 0 || stride < width * 4) {
        return 0;
    }

    bitmap->pixels = (unsigned char*)pixels;
    bitmap->width = width;
    bitmap->height = height;
    bitmap->stride = stride;
    return 1;
}

PLOT_API PlotRect PLOT_CALL plot_make_rect(int x, int y, int width, int height) {
    PlotRect rect;
    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;
    return rect;
}

PLOT_API int PLOT_CALL plot_init_parameter_set(PlotParameterSet* params, PlotParameter* items, int count) {
    int i;

    if (params == NULL || items == NULL || count < 0) {
        return 0;
    }

    params->items = items;
    params->count = count;

    for (i = 0; i < count; ++i) {
        params->items[i].value = plot_normalize_parameter_value(&params->items[i], params->items[i].value);
    }

    return 1;
}

PLOT_API int PLOT_CALL plot_set_parameter_at(PlotParameterSet* params, int index, double value) {
    if (!plot_params_valid(params) || index < 0 || index >= params->count) {
        return 0;
    }

    params->items[index].value = plot_normalize_parameter_value(&params->items[index], value);
    return 1;
}

PLOT_API double PLOT_CALL plot_get_parameter_at(const PlotParameterSet* params, int index, double default_value) {
    if (!plot_params_valid(params) || index < 0 || index >= params->count) {
        return default_value;
    }

    return params->items[index].value;
}

PLOT_API int PLOT_CALL plot_find_parameter(const PlotParameterSet* params, const char* name) {
    int i;

    if (!plot_params_valid(params) || name == NULL) {
        return -1;
    }

    for (i = 0; i < params->count; ++i) {
        if (params->items[i].name != NULL && strcmp(params->items[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}

PLOT_API double PLOT_CALL plot_get_parameter(const PlotParameterSet* params, const char* name, double default_value) {
    int index = plot_find_parameter(params, name);
    if (index < 0) {
        return default_value;
    }
    return params->items[index].value;
}

PLOT_API int PLOT_CALL plot_set_parameter(PlotParameterSet* params, const char* name, double value) {
    int index = plot_find_parameter(params, name);
    if (index < 0) {
        return 0;
    }
    return plot_set_parameter_at(params, index, value);
}

PLOT_API void PLOT_CALL plot_clear(PlotBitmap* bitmap, unsigned int color) {
    int y;

    if (!plot_bitmap_valid(bitmap)) {
        return;
    }

    for (y = 0; y < bitmap->height; ++y) {
        int x;
        for (x = 0; x < bitmap->width; ++x) {
            plot_put_pixel(bitmap, x, y, color);
        }
    }
}

PLOT_API void PLOT_CALL plot_fill_rect(PlotBitmap* bitmap, PlotRect rect, unsigned int color) {
    int x0;
    int y0;
    int x1;
    int y1;
    int y;

    if (!plot_bitmap_valid(bitmap) || !plot_rect_valid(rect)) {
        return;
    }

    x0 = plot_clamp_int(rect.x, 0, bitmap->width);
    y0 = plot_clamp_int(rect.y, 0, bitmap->height);
    x1 = plot_clamp_int(rect.x + rect.width, 0, bitmap->width);
    y1 = plot_clamp_int(rect.y + rect.height, 0, bitmap->height);

    for (y = y0; y < y1; ++y) {
        int x;
        for (x = x0; x < x1; ++x) {
            plot_put_pixel(bitmap, x, y, color);
        }
    }
}

PLOT_API void PLOT_CALL plot_draw_rect(PlotBitmap* bitmap, PlotRect rect, unsigned int color) {
    if (!plot_bitmap_valid(bitmap) || !plot_rect_valid(rect)) {
        return;
    }

    plot_draw_line(bitmap, rect.x, rect.y, rect.x + rect.width - 1, rect.y, color);
    plot_draw_line(bitmap, rect.x, rect.y, rect.x, rect.y + rect.height - 1, color);
    plot_draw_line(bitmap, rect.x + rect.width - 1, rect.y, rect.x + rect.width - 1, rect.y + rect.height - 1, color);
    plot_draw_line(bitmap, rect.x, rect.y + rect.height - 1, rect.x + rect.width - 1, rect.y + rect.height - 1, color);
}

PLOT_API void PLOT_CALL plot_draw_line(PlotBitmap* bitmap, int x0, int y0, int x1, int y1, unsigned int color) {
    PlotRect full_rect;
    if (!plot_bitmap_valid(bitmap)) {
        return;
    }
    full_rect = plot_make_rect(0, 0, bitmap->width, bitmap->height);
    plot_draw_line_clipped(bitmap, &full_rect, x0, y0, x1, y1, color);
}

PLOT_API void PLOT_CALL plot_draw_grid(PlotBitmap* bitmap, PlotRect rect, int x_steps, int y_steps, unsigned int color) {
    int i;

    if (!plot_bitmap_valid(bitmap) || !plot_rect_valid(rect)) {
        return;
    }

    if (x_steps > 1) {
        for (i = 1; i < x_steps; ++i) {
            int x = rect.x + (i * rect.width) / x_steps;
            plot_draw_line_clipped(bitmap, &rect, x, rect.y, x, rect.y + rect.height - 1, color);
        }
    }

    if (y_steps > 1) {
        for (i = 1; i < y_steps; ++i) {
            int y = rect.y + (i * rect.height) / y_steps;
            plot_draw_line_clipped(bitmap, &rect, rect.x, y, rect.x + rect.width - 1, y, color);
        }
    }
}

PLOT_API void PLOT_CALL plot_draw_axes(
    PlotBitmap* bitmap,
    PlotRect rect,
    double x_min,
    double x_max,
    double y_min,
    double y_max,
    unsigned int color
) {
    if (!plot_bitmap_valid(bitmap) || !plot_rect_valid(rect)) {
        return;
    }

    if (x_min <= 0.0 && x_max >= 0.0) {
        int x = plot_map_x(rect, 0.0, x_min, x_max);
        plot_draw_line_clipped(bitmap, &rect, x, rect.y, x, rect.y + rect.height - 1, color);
    }

    if (y_min <= 0.0 && y_max >= 0.0) {
        int y = plot_map_y(rect, 0.0, y_min, y_max);
        plot_draw_line_clipped(bitmap, &rect, rect.x, y, rect.x + rect.width - 1, y, color);
    }
}

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
) {
    int i;

    if (!plot_bitmap_valid(bitmap) || !plot_rect_valid(rect) || points == NULL || count < 2) {
        return;
    }

    for (i = 1; i < count; ++i) {
        int x0 = plot_map_x(rect, points[i - 1].x, x_min, x_max);
        int y0 = plot_map_y(rect, points[i - 1].y, y_min, y_max);
        int x1 = plot_map_x(rect, points[i].x, x_min, x_max);
        int y1 = plot_map_y(rect, points[i].y, y_min, y_max);
        plot_draw_line_clipped(bitmap, &rect, x0, y0, x1, y1, color);
    }
}

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
) {
    int i;

    if (!plot_bitmap_valid(bitmap) || !plot_rect_valid(rect) || points == NULL || count <= 0) {
        return;
    }

    for (i = 0; i < count; ++i) {
        int x = plot_map_x(rect, points[i].x, x_min, x_max);
        int y = plot_map_y(rect, points[i].y, y_min, y_max);
        plot_draw_filled_circle(bitmap, &rect, x, y, radius, color);
    }
}

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
) {
    int i;
    int has_prev = 0;
    int prev_x = 0;
    int prev_y = 0;

    if (!plot_bitmap_valid(bitmap) || !plot_rect_valid(rect) || function == NULL || samples < 2) {
        return;
    }

    for (i = 0; i < samples; ++i) {
        double t = (double)i / (double)(samples - 1);
        double x_value = x_min + (x_max - x_min) * t;
        double y_value = function(x_value, user_data);

        if (!plot_isfinite(y_value)) {
            has_prev = 0;
            continue;
        }

        if (y_value < y_min || y_value > y_max) {
            has_prev = 0;
            continue;
        }

        {
            int x = plot_map_x(rect, x_value, x_min, x_max);
            int y = plot_map_y(rect, y_value, y_min, y_max);
            if (has_prev) {
                plot_draw_line_clipped(bitmap, &rect, prev_x, prev_y, x, y, color);
            }
            prev_x = x;
            prev_y = y;
            has_prev = 1;
        }
    }
}

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
) {
    int i;
    int has_prev = 0;
    int prev_x = 0;
    int prev_y = 0;

    if (!plot_bitmap_valid(bitmap) || !plot_rect_valid(rect) || function == NULL || samples < 2) {
        return;
    }

    for (i = 0; i < samples; ++i) {
        double t = (double)i / (double)(samples - 1);
        double x_value = x_min + (x_max - x_min) * t;
        double y_value = function(x_value, params, user_data);

        if (!plot_isfinite(y_value)) {
            has_prev = 0;
            continue;
        }

        if (y_value < y_min || y_value > y_max) {
            has_prev = 0;
            continue;
        }

        {
            int x = plot_map_x(rect, x_value, x_min, x_max);
            int y = plot_map_y(rect, y_value, y_min, y_max);
            if (has_prev) {
                plot_draw_line_clipped(bitmap, &rect, prev_x, prev_y, x, y, color);
            }
            prev_x = x;
            prev_y = y;
            has_prev = 1;
        }
    }
}

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
) {
    int i;
    int bar_width;
    int zero_y;

    if (!plot_bitmap_valid(bitmap) || !plot_rect_valid(rect) || values == NULL || count <= 0 || max_value == min_value) {
        return;
    }

    gap_pixels = plot_clamp_int(gap_pixels, 0, rect.width / (count > 0 ? count : 1));
    bar_width = rect.width / count;
    zero_y = plot_map_y(rect, plot_clamp_double(0.0, min_value, max_value), min_value, max_value);

    for (i = 0; i < count; ++i) {
        int left = rect.x + i * bar_width + gap_pixels / 2;
        int right = rect.x + (i + 1) * bar_width - 1 - gap_pixels / 2;
        int value_y = plot_map_y(rect, values[i], min_value, max_value);
        int top = value_y < zero_y ? value_y : zero_y;
        int bottom = value_y > zero_y ? value_y : zero_y;
        PlotRect bar_rect;

        if (right < left) {
            right = left;
        }

        bar_rect = plot_make_rect(left, top, right - left + 1, bottom - top + 1);
        plot_fill_rect(bitmap, bar_rect, fill_color);
        plot_draw_rect(bitmap, bar_rect, border_color);
    }
}

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
) {
    int i;
    int* histogram;
    double* bars;
    int max_bin_count = 0;

    if (!plot_bitmap_valid(bitmap) || !plot_rect_valid(rect) || values == NULL || count <= 0 || bins <= 0 || max_value <= min_value) {
        return;
    }

    histogram = (int*)calloc((size_t)bins, sizeof(int));
    bars = (double*)calloc((size_t)bins, sizeof(double));
    if (histogram == NULL || bars == NULL) {
        free(histogram);
        free(bars);
        return;
    }

    for (i = 0; i < count; ++i) {
        if (values[i] >= min_value && values[i] <= max_value) {
            double t = (values[i] - min_value) / (max_value - min_value);
            int index = (int)floor(t * bins);
            if (index >= bins) {
                index = bins - 1;
            }
            if (index < 0) {
                index = 0;
            }
            histogram[index] += 1;
            if (histogram[index] > max_bin_count) {
                max_bin_count = histogram[index];
            }
        }
    }

    if (max_bin_count > 0) {
        for (i = 0; i < bins; ++i) {
            bars[i] = (double)histogram[i];
        }
        plot_draw_bars(bitmap, rect, bars, bins, 0.0, (double)max_bin_count, fill_color, border_color, gap_pixels);
    }

    free(histogram);
    free(bars);
}

PLOT_API int PLOT_CALL plot_compute_stats(const double* values, int count, PlotStats* out_stats) {
    int i;
    double sum = 0.0;
    double variance_sum = 0.0;
    double min_value;
    double max_value;
    double mean;

    if (values == NULL || out_stats == NULL || count <= 0) {
        return 0;
    }

    min_value = values[0];
    max_value = values[0];

    for (i = 0; i < count; ++i) {
        if (values[i] < min_value) {
            min_value = values[i];
        }
        if (values[i] > max_value) {
            max_value = values[i];
        }
        sum += values[i];
    }

    mean = sum / (double)count;

    for (i = 0; i < count; ++i) {
        double d = values[i] - mean;
        variance_sum += d * d;
    }

    out_stats->min = min_value;
    out_stats->max = max_value;
    out_stats->mean = mean;
    out_stats->variance = variance_sum / (double)count;
    out_stats->stddev = sqrt(out_stats->variance);

    return 1;
}
