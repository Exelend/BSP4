/*
 * .c-Datei zur Aufgabe 4 BSP WS17/18
 * Hauke Goldhammer, Martin Witte
 *
 */

/*
 * includes
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/vmalloc.h>      // Speicher allocieren
#include <linux/moduleparam.h>  // Modulparameter
#include <linux/kernel.h>       // printk()
#include <linux/slab.h>         // kmalloc()
#include <linux/fs.h>           // everything...
#include <linux/errno.h>        // error codes
#include <linux/types.h>        // size_t
#include <linux/jiffies.h>      // for Timer-Ticks
#include <linux/mutex.h>        // Mutex, ...
#include <linux/uaccess.h>      // copy_from_user

#include "tzm.h"

/*
 * Variablen Dekleration und Initialisierung
 */
static int majorNumber = -1;
static int ret_val_time = -1;
static int ret_val_number = -1;
static int counter = -1;
static unsigned int last_newLine = 0;
static unsigned int timediff_in_ms = -1;

struct mutex RW_mutex;
struct mutex Open_R_mutex;
struct mutex Open_W_mutex;

static struct class*  devClass  = NULL; // The device-driver class struct pointer
static struct device* deviceDriver = NULL; // The device-driver device struct pointer

/*
 * Modul Beschreibung
 */
MODULE_LICENSE("GPL");                                  // The license type -- this affects runtime behavior
MODULE_AUTHOR("Martin Witte, Hauke Goldhammer");        // The author -- visible when you use modinfo
MODULE_DESCRIPTION("A simple Linux driver for stuff."); // The description -- see modinfo
MODULE_VERSION("1.0");                                  // The version of the module


/*
 * Modulparameter
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

/*
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
    mutex_unlock (&RW_mutex);
    mutex_destroy(&RW_mutex);
    mutex_unlock (&Open_R_mutex);
    mutex_destroy(&Open_R_mutex);
    mutex_unlock (&Open_W_mutex);
    mutex_destroy(&Open_W_mutex);
    device_destroy(devClass, MKDEV(majorNumber, 0));
    class_unregister(devClass);
    class_destroy(devClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "tzm Module unloaded.\n");
}

ssize_t tzm_write(struct file *filp, const char __user* buf, size_t bufSize, loff_t *f_pos){
    char* str;
    int i;
    PDEBUG("tzm_write -> Start");
    mutex_lock (&RW_mutex);
    // Speicher allocieren
    str = (char*) vmalloc(bufSize);
    if(str == 0){                                             // Prüfen, ob allocieren erfolgreich war
        printk(KERN_ALERT "tzm_write: vmalloc -> FAIL!\n");
        mutex_unlock (&RW_mutex);
        return EXIT_FAILURE;
    }
    // String kopieren
    if(copy_from_user(str, buf, (long)bufSize) != 0){         // String in "Kernel" kopieren und prüfen ob alles kopiert worden ist.
        printk(KERN_ALERT "tzm_write: copy_from_user -> FAIL!\n");
        vfree(str);
        mutex_unlock (&RW_mutex);
        return EXIT_FAILURE;
    }
    // Zeichen zählen und Zeit errechnen
    counter = 0;
    for(i = 0 ; i < bufSize; i++){
        counter++;
        if(str[i] == '\n'){
            unsigned int curTime;
            curTime = jiffies_to_msecs(get_jiffies_64());
            // Prüfen ob erste Eingabe
            if(last_newLine == ret_val_time){
                timediff_in_ms = ret_val_time;
            } else {
                timediff_in_ms = curTime - last_newLine;
            }
            printk(KERN_INFO "Time: %d\nSigns: %d\n", (int)timediff_in_ms, counter);
            last_newLine = curTime;
            vfree(str);
            mutex_unlock (&RW_mutex);
            return counter;
        }    
    }
    mutex_unlock (&RW_mutex);
    vfree(str);
    PDEBUG("tzm_write ->OK\n");
    return -1;
}

ssize_t tzm_read(struct file* filp, char __user* buf, size_t count, loff_t* f_pos ){
    char string[(int)count];
    PDEBUG("tzm_read -> Start");
    sprintf(string, "Time: %d\nSigns: %d\n\0", (int)timediff_in_ms, counter);
    mutex_lock (&RW_mutex);
    printk(KERN_INFO "%s", string);
    // Daten an 'User-Space' uebergeben
    if(copy_to_user(buf, string, count) != 0){              // wenn nicht richtig kopiert wurde:
        printk(KERN_ALERT "tzm_read: copy_to_user -> FAIL!\n");
        mutex_unlock (&RW_mutex);
        return EXIT_FAILURE;
    }    
    mutex_unlock (&RW_mutex);
    PDEBUG("tzm_read -> OK\n");
    return counter;
}    

/*
 * Zur überwachung der 'Geräteanzahl' (MAX 1 Gerät!)
 */ 
int tzm_open(struct inode* struc_node, struct file* file){
    PDEBUG("tzm_open -> Start");
    
    if(file->f_mode ==  FMODE_READ){
        if(mutex_trylock(&Open_R_mutex) == 0){
            PDEBUG("tzm_open -> Fail! Busy, 1 Device is READ running\n");
            return -EBUSY;
        }
    } else if(file->f_mode == FMODE_WRITE) {
        if(mutex_trylock(&Open_W_mutex) == 0){
            PDEBUG("tzm_open -> Fail! Busy, 1 Device is WRITE running\n");
            return -EBUSY;
        }
    } else {
        if(mutex_trylock(&Open_R_mutex) == 0){
            PDEBUG("tzm_open -> Fail! Busy, 1 Device is READ running\n");
            return -EBUSY;
        }
        if(mutex_trylock(&Open_W_mutex) == 0){
            PDEBUG("tzm_open -> Fail! Busy, 1 Device is WRITE running\n");
            return -EBUSY;
        }
    }
    
    PDEBUG("tzm_open -> OK\n");
    return EXIT_SUCCESS;
}   

int tzm_release(struct inode* struc_node, struct file* file){
    PDEBUG("tzm_release -> Start");
    mutex_unlock(&RW_mutex);
    mutex_unlock(&Open_R_mutex);
    mutex_unlock(&Open_W_mutex);
    PDEBUG("tzm_release -> OK\n");
    return EXIT_SUCCESS;
}

static int __init tzm_initial( void ){
    PDEBUG("tzm_initial -> Start");
    mutex_init(&Open_R_mutex);
    mutex_init(&Open_W_mutex);
    mutex_init(&RW_mutex);
    counter = ret_val_number;
    last_newLine = ret_val_time;
    timediff_in_ms = ret_val_time;
    
    // Dynamische major number vom Kernel holen.
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber<0){
        mutex_unlock (&RW_mutex);
        mutex_destroy(&RW_mutex);
        mutex_unlock (&Open_R_mutex);
        mutex_destroy(&Open_R_mutex);
        mutex_unlock (&Open_W_mutex);
        mutex_destroy(&Open_W_mutex);
        printk(KERN_ALERT "Failed to register a major number\n");
        return majorNumber;
    }
    PDEBUG("Register Major number -> OK\n"); // Evtl. Fehler: Keine Parameter übergeben.
    
    // Divice-Klasse erstellen (registrieren)
    devClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(devClass)){                // Prüfen,ob class i.O., wenn nicht -> aufräumen 
        mutex_unlock (&RW_mutex);
        mutex_destroy(&RW_mutex);
        mutex_unlock (&Open_R_mutex);
        mutex_destroy(&Open_R_mutex);
        mutex_unlock (&Open_W_mutex);
        mutex_destroy(&Open_W_mutex);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(devClass);
    }
    PDEBUG("class_create -> OK\n");
    
    // Device-treiber erstellen (registrieren)
    deviceDriver = device_create(devClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(deviceDriver)){               // Clean up if there is an error
        mutex_unlock (&RW_mutex);
        mutex_destroy(&RW_mutex);
        mutex_unlock (&Open_R_mutex);
        mutex_destroy(&Open_R_mutex);
        mutex_unlock (&Open_W_mutex);
        mutex_destroy(&Open_W_mutex);
        class_destroy(devClass);           // Repeated code but the alternative is goto statements
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device (device_create)\n");
        return PTR_ERR(deviceDriver);
    }
    
    PDEBUG("tzm_inital -> OK\n"); // Evtl. Fehler: Keine Parameter übergeben.
    printk(KERN_INFO "tzm Module loaded.\n");
    return EXIT_SUCCESS;
}


module_init(tzm_initial);
module_exit(tzm_exit);
