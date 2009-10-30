/*
 * Copyright 2009  Luc Verhaegen <libv@exsuse.de>
 * Copyright 2009  Matthias Hopf <mhopf@novell.com>
 * Copyright 2009  Egbert Eich   <eich@novell.com>
 * Copyright 2009  Jung-uk Kim   <jkim@FreeBSD.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_XF86_ANSIC_H
# include "xf86_ansic.h"
# define dirent _xf86dirent
#else
# include <unistd.h>
# include <sys/types.h>
# include <dirent.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <stdio.h>
# include <string.h>
# include <errno.h>
#endif

#include "xf86.h"

#include "rhd.h"
#include "rhd_connector.h"
#include "rhd_output.h"
#include "rhd_acpi.h"


#if defined(__FreeBSD__) || defined(__DragonFly__)

#include <stdlib.h>
#include <sys/sysctl.h>

#define	ACPI_VIDEO_LEVELS	"hw.acpi.video.lcd0.levels"
#define	ACPI_VIDEO_BRIGHTNESS	"hw.acpi.video.lcd0.brightness"

/*
 * Get/Set LCD backlight brightness via acpi_video(4).
 */
static Bool
rhdDoBacklight(struct rhdOutput *Output, Bool do_write, int *val)
{
    int *levels;
    size_t len;
    int level, max_val, num_levels;
    int i;
    RHDFUNC(Output);

    if (sysctlbyname(ACPI_VIDEO_LEVELS, NULL, &len, NULL, 0) != 0 || len == 0)
	return FALSE;
    levels = (int *)malloc(len);
    if (levels == NULL)
	return FALSE;
    if (sysctlbyname(ACPI_VIDEO_LEVELS, levels, &len, NULL, 0) != 0) {
	free(levels);
	return FALSE;
    }

    num_levels = len / sizeof(*levels);
    for (i = 0, max_val = 0; i < num_levels; i++)
	if (levels[i] > max_val)
	    max_val = levels[i];

    if (do_write) {
	int d1 = max_val * RHD_BACKLIGHT_PROPERTY_MAX + 1;
	for (i = 0, level = -1; i < num_levels; i++) {
	    int d2 = abs(*val * max_val - levels[i] * RHD_BACKLIGHT_PROPERTY_MAX);
	    if (d2 < d1) {
		level = levels[i];
		d1 = d2;
	    }
	}
	free(levels);
	if (level < 0)
	    return FALSE;
	if (sysctlbyname(ACPI_VIDEO_BRIGHTNESS, NULL, 0, &level, sizeof(level)) != 0)
	    return FALSE;
	RHDDebug(Output->scrnIndex, "%s: Wrote value %i (ACPI %i)\n", __func__, *val, level);
    } else {
	free(levels);
	len = sizeof(level);
	if (sysctlbyname(ACPI_VIDEO_BRIGHTNESS, &level, &len, NULL, 0) != 0)
	    return FALSE;
	*val = level * RHD_BACKLIGHT_PROPERTY_MAX / max_val;
	RHDDebug(Output->scrnIndex, "%s: Read value %i (ACPI %i)\n", __func__, *val, level);
    }

    return TRUE;
}

#elif defined(__linux__)

#define ACPI_PATH "/sys/class/backlight"

/*
 *
 */
static Bool
rhdDoBacklight(struct rhdOutput *Output, Bool do_write, int *val)
{
    DIR *dir = opendir(ACPI_PATH);
    struct dirent *dirent;
    char buf[10];
    RHDFUNC(Output);

    if (!dir)
	return -1;

    while ((dirent = readdir(dir)) != NULL) {
	char path[PATH_MAX];
	int fd_max;

	snprintf(path,PATH_MAX,"%s/%s/max_brightness",ACPI_PATH,dirent->d_name);
	if ((fd_max = open(path,  O_RDONLY)) > 0) {
	    int max_val;

	    while ((read(fd_max,buf,9) == -1)
		   && (errno == EINTR || errno == EAGAIN)) {};
	    close (fd_max);

	    if (sscanf(buf,"%i\n",&max_val) == 1) {
		int fd;

		snprintf(path,PATH_MAX,"%s/%s/%s",ACPI_PATH,dirent->d_name,
			 do_write ? "brightness" : "actual_brightness");
		if ((fd = open(path, do_write ? O_WRONLY : O_RDONLY)) > 0) {

		    if (do_write) {

			snprintf(buf,10,"%i\n",(*val * max_val) / RHD_BACKLIGHT_PROPERTY_MAX);
			while ((write(fd,buf,strlen(buf)) <= 0)
			       && (errno == EINTR || errno == EAGAIN)) {};

			close (fd);
			closedir (dir);
			RHDDebug(Output->scrnIndex,"%s: Wrote value %i to %s\n",
				 __func__,*val,path);

			return TRUE;
		    } else {
			memset(buf,0,10);

			while ((read(fd,buf,9) == -1)
			       && (errno == EINTR || errno == EAGAIN)) {};

			if (sscanf(buf,"%i\n",val) == 1) {
			    *val = (*val * RHD_BACKLIGHT_PROPERTY_MAX) / max_val;

			    close(fd);
			    closedir(dir);
			    RHDDebug(Output->scrnIndex,"%s: Read value %i from %s\n",
				     __func__,*val,path);

			    return TRUE;
			}
		    }
		    close (fd);
		}
	    }
	}
    }
    closedir(dir);

    return FALSE;
}
#else

/*
 * Stub
 */
static Bool
rhdDoBacklight(struct rhdOutput *Output, Bool do_write, int *val)
{
    if (do_write)
	&val = -1;
    return FALSE;
}

#endif

/*
 * RhdACPIGetBacklightControl(): return backlight value in range 0..255;
 * -1 means no ACPI BL support.
 */
int
RhdACPIGetBacklightControl(struct rhdOutput *Output)
{
    int ret;

    RHDFUNC(Output);

    rhdDoBacklight(Output, FALSE, &ret);
    return ret;
}

/*
 *
 */
void
RhdACPISetBacklightControl(struct rhdOutput *Output, int val)
{
    RHDFUNC(Output);

    rhdDoBacklight(Output, TRUE, &val);
}
