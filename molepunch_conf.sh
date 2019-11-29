#!/bin/bash

NDEV=mp0
ip link set mtu 1000 dev $NDEV
ip link set up dev $NDEV
ip addr add 10.173.173.1/24 dev $NDEV
