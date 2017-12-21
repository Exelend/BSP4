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

int tzm_test( void ){
    // Dynamische major number vom Kernel holen.
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber<0){
        printk(KERN_ALERT "Failed to register a major number\n");
        return majorNumber;
    }
	printk(KERN_INFO "Module loaded.\n");
	return EXIT_SUCCESS;
}

void tzm_exit( void ){
    printk(KERN_INFO "Module unloaded.\n");
}

void tzm_write(void){
    
}

void tzm_read(void ){
    
}    

module_init(tzm_test);
module_exit(tzm_exit);
