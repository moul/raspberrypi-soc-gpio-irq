/*
 * Raspberry PI - SoC GPIO IRQ handling
 *
 * Copyright (C) 2015 Jeanne D'hack
 *
 * Manfred Touron <m@42.am>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/reboot.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/of_gpio.h>
#include <linux/idr.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/reboot.h>
#include <linux/sysrq.h>
#include <linux/syscalls.h>
#include <linux/kthread.h>
#include <linux/gpio.h>

#define RASPBERRYPI_GPIO_SWRESET 47
#define RASPBERRYPI_GPIO_BOOTED 42

static int g_irq = -1;
static struct task_struct *task;
static int need_reboot = 0;
static atomic_t probe_count = ATOMIC_INIT(0);
static DECLARE_WAIT_QUEUE_HEAD(probe_waitqueue);
static void raspberrypi_reboot(void);

static int raspberrypi_reboot_thread(void *arg) {
  /* block the thread and wait for a wakeup.
   wakeup is done in two cases: module unload and reset gpio state change */
  wait_event_interruptible(probe_waitqueue, atomic_read(&probe_count) > 0);

  /* clean the counter */
  atomic_dec(&probe_count);

  /* reboot if woke up from gpio state change */
  if (need_reboot) {
    raspberrypi_reboot();
  }

  return 0;
}

static void raspberrypi_reboot(void) {
  /* Call /sbin/reset */
  char *argv[] = {
    "/sbin/init",
    "6",
    NULL
  };
  char *envp[] = {
    "HOME=/",
    "PWD=/",
    "PATH=/sbin",
    NULL
  };
  int ret;

  ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
  if (!ret) {
    printk("Raspberry PI SoC IRQ: Soft resetting\n");
  } else {
    printk("Raspberry PI SoC IRQ: cannot soft-reset\n");
  }
}

static irqreturn_t raspberrypi_irq_resethandler(int irq, void *dev_id) {
  /* handled when reset gpio state changed */
  if (irq == g_irq) {
    need_reboot = 1;
    atomic_inc(&probe_count);
    wake_up(&probe_waitqueue);
    irq_clear_status_flags(irq, IRQ_LEVEL);
  }
  return IRQ_HANDLED;
}

static int __init raspberrypigpio_init(void) {
  unsigned int gpio;
  int ret;

  printk(KERN_DEBUG "Raspberry PI SoC IRQ: initializing\n");

  /* create reboot thread */
  task = kthread_create(raspberrypi_reboot_thread, NULL, "raspberry-pi-soc-irq");
  if (IS_ERR(task)) {
    printk(KERN_ALERT "kthread_create error\n");
  }
  wake_up_process(task);

  /* setup an irq that watch the reset gpio state */
  gpio = RASPBERRYPI_GPIO_SWRESET;
  gpio_request(gpio, "softreset");
  gpio_direction_input(gpio);
  g_irq = gpio_to_irq(gpio);
  irq_clear_status_flags(g_irq, IRQ_LEVEL);
  ret = request_any_context_irq(g_irq, raspberrypi_irq_resethandler,
				IRQF_TRIGGER_FALLING, "raspberry-pi", NULL);

  /* enable console, switch the booted gpio */
  gpio = RASPBERRYPI_GPIO_BOOTED;
  gpio_request(gpio, "booted");
  gpio_direction_output(gpio, 0);

  printk(KERN_INFO "Raspberry PI SoC IRQ: initialized\n");
  return 0;
}

static void __exit raspberrypigpio_cleanup(void) {
  printk(KERN_DEBUG "Raspberry PI SoC IRQ: cleaning\n");

  /* terminate the thread */
  need_reboot = 0;
  atomic_inc(&probe_count);
  wake_up(&probe_waitqueue);

  /* ensure free irq */
  if (-1 == g_irq) {
    free_irq(g_irq, NULL);
  }

  /* free gpio */
  gpio_free(RASPBERRYPI_GPIO_SWRESET);
  gpio_free(RASPBERRYPI_GPIO_BOOTED);

  printk(KERN_DEBUG "Raspberry SoC IRQ cleaned\n");
  return;
}

module_init(raspberrypigpio_init);
module_exit(raspberrypigpio_cleanup);

MODULE_AUTHOR("Raspberry");
MODULE_DESCRIPTION("Raspberry - Raspberry PI SoC IRQ");
MODULE_LICENSE("GPL");
