#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/kernel_stat.h>
#include <linux/cpumask.h>
#include <linux/jiffies.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Barbara Gabała i Weronika Walczuk");
MODULE_DESCRIPTION("Monitor CPU Usage with Fixed 10-second Intervals");

static struct timer_list cpu_timer;

static u64 prev_total_active = 0;
static u64 prev_total_idle = 0;

static void calculate_cpu_usage(struct timer_list *timer) {
    int cpu;
    u64 total_active = 0;
    u64 total_idle = 0;

    // Aggregate CPU times across all CPUs
    for_each_possible_cpu(cpu) {
        const struct kernel_cpustat *kstat = &kcpustat_cpu(cpu);

        u64 user_time = kstat->cpustat[CPUTIME_USER];
        u64 nice_time = kstat->cpustat[CPUTIME_NICE];
        u64 system_time = kstat->cpustat[CPUTIME_SYSTEM];
        u64 idle_time = kstat->cpustat[CPUTIME_IDLE];

        total_active += user_time + nice_time + system_time;
        total_idle += idle_time;
    }

    if (prev_total_active || prev_total_idle) {
        u64 delta_active = total_active - prev_total_active;
        u64 delta_idle = total_idle - prev_total_idle;
        u64 delta_total = delta_active + delta_idle;
        u64 cpu_usage = (delta_active * 100) / delta_total;

        pr_info("CPU Usage: %llu%%\n", cpu_usage);
    }

    // Update previous values
    prev_total_active = total_active;
    prev_total_idle = total_idle;

    // Reschedule timer for the next 10-second interval
    mod_timer(&cpu_timer, jiffies + 10 * HZ);
}

static int __init monitor_init(void) {
    pr_info("CPU Usage Monitor Module Loaded with 10-second intervals\n");

    // Initialize and start the timer
    timer_setup(&cpu_timer, calculate_cpu_usage, 0);
    mod_timer(&cpu_timer, jiffies + 10 * HZ);

    return 0;
}

static void __exit monitor_exit(void) {
    pr_info("CPU Usage Monitor Module Unloaded\n");

    // Remove the timer
    del_timer(&cpu_timer);
}

module_init(monitor_init);
module_exit(monitor_exit);