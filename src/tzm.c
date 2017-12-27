/*
 * .c-Datei zur Aufgabe 4 BSP WS17/18
 * Hauke Goldhammer, Martin Witte
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */

#include <linux/jiffies.h>  // for Timer-Ticks

static u64 last_newLine = 0;
static u64 last_duration = 0;
static int counter = 0;

/**
 * callback Funktionen zuweisen
 */
static struct file_operations fops =
{
   //.open = dev_open,
   .read = tzm_read,
   .write = tzm_write,
   //.release = dev_release,
};

int tzm_inital( void ){
    // Dynamische major number vom Kernel holen.
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber<0){
        printk(KERN_ALERT "Failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "tzm Module loaded.\n");
    last_newLine = get_jiffes_64();
    return EXIT_SUCCESS;
}

void tzm_exit( void ){
    printk(KERN_INFO "tzm Module unloaded.\n");
}

ssize_t tzm_write(struct file *filp, const char __user *buf, size_t bufSize, loff_t *f_pos){
    counter = 0;
    for(int i = 0 ; i < bufSize; i++){
        counter++;
        if(buf[i] == '\n'){
            last_duration = get_jiffes_64() - last_newLine;
            printk(KERN_INFO "Ticks: %d \nSigns: %d\n", last_duration, counter); // Evtl Fehlerquelle!!!!!!! (Formatierung)
            return counter;
        }    
    }
    return -1;
}

ssize_t tzm_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos ){
    printk(KERN_INFO "Ticks: %d \nSigns: %d\n", last_duration, counter); // Evtl Fehlerquelle!!!!!!! (Formatierung)
    return counter;
}    

module_init(tzm_initial);
module_exit(tzm_exit);
