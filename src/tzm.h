/*
 * Header-Datei zu tzm.c, BSP 4
 * 
 * Hauke Goldhammer, Martin Witte
 */

#ifndef TZM_H_

#define TZM_H_

#define DEFAULT_VALUE -1

//#undefine DEBUG_ON
#define DEBUG_ON 


#undef PDEBUG

#ifdef DEBUG_ON
#   define PDEBUG(msg, args...) printk( KERN_DEBUG "tzm: " msg, ##args)
#else
#   define PDEBUG(msg, args...) // kein debugging
#endif // DEBUG_ON


#endif // TZM_H_
