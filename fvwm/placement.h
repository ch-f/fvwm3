/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PLACEMENT_H
#define PLACEMENT_H

/*  RBW - 11/02/1998  */
int SmartPlacement(FvwmWindow *t, int width, int height,int *x,int *y,
		   int pdeltax, int pdeltay);
/*  RBW - 11/02/1998  */
Bool PlaceWindow(FvwmWindow *tmp_win, style_flags *sflag, int Desk, int PageX,
		 int PageY);
void PlaceAgain_func(F_CMD_ARGS);

#endif /* PLACEMENT_H */
