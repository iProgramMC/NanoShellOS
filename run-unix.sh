qemu-system-i386                                \
-device sb16                                    \
-d cpu_reset                                    \
-m 256M                                         \
-boot d                                         \
-cdrom ./build/image.iso                        \
-debugcon stdio                                 \
-accel tcg                                      \
-hda vdisk.vdi                                  \
-monitor telnet:127.0.0.1:26362,server,nowait

