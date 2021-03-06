/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <mach/board.h>
#include <mach/camera.h>
#include <mach/gpiomux.h>

int msm_cam_clk_enable(struct device *dev, struct msm_cam_clk_info *clk_info,
		struct clk **clk_ptr, int num_clk, int enable)
{
	int i;
	int rc = 0;
	if (enable) {
		for (i = 0; i < num_clk; i++) {
			clk_ptr[i] = clk_get(dev, clk_info[i].clk_name);
			if (IS_ERR(clk_ptr[i])) {
				pr_err("%s get failed\n", clk_info[i].clk_name);
				rc = PTR_ERR(clk_ptr[i]);
				goto cam_clk_get_err;
			}
			if (clk_info[i].clk_rate >= 0) {
				rc = clk_set_rate(clk_ptr[i],
							clk_info[i].clk_rate);
				if (rc < 0) {
					pr_err("%s set failed\n",
						   clk_info[i].clk_name);
					goto cam_clk_set_err;
				}
			}
			rc = clk_enable(clk_ptr[i]);
			if (rc < 0) {
				pr_err("%s enable failed\n",
					   clk_info[i].clk_name);
				goto cam_clk_set_err;
			}
		}
	} else {
		for (i = num_clk - 1; i >= 0; i--) {
			if (clk_ptr[i] != NULL)
				clk_disable(clk_ptr[i]);
				clk_put(clk_ptr[i]);
		}
	}
	return rc;


cam_clk_set_err:
	clk_put(clk_ptr[i]);
cam_clk_get_err:
	for (i--; i >= 0; i--) {
		if (clk_ptr[i] != NULL) {
			clk_disable(clk_ptr[i]);
			clk_put(clk_ptr[i]);
		}
	}
	return rc;
}

int msm_camera_config_vreg(struct device *dev, struct camera_vreg_t *cam_vreg,
		int num_vreg, struct regulator **reg_ptr, int config)
{
	int i = 0;
	int rc = 0;
	struct camera_vreg_t *curr_vreg;
	if (config) {
		for (i = 0; i < num_vreg; i++) {
			curr_vreg = &cam_vreg[i];
			reg_ptr[i] = regulator_get(dev,
				curr_vreg->reg_name);
			if (IS_ERR(reg_ptr[i])) {
				pr_err("%s: %s get failed\n",
				 __func__,
				 curr_vreg->reg_name);
				reg_ptr[i] = NULL;
				goto vreg_get_fail;
			}
			if (curr_vreg->type == REG_LDO) {
				rc = regulator_set_voltage(
				reg_ptr[i],
				curr_vreg->min_voltage,
				curr_vreg->max_voltage);
				if (rc < 0) {
					pr_err(
					"%s: %s set voltage failed\n",
					__func__,
					curr_vreg->reg_name);
					goto vreg_set_voltage_fail;
				}
				rc = regulator_set_optimum_mode(
					reg_ptr[i],
					curr_vreg->op_mode);
				if (rc < 0) {
					pr_err(
					"%s: %s set optimum mode failed\n",
					__func__,
					curr_vreg->reg_name);
					goto vreg_set_opt_mode_fail;
				}
			}
		}
	} else {
		for (i = num_vreg-1; i >= 0; i--) {
			curr_vreg = &cam_vreg[i];
			if (reg_ptr[i]) {
				if (curr_vreg->type == REG_LDO) {
					regulator_set_optimum_mode(
						reg_ptr[i], 0);
					regulator_set_voltage(
						reg_ptr[i],
						0, curr_vreg->max_voltage);
				}
				regulator_put(reg_ptr[i]);
				reg_ptr[i] = NULL;
			}
		}
	}
	return 0;

vreg_unconfig:
if (curr_vreg->type == REG_LDO)
	regulator_set_optimum_mode(reg_ptr[i], 0);

vreg_set_opt_mode_fail:
if (curr_vreg->type == REG_LDO)
	regulator_set_voltage(reg_ptr[i], 0,
		curr_vreg->max_voltage);

vreg_set_voltage_fail:
	regulator_put(reg_ptr[i]);
	reg_ptr[i] = NULL;

vreg_get_fail:
	for (i--; i >= 0; i--) {
		curr_vreg = &cam_vreg[i];
		goto vreg_unconfig;
	}
	return -ENODEV;
}

int msm_camera_enable_vreg(struct device *dev, struct camera_vreg_t *cam_vreg,
		int num_vreg, struct regulator **reg_ptr, int enable)
{
	int i = 0, rc = 0;
	if (enable) {
		for (i = 0; i < num_vreg; i++) {
			if (IS_ERR(reg_ptr[i])) {
				pr_err("%s: %s null regulator\n",
				__func__, cam_vreg[i].reg_name);
				goto disable_vreg;
			}
			rc = regulator_enable(reg_ptr[i]);
			if (rc < 0) {
				pr_err("%s: %s enable failed\n",
				__func__, cam_vreg[i].reg_name);
				goto disable_vreg;
			}
		}
	} else {
		for (i = num_vreg-1; i >= 0; i--)
			regulator_disable(reg_ptr[i]);
	}
	return rc;
disable_vreg:
	for (i--; i >= 0; i--) {
		regulator_disable(reg_ptr[i]);
		goto disable_vreg;
	}
	return rc;
}
