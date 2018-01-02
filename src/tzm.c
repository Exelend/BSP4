/*
 * .c-Datei zur Aufgabe 4 BSP WS17/18
 * Hauke Goldhammer, Martin Witte
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	// printk()
#include <linux/slab.h>		// kmalloc()
#include <linux/fs.h>		// everything...
#include <linux/errno.h>	// error codes
#include <linux/types.h>	// size_t
#include <linux/jiffies.h>  // for Timer-Ticks
#include <pthread. h.>      // Mutex, ...

#include "tzm.h"

static int ret_val_time = -1;
static int ret_val_number = -1;
static u64 last_newLine = 0;
static u64 last_duration = 0;
static int counter = ret_val_number;
static int open_devices = ret_val_time;

pthread_mutex_t mutex;


/**
 * callback Funktionen zuweisen
 */
static struct file_operations fops =
{
   .open = tzm_open,
   .read = tzm_read,
   .write = tzm_write,
   //.release = tzm_release,
};

int tzm_inital( void ){
    // Dynamische major number vom Kernel holen.
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber<0){
        printk(KERN_ALERT "Failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "tzm Module loaded.\n");
    last_newLine = get_jiffies_64();
    if(pthread_mutex_init (&mutex, NULL) != 0 ){
        printk(KERN_ALERT "Failed mutex_init\n");
        return EXIT_FAILURE;
    }
    PDEBUG("tzm_inital -> OK"); // Evtl. Fehler: Keine Parameter übergeben.
    return EXIT_SUCCESS;
}

void tzm_exit( void ){
    pthread_mutex_destroy(&mutex);
    printk(KERN_INFO "tzm Module unloaded.\n");
}

ssize_t tzm_write(struct file *filp, const char __user *buf, size_t bufSize, loff_t *f_pos){
    counter = 0;
    for(int i = 0 ; i < bufSize; i++){
        counter++;
        if(buf[i] == '\n'){
            last_duration = get_jiffies_64() - last_newLine;
            printk(KERN_INFO "Time: % " PRIu64 "\nSigns: %d\n", ((uint64_t)last_duration)/HZ, counter); // Evtl Fehlerquelle!!!!!!! (Formatierung)
            return counter;
        }    
    }
    return -1;
}

ssize_t tzm_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos ){
    printk(KERN_INFO "Time: % " PRIu64 "\nSigns: %d\n", ((uint64_t)last_duration)/HZ, counter); // Evtl Fehlerquelle!!!!!!! (Formatierung)
    return counter;
}    

/*
 * Zur überwachung der 'Geräteanzahl' (MAX 1 Gerät!)
 */ 
int tzm_open(struct inode *, struct file *){
    if(open_devices != 0){
        return EBUSY;
    }
    open_devices++;
    return EXIT_SUCCESS;
}    

module_init(tzm_initial);
module_exit(tzm_exit);
