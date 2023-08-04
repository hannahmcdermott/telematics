/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdbool.h>
#include <time.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/smf.h>

#define SW0_NODE DT_ALIAS(sw0)

/* mock events */
#define MOCK_E BIT(0)

static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});

static struct gpio_callback btn_cb_data;

/* declaration of state table */
static const struct smf_state bsm[];

/* list of states in BSM */
enum bsm_state
{
	IDLE,
	WAITING,
	ONGOING,
	FINISHING,
	DONE,
};

/* reservation object */
struct booking
{
	struct smf_ctx ctx;
	time_t start_tm;
	time_t end_tm;
	uint16_t id;

	/* events */
	struct k_event received;
	// struct k_event cancelled;
	// struct k_event changed;
	// struct k_event start_pressed;
	// struct k_event stop_pressed;
	int32_t events;

} booking_t;

void mock_event(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	printk("mock event happened\n");
	/* generates mock button press event */
	k_event_post(&booking_t.received, MOCK_E);
}

/*struct
{
	uint16_t id;
	time_t start_time;
	time_t end_time;
	uint16_t duration;
	//	bool BEGAN;
	//	bool STOP;
} booking_data;*/

/* state IDLE */
static void idle_entry(void *o)
{
	printk("Waiting for booking...");
}

static void idle_run(void *o)
{
	struct booking *b = (struct booking *)o;

	/* mocks event of receiving booking object */
	if (b->events & MOCK_E)
	{
		// initializes booking data
		// b->start_tm = o.start_tm;
		// b->end_tm = = o.end_tm;
		smf_set_state(SMF_CTX(&booking_t), &bsm[WAITING]);
	}
}

/* state WAITING */
static void waiting_entry(void *o)
{
	printk("Waiting to start ride...");
}

static void waiting_run(void *o)
{
	struct booking *b = (struct booking *)o;

	/* mocks event of user pressing start ride */
	if (b->events & MOCK_E)
	{
		smf_set_state(SMF_CTX(&booking_t), &bsm[ONGOING]);
	}
}

static void waiting_exit(void *o)
{
	// struct booking *b = (struct booking *)o;
	//  starts countdown(duration)
}

/* state ONGOING */
static void ongoing_entry(void *o)
{
	printk("Ongoing state by default starts in parking and onTime state...");
}

static void ongoing_run(void *o)
{
	struct booking *b = (struct booking *)o;

	/* simulates event of user pressing finish ride */
	if (b->events & MOCK_E)
	{
		smf_set_state(SMF_CTX(&booking_t), &bsm[FINISHING]);
	}
}

/* state FINISHING */
static void finishing_entry(void *o)
{
	printk("Waiting for the location to be confirmed...");
}

static void finishing_run(void *o)
{
	struct booking *b = (struct booking *)o;

	/* simulates event of location confirmed */
	if (b->events & MOCK_E)
	{
		smf_set_state(SMF_CTX(&booking_t), &bsm[IDLE]);
	}
}

/* Populating state table */
static const struct smf_state bsm[] = {
	[IDLE] = SMF_CREATE_STATE(idle_entry, idle_run, NULL, NULL),
	[WAITING] = SMF_CREATE_STATE(waiting_entry, waiting_run, waiting_exit, NULL),
	[ONGOING] = SMF_CREATE_STATE(ongoing_entry, ongoing_run, NULL, NULL),
	[FINISHING] = SMF_CREATE_STATE(finishing_entry, finishing_run, NULL, NULL),
};

void main(void)
{
	int ret;

	if (!device_is_ready(btn.port))
	{
		printk("Error: button device %s is not ready\n", btn.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&btn, GPIO_INPUT);
	if (ret != 0)
	{
		printk("Error %d: failed to configure %s pin %d\n", ret, btn.port->name, btn.pin);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&btn, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0)
	{
		printk("Error %d: failed to configure interrupt on %s pin %d\n", ret, btn.port->name, btn.pin);
		return;
	}

	gpio_init_callback(&btn_cb_data, mock_event, BIT(btn.pin));
	gpio_add_callback(btn.port, &btn_cb_data);

	/* initialize event */
	k_event_init(&booking_t.received);

	/* set initial state */
	smf_set_initial(SMF_CTX(&booking_t), &bsm[IDLE]);

	/* run state machine */
	while (1)
	{
		/* Block until an event is detected */
		booking_t.events = k_event_wait(&booking_t.received, MOCK_E, true, K_FOREVER);
		ret = smf_run_state(SMF_CTX(&booking_t));
		if (ret)
		{
			break;
		}
	}
}
