// SPDX-License-Identifier: GPL-2.0-only
/*
 * FlyPOS Pro serial touchscreen driver
 *
 * Copyright (c) 2025 Asterleen // Marko Deinterlace
 * Based on Hampshire serial driver by Adam Bennett
 */


#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/serio.h>

#define DRIVER_DESC "FlyPOS Pro serial touchscreen driver"


// actually it would be nice to add this to linux/serio.h
// but Linus likely won't accept this driver into the main kernel
// anybody to try tho?
#define SERIO_FLYTOUCH 0xe4

MODULE_AUTHOR("Asterleen <syn@asterleen.com>");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

/*
 * Definitions & global arrays.
 */

#define FLYTOUCH_TOUCH_BEGIN   0xc8
#define FLYTOUCH_TOUCH_END     0x88
#define FLYTOUCH_BUFFER_LENGTH 4

#define FLYTOUCH_MIN_XC 0
#define FLYTOUCH_MAX_XC 0x8000
#define FLYTOUCH_MIN_YC 0
#define FLYTOUCH_MAX_YC 0x8000


/*
 * Per-touchscreen data.
 */

enum {
    FLYTOUCH_STATE_NONE,
    FLYTOUCH_STATE_PRESSED,
    FLYTOUCH_STATE_RELEASED
};

struct flytouch {
    struct input_dev *dev;
    struct serio *serio;
    unsigned char state, idx;
    unsigned char buffer[FLYTOUCH_BUFFER_LENGTH];
    char phys[32];
};


static void flytouch_process_data(struct flytouch *pflytouch, unsigned char data)
{
    struct input_dev *dev = pflytouch->dev;

    if (pflytouch->idx == 0) {
        switch (data) {
            case 0xc8:
                pflytouch->state = FLYTOUCH_STATE_PRESSED;
                break;

            case 0x88:
                pflytouch->state = FLYTOUCH_STATE_RELEASED;
                break;

            default:
                dev_dbg(&pflytouch->serio->dev, "Out of order state byte received: %#x\n", data);
        }
    } else {
        if (pflytouch->state == FLYTOUCH_STATE_PRESSED || pflytouch->state == FLYTOUCH_STATE_RELEASED) {
            pflytouch->buffer[pflytouch->idx - 1] = data;

            if (pflytouch->idx == FLYTOUCH_BUFFER_LENGTH) {
                input_report_abs(dev, ABS_X, (pflytouch->buffer[0] << 8) | pflytouch->buffer[1]);
                input_report_abs(dev, ABS_Y, (pflytouch->buffer[2] << 8) | pflytouch->buffer[3]);
                input_report_key(dev, BTN_TOUCH, pflytouch->state == FLYTOUCH_STATE_PRESSED);
                input_sync(dev);

                pflytouch->state = FLYTOUCH_STATE_NONE;
            }
        }
    }

    if (pflytouch->idx == FLYTOUCH_BUFFER_LENGTH) {
        pflytouch->idx = 0;
    } else {
        pflytouch->idx++;
    }
}

static irqreturn_t flytouch_interrupt(struct serio *serio,
                unsigned char data, unsigned int flags)
{
    struct flytouch *pflytouch = serio_get_drvdata(serio);

    flytouch_process_data(pflytouch, data);

    return IRQ_HANDLED;
}

static void flytouch_disconnect(struct serio *serio)
{
    struct flytouch *pflytouch = serio_get_drvdata(serio);

    input_get_device(pflytouch->dev);
    input_unregister_device(pflytouch->dev);
    serio_close(serio);
    serio_set_drvdata(serio, NULL);
    input_put_device(pflytouch->dev);
    kfree(pflytouch);
}

/*
 * flytouch_connect() is the routine that is called when someone adds a
 * new serio device that supports FlyPOS protocol and registers it as
 * an input device. This is usually accomplished using inputattach.
 */

static int flytouch_connect(struct serio *serio, struct serio_driver *drv)
{
    struct flytouch *pflytouch;
    struct input_dev *input_dev;
    int err;

    pflytouch = kzalloc(sizeof(*pflytouch), GFP_KERNEL);
    input_dev = input_allocate_device();
    if (!pflytouch || !input_dev) {
        err = -ENOMEM;
        goto fail1;
    }

    pflytouch->serio = serio;
    pflytouch->dev = input_dev;
    snprintf(pflytouch->phys, sizeof(pflytouch->phys),
                "%s/input0", serio->phys);

    input_dev->name = "FlyPOS Pro Serial TouchScreen";
    input_dev->phys = pflytouch->phys;
    input_dev->id.bustype = BUS_RS232;
    input_dev->id.vendor = SERIO_UNKNOWN; // Linux knows nothing about FlyTech
    input_dev->id.product = 0;
    input_dev->id.version = 0x0001;
    input_dev->dev.parent = &serio->dev;
    input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
    input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
    input_set_abs_params(pflytouch->dev, ABS_X,
                            FLYTOUCH_MIN_XC, FLYTOUCH_MAX_XC, 0, 0);
    input_set_abs_params(pflytouch->dev, ABS_Y,
                            FLYTOUCH_MIN_YC, FLYTOUCH_MAX_YC, 0, 0);

    serio_set_drvdata(serio, pflytouch);

    err = serio_open(serio, drv);
    if (err)
        goto fail2;

    err = input_register_device(pflytouch->dev);
    if (err)
        goto fail3;

    return 0;

 fail3: serio_close(serio);
 fail2: serio_set_drvdata(serio, NULL);
 fail1: input_free_device(input_dev);
        kfree(pflytouch);
        return err;
}

/*
 * The serio driver structure.
 */

static const struct serio_device_id flytouch_serio_ids[] = {
    {
        .type   = SERIO_RS232,
        .proto  = SERIO_FLYTOUCH,
        .id     = SERIO_ANY,
        .extra  = SERIO_ANY,
    },
    { 0 }
};

MODULE_DEVICE_TABLE(serio, flytouch_serio_ids);

static struct serio_driver flytouch_drv = {
    .driver         = {
            .name   = "flytouch",
    },
    .description    = DRIVER_DESC,
    .id_table       = flytouch_serio_ids,
    .interrupt      = flytouch_interrupt,
    .connect        = flytouch_connect,
    .disconnect     = flytouch_disconnect,
};

module_serio_driver(flytouch_drv);
