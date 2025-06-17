#!/bin/bash

IP=192.168.10.1/24

# Find a physical Ethernet interface (not docker, veth, etc.)
IFACE=$(ip -o link show | awk -F': ' '{print $2}' | grep -Ev '^(lo|docker|veth|br|virbr)' | while read iface; do
    if [[ -e "/sys/class/net/$iface" && $(cat "/sys/class/net/$iface/type") -eq 1 ]]; then
        echo "$iface"
        break
    fi
done)

if [[ -z "$IFACE" ]]; then
    echo "No se encontró una interfaz de red Ethernet válida!"
    exit 1
fi

echo "Asignando IP $IP a la interfaz de red $IFACE"
sudo ip addr add "$IP" dev "$IFACE" 
