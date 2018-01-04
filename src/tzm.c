/*
 * .c-Datei zur Aufgabe 4 BSP WS17/18
 * Hauke Goldhammer, Martin Witte
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/vmalloc.h>      // Speicher allocieren
#include <linux/moduleparam.h>  // Modulparameter
#include <linux/kernel.h>	    // printk()
#include <linux/slab.h>		    // kmalloc()
#include <linux/fs.h>		    // everything...
#include <linux/errno.h>	    // error codes
#include <linux/types.h>	    // size_t
#include <linux/jiffies.h>      // for Timer-Ticks
#include <linux/mutex.h>        // Mutex, ...
#include <linux/uaccess.h>      // copy_from_user

#include "tzm.h"

static int majorNumber = -1;
static int ret_val_time = -1;
static int ret_val_number = -1;
static u64 last_newLine = 0;
static u64 last_duration = 0;
static int counter = -1;
static int timediff_in_ms = -1;//(int64_t) ret_val_time;
static int open_devices = 0;

struct mutex my_mutex;

static struct class*  devClass  = NULL; // The device-driver class struct pointer
static struct device* deviceDriver = NULL; ///< The device-driver device struct pointer

/*
 * Modul Beschreibung
 */
MODULE_LICENSE("GPL");              ///< The license type -- this affects runtime behavior
MODULE_AUTHOR("Martin Witte, Hauke Goldhammer");      ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A simple Linux driver for stuff.");  ///< The description -- see modinfo
MODULE_VERSION("0.1");              ///< The version of the module


/*
 * module parameter
 */ 
// Default time
module_param(ret_val_time, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(ret_val_time, "Default time");
// Default number-counter
module_param(ret_val_number, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(ret_val_number, "Default number-counter");


//Forward declarations
int tzm_open(struct inode* struc_node, struct file* file);
ssize_t tzm_read(struct file* filp, char __user* buf, size_t count, loff_t* f_pos );
ssize_t tzm_write(struct file *filp, const char __user* buf, size_t bufSize, loff_t *f_pos);
int tzm_release(struct inode* struc_node, struct file* file);


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

static void __exit tzm_exit( void ){
    mutex_unlock (&my_mutex);
    mutex_destroy(&my_mutex);
    device_destroy(devClass, MKDEV(majorNumber, 0));
    class_unregister(devClass);
    class_destroy(devClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "tzm Module unloaded.\n");
}

ssize_t tzm_write(struct file *filp, const char __user* buf, size_t bufSize, loff_t *f_pos){
    PDEBUG("tzm_write -> Start");
    char* str;
    int i;
    mutex_lock (&my_mutex);
    // Speicher allocieren
    str = (char*) vmalloc(bufSize);
    if(str == 0){
        printk(KERN_ALERT "tzm_write: vmalloc -> FAIL!\n");
        return EXIT_FAILURE;
    }
    // String kopieren
    if(copy_from_user(str, buf, (long)bufSize) != 0){         // String in "Kernel" kopieren und prüfen ob alles kopiert worden ist.
        printk(KERN_ALERT "tzm_write: copy_from_user -> FAIL!\n");
        return EXIT_FAILURE;
    }
    // Zeichen zählen und Zeit errechnen
    counter = 0;
    for(i = 0 ; i < bufSize; i++){
        counter++;
        if(buf[i] == '\n'){
            last_duration = get_jiffies_64() - last_newLine;
            timediff_in_ms = (int) (((int64_t)last_duration)/HZ);
            printk(KERN_INFO "Time: %d\nSigns: %d\n", timediff_in_ms, counter); // Evtl Fehlerquelle!!!!!!! (Formatierung)
            mutex_unlock (&my_mutex);
            vfree(str);
            return counter;
        }    
    }
    mutex_unlock (&my_mutex);
    vfree(str);
    PDEBUG("tzm_write ->OK");
    return -1;
}

ssize_t tzm_read(struct file* filp, char __user* buf, size_t count, loff_t* f_pos ){
    PDEBUG("tzm_read -> Start");
    mutex_lock (&my_mutex);
    printk(KERN_INFO "Time: %d\nSigns: %d\n", timediff_in_ms, counter);
    mutex_unlock (&my_mutex);
    PDEBUG("tzm_read -> OK");
    return counter;
}    

/*
 * Zur überwachung der 'Geräteanzahl' (MAX 1 Gerät!)
 */ 
int tzm_open(struct inode* struc_node, struct file* file){
    PDEBUG("tzm_open -> Start");
    if(open_devices != 0){
        return EBUSY;
    }
    mutex_init(&my_mutex);
    open_devices++;
    PDEBUG("tzm_open -> OK");
    return EXIT_SUCCESS;
}   

int tzm_release(struct inode* struc_node, struct file* file){
    PDEBUG("tzm_release -> Start");
    mutex_unlock (&my_mutex);
    mutex_destroy(&my_mutex);
    open_devices--;
    PDEBUG("tzm_release -> OK");
    return EXIT_SUCCESS;
}

static int __init tzm_initial( void ){
    PDEBUG("tzm_initial -> Start");
    counter = ret_val_number;
    timediff_in_ms = (int) ret_val_time;
    
    // Dynamische major number vom Kernel holen.
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber<0){
        printk(KERN_ALERT "Failed to register a major number\n");
        return majorNumber;
    }
    PDEBUG("Register Major number -> OK\n"); // Evtl. Fehler: Keine Parameter übergeben.
    
    // Divice-Klasse erstellen (registrieren)
    devClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(devClass)){                // Prüfen,ob class i.O., wenn nicht -> aufräumen 
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(devClass);          // Returnwert als errno zurückgeben.
    }
    PDEBUG("class_create -> OK");
    
    // Device-treiber erstellen (registrieren)
    deviceDriver = device_create(devClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(deviceDriver)){               // Clean up if there is an error
        class_destroy(devClass);           // Repeated code but the alternative is goto statements
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device (device_create)\n");
        return PTR_ERR(deviceDriver);
    }
    PDEBUG("device-Driver created correctly\n");
    
    
    last_newLine = get_jiffies_64();
    PDEBUG("tzm_inital -> OK\n"); // Evtl. Fehler: Keine Parameter übergeben.
    printk(KERN_INFO "tzm Module loaded.\n");
    return EXIT_SUCCESS;
}


module_init(tzm_initial);
module_exit(tzm_exit);
