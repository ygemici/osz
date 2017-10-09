/*
 * core/aarch64/rpi/platform.c
 *
 * Copyright 2016 CC-by-nc-sa bztsrc@github
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * You are free to:
 *
 * - Share — copy and redistribute the material in any medium or format
 * - Adapt — remix, transform, and build upon the material
 *     The licensor cannot revoke these freedoms as long as you follow
 *     the license terms.
 *
 * Under the following terms:
 *
 * - Attribution — You must give appropriate credit, provide a link to
 *     the license, and indicate if changes were made. You may do so in
 *     any reasonable manner, but not in any way that suggests the
 *     licensor endorses you or your use.
 * - NonCommercial — You may not use the material for commercial purposes.
 * - ShareAlike — If you remix, transform, or build upon the material,
 *     you must distribute your contributions under the same license as
 *     the original.
 *
 * @brief Platform glue code
 */

/**
 * Initialize platform dependent part. Called by main()
 */
void platform_init()
{
}

/**
 * Parse platform specific parameters, called by env_init()
 */
unsigned char *platform_parse(unsigned char *env)
{
    env++;
    return env;
}

/**
 * Set up timer. Called by isr_init()
 */
void platform_timer()
{
}

/**
 * Detect devices on platform. Called by sys_init()
 */
void platform_enumerate()
{
}

/**
 * Power off the platform. Called by kprintf_poweroff()
 */
void platform_poweroff()
{
}

/**
 * Reboot computer. Called by kprintf_reboot()
 */
void platform_reset()
{
}

/**
 * Hang computer.
 */
void platform_halt()
{
}
