#
# Linux kernel configuration
#

# kernel fundamentals
LX_ENABLE += TTY SERIAL_EARLYCON SERIAL_OF_PLATFORM PRINTK HAS_IOMEM SMP

# support disabling ticking during idle
LX_ENABLE  += NO_HZ_IDLE

# initrd support
LX_ENABLE += BINFMT_ELF BLK_DEV_INITRD

# network infrastructure
LX_ENABLE += NET NETDEVICES

# PCI support
LX_ENABLE += PCI

# Virtio devices
LX_ENABLE += VIRTIO_MENU VIRTIO_PCI VIRTIO_NET VIRTIO_CONSOLE

# network protocols
LX_ENABLE += INET IP_PNP IP_PNP_DHCP

# random number generator
LX_ENABLE += HW_RANDOM ARCH_RANDOM RANDOM_TRUST_CPU RANDOM_TRUST_BOOTLOADER
LX_ENABLE += CRYPTO_JITTERENTROPY CRYPTO_ANSI_CPRNG HW_RANDOM_TIMERIOMEM

# wireguard driver
LX_ENABLE += WIREGUARD WIREGUARD_DEBUG

# disable cpu frequency scaling
LX_DISABLE += SCHED_MC CPU_FREQ

# disable optimization not possible in PIC code under Genode
LX_DISABLE += CC_HAS_ASM_GOTO

# slim down kernel by removing superfluous drivers
LX_DISABLE += HID HID_GENERIC USB_HID VGA_CONSOLE DUMMY_CONSOLE NLS VGA_ARB DEBUG_LIST
LX_DISABLE += INPUT_KEYBOARD INPUT_MOUSE WLAN WIRELESS ETHERNET
