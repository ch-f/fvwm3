/* -*-c-*- */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#ifndef FRAME_H
#define FRAME_H

/* ---------------------------- included header files ----------------------- */

/* ---------------------------- global definitions -------------------------- */

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

typedef enum
{
	FRAME_MR_SETUP,
	FRAME_MR_OPAQUE,
	FRAME_MR_SHRINK,
	FRAME_MR_SCROLL
} frame_move_resize_mode;

typedef struct
{
	int decor_grav;
	int title_grav;
	int lbutton_grav;
	int rbutton_grav;
	int parent_grav;
	int client_grav;
} frame_decor_gravities_type;

/* details are hidden in frame.c */
typedef void *frame_move_resize_args;

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

void frame_init(void);
void frame_destroyed_frame(
	Window frame_w);
frame_move_resize_args frame_create_move_resize_args(
	FvwmWindow *fw, frame_move_resize_mode mr_mode,
	rectangle *start_g, rectangle *end_g, int anim_steps);
void frame_update_move_resize_args(
	frame_move_resize_args mr_args, rectangle *end_g);
void frame_free_move_resize_args(
	frame_move_resize_args mr_args);
void frame_get_sidebar_geometry(
	FvwmWindow *fw, DecorFaceStyle *borderstyle, rectangle *frame_g,
	rectangle *ret_g, Bool *ret_has_x_marks, Bool *ret_has_y_marks);
window_parts frame_get_changed_border_parts(
	rectangle *old_sidebar_g, rectangle *new_sidebar_g);
int frame_window_id_to_context(
	FvwmWindow *fw, Window w, int *ret_num);
void frame_move_resize(
	FvwmWindow *fw, frame_move_resize_args mr_args);
void frame_get_resize_decor_gravities(
	frame_decor_gravities_type *ret_grav, direction_type title_dir,
	frame_move_resize_mode rmode);
void frame_set_decor_gravities(
	FvwmWindow *fw, frame_decor_gravities_type *grav);
void frame_setup_window(
	FvwmWindow *fw, int x, int y, int w, int h,
	Bool do_send_configure_notify);
void frame_force_setup_window(
	FvwmWindow *fw, int x, int y, int w, int h,
	Bool do_send_configure_notify);
void frame_setup_shape(
	FvwmWindow *fw, int w, int h);

#endif /* FRAME_H */
