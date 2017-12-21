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
#include <stdio.h>


int main( int argc, const char* argv[]){
	printf("Test, Test !!!");
	return 0;
}
