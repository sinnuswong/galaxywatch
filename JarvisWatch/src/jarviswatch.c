/*jarvis circle rotate*/
#include <tizen.h>
#include "jarviswatch.h"
#include <cairo.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <iconv.h>
#define OUTLEN 255

double interval = 0.06;
#define TEXT_BUF_SIZE 20
typedef struct appdata {
	/* Variables for basic UI contents */
	Evas_Object *win;
	Evas_Object *img;
	//Evas_Object *label;
	/* Variables for watch size information */
	int width;
	int height;
	/* Variables for cairo image backend contents */
	cairo_t *cairo;
	cairo_surface_t *surface;
	unsigned char *pixels;
	Ecore_Timer *timer;

	watch_time_h *watch_time;
	int hour24, minute, second, msecond;
	int year, month, day, day_of_week;
	unsigned char is_first_day;
	cairo_pattern_t* edge_pattern;
	cairo_pattern_t* center_pattern;
	int cx;
	int cy;

	unsigned char isDown;
	int prex, prey;
	int dx, dy;
	int downX, downY;
	double downStamp;
	double avg_speedX;
	double avg_speedY;
	Ecore_Event_Handler * handler;
	bool ambient_mode;
	double earth_center_x, earth_center_y;
	double earth_side, earth_radian, moon_radian, moon_center_x, moon_center_y;

	char *watch_text;
	cairo_text_extents_t *extents;

	double * bkcolor;
	double * line_color;
	double * center_color;
	double* time_text_color;

	double angle1;
	double angle2;
	double an;

	double alpha_time_text;

	int counter;

} appdata_s;
char * moon_color_s = "#FFFFA0";
char * sun_color_s = "#FFFFA0";
char * earth_color_s = "#4169E1";

char *month[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug",
		"Sep", "Oct", "Nov", "Dec" };
char *week[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

static double ANGLE(double angle) {
	return angle * 3.1415926 / 180.0;
}
static void update_solar_system(appdata_s *ad);
static double getNumber(char a) {
	double res = 0;
	if (a >= 'A' && a <= 'F')
		res = (a - 'A' + 10);
	else
		res = (a - '0');
	return res;
}
static double* get_color(char* in) {
	double* res = (double*) malloc(3 * sizeof(double));
	double a, b, c;
	a = getNumber(in[1]) * 16 + getNumber(in[2]);
	b = getNumber(in[3]) * 16 + getNumber(in[4]);
	c = getNumber(in[5]) * 16 + getNumber(in[6]);
	//dlog_print(DLOG_ERROR, LOG_TAG, "get color %f,%f,%f",a,b,c);
	res[0] = a / 255;
	res[1] = b / 255;
	res[2] = c / 255;
	//dlog_print(DLOG_ERROR, LOG_TAG, "get color %f,%f,%f",res[0],res[1],res[2]);
	return res;
}

static void free_data(appdata_s *ad) {
	ecore_timer_del(ad->timer);
	cairo_surface_destroy(ad->surface);
	cairo_destroy(ad->cairo);

	if (ad->edge_pattern) {
		cairo_pattern_destroy(ad->edge_pattern);
	}
	free(moon_color_s);
	free(sun_color_s);
	free(earth_color_s);
	free(ad->watch_text);
	free(ad->extents);
	watch_time_delete(*(ad->watch_time));
}
int init_solar_system(appdata_s *ad) {
	ad->watch_time = (watch_time_h*) malloc(sizeof(watch_time_h));
	//init color
	ad->bkcolor = (double*) malloc(3 * sizeof(double));
	ad->bkcolor[0] = (double) 29 / 256;
	ad->bkcolor[1] = (double) 182 / 256;
	ad->bkcolor[2] = (double) 252 / 256;

	ad->line_color = (double*) malloc(3 * sizeof(double));
	ad->line_color[0] = (double) 54 / 256;
	ad->line_color[1] = (double) 163 / 256;
	ad->line_color[2] = (double) 230 / 256;

	ad->time_text_color = (double*) malloc(3 * sizeof(double));
	ad->time_text_color[0] = ad->bkcolor[0] - 0.05;
	ad->time_text_color[1] = ad->bkcolor[1] - 0.05;
	ad->time_text_color[2] = ad->bkcolor[2] - 0.05;

	ad->center_color = (double*) malloc(3 * sizeof(double));
	ad->center_color[0] = (double) 1 / 256;
	ad->center_color[1] = (double) 81 / 256;
	ad->center_color[2] = (double) 78 / 256;

	int cx, cy, size;
	size = ad->width / 2;
	cx = size;
	cy = size;
	int r = size;
	ad->cx = cx;
	ad->cy = cy;

	/* background black */
	cairo_set_source_rgba(ad->cairo, 0, 0, 0, 1);
	cairo_arc(ad->cairo, cx, cy, r, ANGLE(0), ANGLE(360));
	cairo_fill(ad->cairo);


	//draw edge
	cairo_set_line_width(ad->cairo, 20);
	ad->edge_pattern = cairo_pattern_create_radial(cx, cy, 160, cx, cy, 180);
	cairo_pattern_add_color_stop_rgba(ad->edge_pattern, 0.36, ad->bkcolor[0],
			ad->bkcolor[1], ad->bkcolor[2], 0);
	cairo_pattern_add_color_stop_rgba(ad->edge_pattern, 0.9, ad->bkcolor[0],
			ad->bkcolor[1], ad->bkcolor[2], 0.6);
	cairo_arc(ad->cairo, cx, cy, 170, ANGLE(0), ANGLE(360));
	cairo_set_source(ad->cairo, ad->edge_pattern);
	cairo_stroke(ad->cairo);

	//wide circle
//	cairo_set_source_rgba(ad->cairo, ad->bkcolor[0], ad->bkcolor[1],
//			ad->bkcolor[2], 1);
//	cairo_set_line_width(ad->cairo, 25);
//	cairo_arc(ad->cairo, 180, 180, 100, 0, ANGLE(360));
//	cairo_stroke(ad->cairo);

	//
	cairo_set_source_rgba(ad->cairo, ad->bkcolor[0],
			ad->bkcolor[1], ad->bkcolor[2], 1);
	cairo_set_line_width(ad->cairo, 1.5);
	cairo_arc(ad->cairo, 180, 180, 137, 0, ANGLE(360));
	cairo_stroke(ad->cairo);

	//init text size and width and length
	ad->watch_text = (char*) malloc(TEXT_BUF_SIZE * sizeof(char));
	ad->extents = (cairo_text_extents_t *) malloc(sizeof(cairo_text_extents_t));
	cairo_select_font_face(ad->cairo, "Courier", CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(ad->cairo, 30);
	//   s sevas_object_move
//	snprintf(ad->watch_text, TEXT_BUF_SIZE, "%02d:%02d", 99, 99);
//	cairo_text_extents(ad->cairo, ad->watch_text, ad->extents);

//origin position.
	ad->angle1 = -15;
	ad->angle2 = -90;
	ad->an = 0;
	update_solar_system(ad);
	return 0;
}

static void update_solar_system(appdata_s *ad) {

	ad->is_first_day = 0;
	int size = ad->width;
	int r = size / 2;

	int hour24 = ad->hour24, minute = ad->minute, second = ad->second, msecond =
			ad->msecond;

	cairo_t *cr = ad->cairo;
	cairo_move_to(cr, 330, 180);
	cairo_set_line_width(cr, 6);
	cairo_set_source_rgba(ad->cairo, 0, 0, 0, 1);
	cairo_arc(ad->cairo, 180, 180, 145, ANGLE(0), ANGLE(360));
	cairo_stroke(ad->cairo);

	cairo_set_line_width(cr, 4);
	cairo_set_source_rgba(cr, (double) 54 / 256, (double) 163 / 256,
			(double) 230 / 256, 1);
	int ant = ad->angle1;
	for (int i = 0; i < 10; i++) {
		ant += 36;
		cairo_arc(cr, 180, 180, 145, ANGLE(ant), ANGLE(ant + 30));
		cairo_stroke(cr);
	}

	//cairo_set_line_width(cr, 2.5);
	cairo_set_source_rgba(ad->cairo, 0,0,0,1); //0,0.157,0.257, 0.6);
	cairo_arc(ad->cairo, 180, 180, 117, ANGLE(0), ANGLE(360));
	cairo_fill(ad->cairo);


	cairo_set_line_width(cr, 1.5);
	cairo_set_source_rgba(cr, (double) 54 / 256, (double) 163 / 256,
			(double) 230 / 256, 1);
	ant = ad->angle2;
	for (int i = 0; i < 5; i++) {
		ant += 72;
		cairo_arc(cr, 180, 180, 115, ANGLE(ant), ANGLE(ant + 36));
		cairo_stroke(cr);
	}

	//draw center

	cairo_set_source_rgba(ad->cairo, ad->bkcolor[0], ad->bkcolor[1],
			ad->bkcolor[2], 1);
	cairo_set_line_width(ad->cairo, 25);
	cairo_arc(ad->cairo, 180, 180, 95, 0, ANGLE(360));
	cairo_stroke(ad->cairo);
	ad->center_pattern = cairo_pattern_create_radial(ad->cx, ad->cy, 0, ad->cx,
			ad->cy, 78);
//	cairo_pattern_add_color_stop_rgba(ad->sun_pattern, 0, 0,0,0, 0);
	cairo_pattern_add_color_stop_rgba(ad->center_pattern, 0.1,
			ad->center_color[0], ad->center_color[1], ad->center_color[2], 0);
	cairo_pattern_add_color_stop_rgba(ad->center_pattern, 0.1,
			ad->center_color[0], ad->center_color[1], ad->center_color[2], 0.2);
	cairo_pattern_add_color_stop_rgba(ad->center_pattern, 0.7,
			ad->center_color[0], ad->center_color[1], ad->center_color[2], 0.8);
	cairo_pattern_add_color_stop_rgba(ad->center_pattern, 1,
			ad->center_color[0], ad->center_color[1], ad->center_color[2], 1);
	//cairo_pattern_add_color_stop_rgba(ad->sun_pattern, 1, (double)1/256, (double)81/256, (double)78/256, 0.8);
	//cairo_pattern_add_color_stop_rgba(ad->sun_pattern, 1, (double)1/256, (double)81/256, (double)78/256, 0.8);
	cairo_arc(ad->cairo, ad->cx, ad->cy, 83, ANGLE(0), ANGLE(360));
	cairo_set_source(ad->cairo, ad->center_pattern);
	cairo_fill(ad->cairo);
	cairo_pattern_destroy(ad->center_pattern);

	cairo_set_source_rgba(cr, ad->bkcolor[0], ad->bkcolor[1], ad->bkcolor[2],
			1);
	cairo_set_line_width(cr, 1.5);
	cairo_arc(cr, 180, 180, 65, 0, ANGLE(360));
	cairo_stroke(cr);
	//draw circle line

	cairo_set_line_width(cr, 2);
	for (int t = 0; t < 3; t++) {
		for (int i = -2; i < 2; i++) {
			int side = 65;
			int an = ad->an + 6 * i + t * 120;
			int px, py;
			px = ad->cx - side * sin(ANGLE(an));
			py = ad->cy - side * cos(ANGLE(an));
			cairo_move_to(cr, px, py);
			side = 70;
			px = ad->cx - side * sin(ANGLE(an));
			py = ad->cy - side * cos(ANGLE(an));
			cairo_line_to(cr, px, py);
			cairo_stroke(cr);
		}
	}
	ad->an += 2;
	cairo_set_font_size(ad->cairo, 33);
	cairo_set_source_rgba(cr, ad->time_text_color[0], ad->time_text_color[1],
			ad->time_text_color[2], 1);
	snprintf(ad->watch_text, TEXT_BUF_SIZE, "%02d:%02d", hour24, minute);
	cairo_text_extents(ad->cairo, ad->watch_text, ad->extents);
	cairo_move_to(cr, ad->cx - ad->extents->width / 2,
			ad->cy + ad->extents->height / 2);
	cairo_show_text(cr, ad->watch_text);

	cairo_set_font_size(ad->cairo, 19);
	cairo_set_source_rgba(cr, ad->time_text_color[0], ad->time_text_color[1],
			ad->time_text_color[2], 1);
	snprintf(ad->watch_text, TEXT_BUF_SIZE, "%s %d", month[ad->month - 1],
			ad->day);
	cairo_text_extents(ad->cairo, ad->watch_text, ad->extents);
	cairo_move_to(cr, ad->cx - ad->extents->width / 2,
			ad->cy + ad->extents->height / 2 - 30);
	cairo_show_text(cr, ad->watch_text);

	cairo_set_font_size(ad->cairo, 19);
	cairo_set_source_rgba(cr, ad->time_text_color[0], ad->time_text_color[1],
			ad->time_text_color[2], 1);
	snprintf(ad->watch_text, TEXT_BUF_SIZE, "%s", week[ad->day_of_week - 1]);
	cairo_text_extents(ad->cairo, ad->watch_text, ad->extents);
	cairo_move_to(cr, ad->cx - ad->extents->width / 2,
			ad->cy + ad->extents->height / 2 + 30);
	cairo_show_text(cr, ad->watch_text);

	/* Display this cairo watch on screen */
	cairo_surface_flush(ad->surface);
	evas_object_image_data_update_add(ad->img, 0, 0, ad->width, ad->height);
	evas_object_show(ad->img);
	ad->angle1 += 1.5;
	ad->angle2 += -1.5;
}

static Eina_Bool update_world(void *a) {
	appdata_s *ad = (appdata_s*) a;
	int ret = watch_time_get_current_time(ad->watch_time);
	if (ret != APP_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get current time. err = %d",
				ret);

	/* Get current time */
	watch_time_get_hour24(*(ad->watch_time), &(ad->hour24));
	watch_time_get_minute(*(ad->watch_time), &(ad->minute));
	watch_time_get_second(*(ad->watch_time), &(ad->second));
	watch_time_get_millisecond(*(ad->watch_time), &(ad->msecond));

//	dlog_print(DLOG_ERROR, LOG_TAG, "current time. err %02d:%02d:%02d:%02d",
//			hour24, minute, second, misec);
//	dlog_print(DLOG_ERROR, LOG_TAG, "current time. err %02d:%02d:%02d:%02d",
//			year, month, day, day_of_week);

	update_solar_system(ad);
	return ECORE_CALLBACK_RENEW;
}

static void create_base_gui(appdata_s *ad) {

	/*
	 * Window
	 * Get the watch application's elm_win.
	 * elm_win is mandatory to manipulate window
	 */
	int ret = watch_app_get_elm_win(&ad->win);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get window. err = %d", ret);
		return;
	}

	evas_object_resize(ad->win, ad->width, ad->height);

	/* Create image */
	ad->img = evas_object_image_filled_add(evas_object_evas_get(ad->win));
	evas_object_image_size_set(ad->img, ad->width, ad->height);
	evas_object_resize(ad->img, ad->width, ad->height);
	evas_object_image_content_hint_set(ad->img,
			EVAS_IMAGE_CONTENT_HINT_DYNAMIC);
	evas_object_image_colorspace_set(ad->img, EVAS_COLORSPACE_ARGB8888);
	evas_object_show(ad->img);
//	ad->handler = ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE, event_move,
//			ad);
//	ad->handler = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP, event_up,
//			ad);
//	ad->handler = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN,
//			event_down, ad);

	/* Create Cairo context */
	ad->pixels = (unsigned char*) evas_object_image_data_get(ad->img, 1);
	ad->surface = cairo_image_surface_create_for_data(ad->pixels,
			CAIRO_FORMAT_ARGB32, ad->width, ad->height, ad->width * 4);
	ad->cairo = cairo_create(ad->surface);
	init_solar_system(ad);
	/* Show window after base gui is set up */
	evas_object_show(ad->win);
}

static bool app_create(int width, int height, void *data) {
	appdata_s *ad = data;
	ad->width = width;
	ad->height = height;
	create_base_gui(ad);
	return true;
}

/*
 * @brief This callback function is called when another application
 * sends the launch request to the application
 */
static void app_control(app_control_h app_control, void *data) {
	/* Handle the launch request. */
}

static void app_pause(void *data) {
	/* Take necessary actions when application becomes invisible. */
	appdata_s *ad = data;
	if (ad->timer)
		ecore_timer_freeze(ad->timer);
}

static void app_resume(void *data) {
	/* Take necessary actions when application becomes visible. */
	appdata_s *ad = data;

	int ret = watch_time_get_current_time(ad->watch_time);
	if (ret != APP_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get current time. err = %d",
				ret);

	/* Get current time */
	watch_time_get_hour24(*(ad->watch_time), &(ad->hour24));
	watch_time_get_minute(*(ad->watch_time), &(ad->minute));
	watch_time_get_second(*(ad->watch_time), &(ad->second));
	watch_time_get_millisecond(*(ad->watch_time), &(ad->msecond));

	int year, month, day;
	watch_time_get_year(*(ad->watch_time), &year);
	watch_time_get_month(*(ad->watch_time), &month);
	watch_time_get_day(*(ad->watch_time), &day);
	watch_time_get_day_of_week(*(ad->watch_time), &(ad->day_of_week));
	if (year != ad->year || month != ad->month || day != ad->day) {
		ad->is_first_day = 1;
		ad->year = year;
		ad->month = month;
		ad->day = day;
	} else {
		ad->is_first_day = 0;
	}

	dlog_print(DLOG_ERROR, LOG_TAG,
			"failed to get current time. err = %d %d %d %d", ad->year,
			ad->month, ad->day, ad->day_of_week);
	if (ad->timer) {
		ecore_timer_thaw(ad->timer);
	} else {
		ad->timer = ecore_timer_add(interval, update_world, ad);
	}
}

/*
 * @brief This callback function is called once after the main loop of the application exits
 */
static void app_terminate(void *data) {
	/* Release all resources. */
	appdata_s *ad = data;
	/* Destroy cairo surface and context */
	free_data(ad);
}

/*
 * @brief This function will be called at each second.
 */
static void app_time_tick(watch_time_h watch_time, void *data) {
	/* Called at each second while your app is visible. Update watch UI. */

	//appdata_s *ad = data;
//	if(ad->ambient_mode == 0)
//	update_watch(ad, watch_time, 0);
//	if (!ad->timer) {
//		app_resume(data);
//	}
}

/*
 * @brief This function will be called at each minute.
 */
static void app_ambient_tick(watch_time_h watch_time, void *data) {
	/* Called at each minute while the device is in ambient mode. Update watch UI. */
	appdata_s *ad = data;
//	update_watch(ad, watch_time, 1);
	if (ad->ambient_mode) {
		update_world(ad);
	}
}

/*
 * @brief This function will be called when the ambient mode is changed
 */
static void app_ambient_changed(bool ambient_mode, void *data) {
	/* Update your watch UI to conform to the ambient mode */
	appdata_s *ad = data;
	ad->ambient_mode = ambient_mode;
	if (ambient_mode) {
		if (ad->timer) {
			ecore_timer_freeze(ad->timer);
		}
	} else {
		if (!ambient_mode) {
			if (!ad->timer) {
				app_resume(data);
			} else {
				ecore_timer_thaw(ad->timer);
			}
		}
	}
}

/*
 * @brief This function will be called when the language is changed
 */
static void watch_app_lang_changed(app_event_info_h event_info, void *user_data) {
	/*APP_EVENT_LANGUAGE_CHANGED*/
	char *locale = NULL;
	app_event_get_language(event_info, &locale);
	elm_language_set(locale);
	free(locale);
	return;
}

/*
 * @brief This function will be called when the region is changed
 */
static void watch_app_region_changed(app_event_info_h event_info,
		void *user_data) {
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

/*
 * @brief Main function of the application
 */
int main(int argc, char *argv[]) {
	appdata_s ad = { 0, };
	int ret = 0;

	watch_app_lifecycle_callback_s event_callback = { 0, };
	app_event_handler_h handlers[5] = { NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;
	event_callback.time_tick = app_time_tick;
	event_callback.ambient_tick = app_ambient_tick;
	event_callback.ambient_changed = app_ambient_changed;

	watch_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED],
			APP_EVENT_LANGUAGE_CHANGED, watch_app_lang_changed, &ad);
	watch_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED],
			APP_EVENT_REGION_FORMAT_CHANGED, watch_app_region_changed, &ad);

	ret = watch_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "watch_app_main() is failed. err = %d",
				ret);

	return ret;
}
