// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
 * Apple SMC Keyboard backlight driver
 *
 * Copyright (C) 2026 Atharva Tiwari <atharvatiwarilinuxdev@gmail.com>
 *
 * Based on drivers/hwmon/applesmc.c
 * Copyright (C) 2007 Nicolas Boichat <nicolas@boichat.ch>
 * Copyright (C) 2010 Henrik Rydberg <rydberg@euromail.se>
 *
 */

#include <linux/leds.h>
#include <linux/mfd/macsmc.h>
#include <linux/platform_device.h>

struct macsmc_led {
	struct led_classdev cdev;
	struct apple_smc *smc;
	u8 brightness;
};

static enum led_brightness macsmc_led_get_brightness(struct led_classdev *cdev)
{
	struct macsmc_led *drv = container_of(cdev, struct macsmc_led, cdev);

	return drv->brightness;
}

static int macsmc_led_set_brightness(struct led_classdev *cdev, enum led_brightness value)
{
	struct macsmc_led *drv = container_of(cdev, struct macsmc_led, cdev);
	int ret;

	ret = apple_smc_write_u16(drv->smc, SMC_KEY(LKSB), (u16) (value & 0xFF));
	if (ret < 0)
		return ret;

	drv->brightness = value & 0xFF;

	return 0;
}

static int macsmc_led_probe(struct platform_device *pdev)
{
	struct macsmc_led *drv;

	drv = devm_kzalloc(&pdev->dev, sizeof(*drv), GFP_KERNEL);
	if (!drv)
		return -ENOMEM;

	drv->smc = dev_get_drvdata(pdev->dev.parent);

	drv->cdev.name = "smc::kbd_backlight";
	drv->cdev.max_brightness = 255;
	drv->cdev.brightness_get = macsmc_led_get_brightness;
	drv->cdev.brightness_set_blocking = macsmc_led_set_brightness;
	drv->cdev.dev = &pdev->dev;

	platform_set_drvdata(pdev, drv);

	return devm_led_classdev_register(&pdev->dev, &drv->cdev);
}

static int macsmc_led_resume(struct device *dev)
{
	struct macsmc_led *drv = dev_get_drvdata(dev);
	int ret;

	ret = apple_smc_write_u16(drv->smc, SMC_KEY(LKSB), (u16) (drv->brightness));
	if (ret < 0)
		return ret;

	return 0;
}

static DEFINE_SIMPLE_DEV_PM_OPS(macsmc_led_pm, NULL, macsmc_led_resume);

static struct platform_driver macsmc_led_driver = {
	.probe = macsmc_led_probe,
	.driver = {
		.name = "macsmc-led",
		.pm = pm_sleep_ptr(&macsmc_led_pm),
	},
};
module_platform_driver(macsmc_led_driver);

MODULE_AUTHOR("Atharva Tiwari <atharvatiwarilinuxdev@gmail.com>");
MODULE_DESCRIPTION("LED keyboard backlight driver for Intel-macs using macsmc");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_ALIAS("platform:macsmc-led");
