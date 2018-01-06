#!/bin/bash

# Bash-skript zu BSP4 tzm
# Hauke Goldhammer, Martin Witte
# Mit Parameterübergabe

rm /dev/tzm                                         # Device-Node löschen
rmmod -v tzm.ko                                     # Modul aus Kernel Entfernen
insmod tzm.ko ret_val_number=333 ret_val_time=33    # Modul in Kernel Laden mit Parameterübergabe
cat /proc/devices | grep tzm | cut -c 1-3           # Major-Device-Number
