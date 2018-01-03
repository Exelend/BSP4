# Bash-skript zu BSP4 tzm
# Hauke Goldhammer, Martin Witte

#!/bin/bash

rm /dev/tzm                                    # Device-Node löschen
rmmod tzm                                      # Modul aus Kernel Entfernen
insmod tzm                                     # Modul in Kernel Laden
cat /proc/devices | grep tzm | cut -c 1-3      # Major-Device-Number
