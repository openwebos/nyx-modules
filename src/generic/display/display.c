/* @@@LICENSE
*
*      Copyright (c) 2010-2013 LG Electronics, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

/*
********************************************************************************
* @file display.c
*
* @brief The DISPLAY module implementation.
********************************************************************************
*/

#include <fcntl.h>
#include <nyx/nyx_module.h>
#include <linux/fb.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#define MM_PER_INCH     25.4f

NYX_DECLARE_MODULE(NYX_DEVICE_DISPLAY, "Display");

nyx_error_t nyx_module_open(nyx_instance_t i, nyx_device_t **d)
{
	if (NULL == d)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	if (NULL != *d)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	nyx_display_device_t *device = (nyx_display_device_t *)calloc(sizeof(
	                                   nyx_display_device_t), 1);

	if (NULL == device)
	{
		return NYX_ERROR_OUT_OF_MEMORY;
	}

	nyx_error_t error = NYX_ERROR_NONE;

	int16_t fd;

	fd = open("/dev/fb0", O_RDWR);

	if (fd < 0)
	{
		free(device);
		return NYX_ERROR_INVALID_FILE_ACCESS;
	}

	struct fb_var_screeninfo scr_info;

	memset(&scr_info, 0, sizeof(struct fb_var_screeninfo));

	if (ioctl(fd, FBIOGET_VSCREENINFO, &scr_info) != 0)
	{
		error = NYX_ERROR_INVALID_OPERATION;
		goto out;
	}

	if (scr_info.xres == 0 || scr_info.yres == 0)
	{
		error = NYX_ERROR_VALUE_OUT_OF_RANGE;
		goto out;
	}

	if (scr_info.width == 0 || scr_info.height == 0)
	{
		error = NYX_ERROR_VALUE_OUT_OF_RANGE;
		goto out;
	}

	device->display_metrics.horizontal_pixels = scr_info.xres;
	device->display_metrics.vertical_pixels = scr_info.yres;

	/* height & width variables in fb_var_screeninfo are in mm, so converting them to inches
	 using MM_PER_INCH macro */

	device->display_metrics.horizontal_dpi = ((float)scr_info.xres *
	        MM_PER_INCH) / (float)scr_info.width;
	device->display_metrics.vertical_dpi = ((float)scr_info.yres * MM_PER_INCH) /
	                                       (float)scr_info.height;

out:

	if (fd >= 0)
	{
		close(fd);
	}

	*d = (nyx_device_t *)device;
	return error;
}

nyx_error_t nyx_module_close(nyx_device_handle_t d)
{
	free(d);
	return NYX_ERROR_NONE;
}
