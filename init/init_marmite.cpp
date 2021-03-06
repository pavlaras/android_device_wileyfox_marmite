/*
   Copyright (c) 2016, The CyanogenMod Project
   Copyright (c) 2017, The LineageOS Project

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.
   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdlib>
#include <string>

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/sysinfo.h>

#include "vendor_init.h"
#include "property_service.h"
#include "log.h"
#include "util.h"

char const *heapstartsize;
char const *heapgrowthlimit;
char const *heapsize;
char const *heapminfree;

static int read_file2(const char *fname, char *data, int max_size)
{
    int fd, rc;

    if (max_size < 1)
        return 0;

    fd = open(fname, O_RDONLY);
    if (fd < 0) {
        ERROR("failed to open '%s'\n", fname);
        return 0;
    }

    rc = read(fd, data, max_size - 1);
    if ((rc > 0) && (rc < max_size))
        data[rc] = '\0';
    else
        data[0] = '\0';
    close(fd);

    return 1;
}

static void init_alarm_boot_properties()
{
    char const *alarm_file = "/proc/sys/kernel/boot_reason";
    char buf[64];

    if (read_file2(alarm_file, buf, sizeof(buf))) {
        /*
         * Setup ro.alarm_boot value to true when it is RTC triggered boot up
         * For existing PMIC chips, the following mapping applies
         * for the value of boot_reason:
         *
         * 0 -> unknown
         * 1 -> hard reset
         * 2 -> sudden momentary power loss (SMPL)
         * 3 -> real time clock (RTC)
         * 4 -> DC charger inserted
         * 5 -> USB charger insertd
         * 6 -> PON1 pin toggled (for secondary PMICs)
         * 7 -> CBLPWR_N pin toggled (for external power supply)
         * 8 -> KPDPWR_N pin toggled (power key pressed)
         */
        if (buf[0] == '3')
            property_set("ro.alarm_boot", "true");
        else
            property_set("ro.alarm_boot", "false");
    }
}

void check_device()
{
    struct sysinfo sys;

    sysinfo(&sys);

    if (sys.totalram > 2048ull * 1024 * 1024) {
        // from - phone-xxhdpi-3072-dalvik-heap.mk
        heapstartsize = "8m";
        heapgrowthlimit = "288m";
        heapsize = "768m";
        heapminfree = "2m";
    } else {
        // from - phone-xxhdpi-2048-dalvik-heap.mk
        heapstartsize = "16m";
        heapgrowthlimit = "192m";
        heapsize = "512m";
        heapminfree = "2m";
    }
}

/* 
 * In some device revisions, there is a sound amplifier that is not activated at startup. 
 * In stock init binary file, there is mention of the folder, which in theory is created when you connect the device in the sysfs section. 
 * If this method does not work, replace this piece of code with the working method or revert it.
 * If you find a more stable method, then replace it.
 * 
 * @Author BeYkeRYkt (21-09-2017)
 */
void check_aw87319(){
    DIR* dir = opendir("/sys/bus/i2c/drivers/AW87319_PA/2-0058");
    if (dir)
    {
        /* Directory exists. */
        //Based on: https://github.com/CyanogenMod/android_hardware_qcom_audio/commit/f6cfe88a8959aacbb0d1782265d4fba52c8854da
        property_set("ro.audio.customplatform", "AW87319");
        property_set("audio.acdb.name", "AW87319");
        property_set("persist.audio.calfile0", "/etc/acdbdata/AW87319/AW87319_Bluetooth_cal.acdb");
        property_set("persist.audio.calfile1", "/etc/acdbdata/AW87319/AW87319_General_cal.acdb");
        property_set("persist.audio.calfile2", "/etc/acdbdata/AW87319/AW87319_Global_cal.acdb");
        property_set("persist.audio.calfile3", "/etc/acdbdata/AW87319/AW87319_Handset_cal.acdb");
        property_set("persist.audio.calfile4", "/etc/acdbdata/AW87319/AW87319_Hdmi_cal.acdb");
        property_set("persist.audio.calfile5", "/etc/acdbdata/AW87319/AW87319_Headset_cal.acdb");
        property_set("persist.audio.calfile6", "/etc/acdbdata/AW87319/AW87319_Speaker_cal.acdb");
        closedir(dir);
    }
}

void vendor_load_properties()
{
    check_device();
    check_aw87319();

    property_set("dalvik.vm.heapstartsize", heapstartsize);
    property_set("dalvik.vm.heapgrowthlimit", heapgrowthlimit);
    property_set("dalvik.vm.heapsize", heapsize);
    property_set("dalvik.vm.heaptargetutilization", "0.75");
    property_set("dalvik.vm.heapminfree", heapminfree);
    property_set("dalvik.vm.heapmaxfree", "8m");

    property_set("qemu.hw.mainkeys", "0");

    std::string cmv = property_get("ro.boot.cmv");

    if (cmv == "mv1") {
        /* Swift 2 */
        property_set("ro.sf.lcd_density", "320");
        property_set("ro.media.maxmem", "10590068224");
    } else if (cmv == "mv2"){
        /* Swift 2 Plus*/
        property_set("ro.sf.lcd_density", "320");
    } else if (cmv == "mv3") {
        /* Swift 2 X */
        property_set("ro.sf.lcd_density", "440");
        property_set("persist.bootanimation.scale", "1.5");
        property_set("sys.lights.capabilities", "3");
    }

    init_alarm_boot_properties();
}

