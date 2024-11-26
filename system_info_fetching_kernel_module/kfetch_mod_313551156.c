#include <linux/fs.h> // register_chrdev
#include <linux/module.h> // module_init, module_create, try_module_get/module_put
#include <linux/init.h>
#include <linux/device.h> // struct class, device_create
#include <linux/cdev.h> // MKDEV
#include <linux/kernel.h> // kernel info
#include <linux/cpumask.h> // num_online_cpus
#include <linux/cpu.h> // cpu_data
#include <linux/uaccess.h> // copy_to_user
#include <linux/utsname.h> // nodename(hostname), release
#include <linux/mm.h> // si_meminfo
#include <linux/sched.h> // for_each_process
#include <linux/ktime.h>
#include <linux/printk.h> // pr_alert
#include <linux/version.h>
#include "kfetch.h"

#define INFO_COUNT 8 // Hostname, Split line, Kernel, CPU, CPUs, Mem, Procs, Uptime 

static int major;
static bool is_device_open = 0;
static char kfetch_buf[KFETCH_BUF_SIZE];
static int kfetch_mask = 0;
static struct class *mem_class;

static int __init initialize_module(void);
static void __exit clean_up_module(void);
static ssize_t kfetch_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t kfetch_write(struct file *, const char __user *, size_t, loff_t *);
static int kfetch_open(struct inode *, struct file *);
static int kfetch_release(struct inode *, struct file *);

const static struct file_operations kfetch_ops = {
    .owner   = THIS_MODULE,
    .read    = kfetch_read,
    .write   = kfetch_write,
    .open    = kfetch_open,
    .release = kfetch_release,
};

int initialize_module(void)
{
    major = register_chrdev(0, KFETCH_DEV_NAME, &kfetch_ops);
    if (major < 0) {
        pr_alert("Failed to register character device %d\n", major);
        return major;
    }

    pr_info("Major number %d is assigned.\n", major);

    #if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
        mem_class = class_create(KFETCH_DEV_NAME);
    #else
        mem_class = class_create(THIS_MODULE, KFETCH_DEV_NAME);
    #endif
    
    device_create(mem_class, NULL, MKDEV(major, 0), NULL, KFETCH_DEV_NAME);

    pr_info("Device created on /dev/%s\n", KFETCH_DEV_NAME);
    
    return 0;
}

void clean_up_module(void)
{
    device_destroy(mem_class, MKDEV(major, 0));
    class_destroy(mem_class);
    unregister_chrdev(major, KFETCH_DEV_NAME);
}

static ssize_t kfetch_read(struct file *filp,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset)
{
    /* fetching the information */
    // Host
    char *hostname = utsname()->nodename;

    // Kernel
    char *kernel_release = utsname()->release;

    // CPU
    unsigned int cpu = 0;
    struct cpuinfo_x86 *c = &cpu_data(cpu);
    char *cpu_name =  c->x86_model_id;

    // CPUs
    int num_of_online_cpus = num_online_cpus();
    int num_of_total_cpus = num_active_cpus();

    // Mem
    struct sysinfo si;
    si_meminfo(&si);
    unsigned long free_memory = si.freeram * PAGE_SIZE / 1048576; // 1024^2 = 1048576 (in MB)
    unsigned long total_memory = si.totalram * PAGE_SIZE / 1048576;

    // Pros
    int num_procs = 0;
    struct task_struct *task_list;
    for_each_process(task_list) {
        num_procs++;
    }

    // Uptime
    ktime_t uptime_ktime = ktime_get();
    long long uptime_min = ktime_to_ns(uptime_ktime) / 1000000000 / 60; // in min

    // Logo
    char *logo[8] = {
    "                      ",
    "        .-.           ",
    "       (.. |          ",
    "       <>  |          ",
    "      / --- \\         ",
    "     ( |   | )        ",
    "   |\\\\_)__(_//|       ",
    "  <__)------(__>      "
    };

    /* kfetch_buf processing */
    
    bool info_show_flag_list[INFO_COUNT] = {
        true,
        true,
        (kfetch_mask & KFETCH_RELEASE) != 0,
        (kfetch_mask & KFETCH_CPU_MODEL) != 0,
        (kfetch_mask & KFETCH_NUM_CPUS) != 0,
        (kfetch_mask & KFETCH_MEM) != 0,
        (kfetch_mask & KFETCH_NUM_PROCS) != 0,
        (kfetch_mask & KFETCH_UPTIME) != 0
    };

    char info_list[INFO_COUNT][64] = {0};

    // info
    const char *predefined_info[INFO_COUNT] = {
        hostname,
        "----------------------------------",
        "Kernal:   %s",
        "CPU:      %s",
        "CPUs:     %d / %d",
        "Mem:      %ld / %ld MB",
        "Procs:    %d",
        "Uptime:   %ld mins"
    };

    sprintf(info_list[0], predefined_info[0]);
    sprintf(info_list[1], predefined_info[1]);
    if (info_show_flag_list[2]) sprintf(info_list[2], predefined_info[2], kernel_release);
    if (info_show_flag_list[3]) sprintf(info_list[3], predefined_info[3], cpu_name);
    if (info_show_flag_list[4]) sprintf(info_list[4], predefined_info[4], num_of_online_cpus, num_of_total_cpus);
    if (info_show_flag_list[5]) sprintf(info_list[5], predefined_info[5], free_memory, total_memory);
    if (info_show_flag_list[6]) sprintf(info_list[6], predefined_info[6], num_procs);
    if (info_show_flag_list[7]) sprintf(info_list[7], predefined_info[7], uptime_min);

    strcpy(kfetch_buf, "");
    for (int i = 0, j = 2; i < INFO_COUNT; i++) {
        strcat(kfetch_buf, logo[i]);

        if (i < 2) {
            strcat(kfetch_buf, info_list[i]); // hostname and split line
        } else {
            while (j < INFO_COUNT) {
                if (info_show_flag_list[j]) {
                    strcat(kfetch_buf, info_list[j]);
                    j++;
                    break;
                }
                j++;
            }
        }

        strcat(kfetch_buf, "\n");
    }

    // size_t buf_length = strlen(kfetch_buf) + 1;
    if (copy_to_user(buffer, kfetch_buf, sizeof(kfetch_buf))) {
        pr_alert("Failed to copy data to user");
        return 0;
    }
    
    return sizeof(kfetch_buf);
    /* cleaning up */
}

static ssize_t kfetch_write(struct file *filp,
                            const char __user *buffer,
                            size_t length,
                            loff_t *offset)
{
    int mask_info;

    if (copy_from_user(&mask_info, buffer, length)) {
        pr_alert("Failed to copy data from user");
        return 0;
    }

    /* setting the information mask */
    kfetch_mask = mask_info;
    return length;
}

static int kfetch_open(struct inode *inode, struct file *file) {

    if (is_device_open)
        return -EBUSY;

    is_device_open = 1;
    try_module_get(THIS_MODULE); // increment the reference count of module

    return 0;
}

static int kfetch_release(struct inode *inode, struct file *file) {

    /* ready for the next caller */
    is_device_open = 0;

    /* 
     * Decrement the usage count
     */
    module_put(THIS_MODULE);

    return 0;
}

module_init(initialize_module);
module_exit(clean_up_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("313551156");
MODULE_DESCRIPTION("System Information Fetching Kernel Module");
