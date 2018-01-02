/*
 * .c-Datei zur Aufgabe 4 BSP WS17/18
 * Hauke Goldhammer, Martin Witte
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>  // Modulparameter
#include <linux/kernel.h>	    // printk()
#include <linux/slab.h>		    // kmalloc()
#include <linux/fs.h>		    // everything...
#include <linux/errno.h>	    // error codes
#include <linux/types.h>	    // size_t
#include <linux/jiffies.h>      // for Timer-Ticks
#include <pthread. h.>          // Mutex, ...

#include "tzm.h"

static int ret_val_time = -1;
static int ret_val_number = -1;
static u64 last_newLine = 0;
static u64 last_duration = 0;
static int counter = ret_val_number;
static int64_t timediff_in_ms = (int64_t) ret_val_time;
static int open_devices = 0;

pthread_mutex_t mutex;

/*
 * module parameter
 */ 
// Default time
module_param(ret_val_time, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODUE_PARAM_DESC(ret_val_time, "Default time");
// Default number-counter
module_param(ret_val_number, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODUE_PARAM_DESC(ret_val_number, "Default number-counter");

/**
 * callback Funktionen zuweisen
 */
static struct file_operations fops =
{
   .open = tzm_open,
   .read = tzm_read,
   .write = tzm_write,
   .release = tzm_release,
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
    PDEBUG("tzm_inital -> OK"); // Evtl. Fehler: Keine Parameter übergeben.
    return EXIT_SUCCESS;
}

void tzm_exit( void ){
    pthread_mutex_unlock (&mutex);
    pthread_mutex_destroy(&mutex);
    printk(KERN_INFO "tzm Module unloaded.\n");
}

ssize_t tzm_write(struct file *filp, const char __user *buf, size_t bufSize, loff_t *f_pos){
    pthread_mutex_lock (&mutex);
    counter = 0;
    for(int i = 0 ; i < bufSize; i++){
        counter++;
        if(buf[i] == '\n'){
            last_duration = get_jiffies_64() - last_newLine;
            timediff_in_ms = ((int64_t)last_duration)/HZ, counter;
            printk(KERN_INFO "Time: % " PRI64 "\nSigns: %d\n", timediff_in_ms); // Evtl Fehlerquelle!!!!!!! (Formatierung)
            pthread_mutex_unlock (&mutex);
            return counter;
        }    
    }
    pthread_mutex_unlock (&mutex);
    return -1;
}

ssize_t tzm_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos ){
    pthread_mutex_lock (&mutex);
    printk(KERN_INFO "Time: % " PRI64 "\nSigns: %d\n", timediff_in_ms); // Evtl Fehlerquelle!!!!!!! (Formatierung)
    pthread_mutex_unlock (&mutex);
    return counter;
}    

/*
 * Zur überwachung der 'Geräteanzahl' (MAX 1 Gerät!)
 */ 
int tzm_open(struct inode *, struct file *){
    if(open_devices != 0){
        return EBUSY;
    }
    if(pthread_mutex_init (&mutex, NULL) != 0 ){
        printk(KERN_ALERT "Failed mutex_init\n");
        return EXIT_FAILURE;
    }
    open_devices++;
    PDEBUG("tzm_open -> OK"); // Evtl. Fehler: Keine Parameter übergeben.
    return EXIT_SUCCESS;
}   

int tzm_release(struct inode *, struct file *){
    pthread_mutex_unlock (&mutex);
    pthread_mutex_destroy(&mutex);
    open_devices--;
    return EXIT_SUCCESS;
}

module_init(tzm_initial);
module_exit(tzm_exit);
