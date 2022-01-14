set_project("AIR105")
set_xmakever("2.5.8")

set_version("0.0.2", {build = "%Y%m%d%H%M"})
add_rules("mode.debug", "mode.release")

local AIR105_VERSION
local luatos = "../LuatOS/"

local sdk_dir = "C:\\gcc-arm-none-eabi-10.3-2021.10\\"
if is_plat("linux") then
    sdk_dir = "/opt/gcc-arm-none-eabi-10.3-2021.10/"
elseif is_plat("windows") then
    sdk_dir = "C:\\gcc-arm-none-eabi-10.3-2021.10\\"
end

toolchain("arm_toolchain")
    set_kind("standalone")
    set_sdkdir(sdk_dir)
toolchain_end()

set_toolchains("arm_toolchain")
set_plat("cross")
set_arch("arm")

--add macro defination
add_defines("__AIR105_BSP__","__USE_XTL__","CMB_CPU_PLATFORM_TYPE=CMB_CPU_ARM_CORTEX_M4","HSE_VALUE=12000000","CORTEX_M4","__FLASH_APP_START_ADDR__=0x01010000","__LUATOS__")
add_defines("__FLASH_OTA_INFO_ADDR__=0x0100F000")
-- set warning all as error
set_warnings("allextra")
-- set_optimize("smallest")
-- set language: c11
set_languages("c11", "cxx11")

add_asflags("-mcpu=cortex-m4","-mfpu=fpv4-sp-d16","-mfloat-abi=hard","-mthumb","-c",{force = true})
-- add_arflags("-mcpu=cortex-m4","-mfpu=fpv4-sp-d16","-mfloat-abi=hard","-mthumb","-c","--specs=nano.specs","-ffunction-sections","-fdata-sections","-fstack-usage","-Og","-DTRACE_LEVEL=4",{force = true})
add_cxflags("-mcpu=cortex-m4","-mfpu=fpv4-sp-d16","-mfloat-abi=hard","-mthumb","-Og","-c","--specs=nano.specs","-ffunction-sections","-fdata-sections","-fstack-usage","-DTRACE_LEVEL=4",{force = true})

add_ldflags("-mcpu=cortex-m4","-mfpu=fpv4-sp-d16","-mfloat-abi=hard","-mthumb","-Og","--specs=nano.specs","--specs=nosys.specs","-Wl,--gc-sections","-Wl,--check-sections","-Wl,--cref","-Wl,--no-whole-archive","-lc_nano","-Wl,--no-whole-archive",{force = true})

set_dependir("$(buildir)/.deps")
set_objectdir("$(buildir)/.objs")

target("bootloader.elf")
    -- set kind
    set_kind("binary")
    set_targetdir("$(buildir)/out")
    -- add deps
    add_files("Third_Party/cm_backtrace/*.c",{public = true})
    --add_files("Third_Party/cm_backtrace/fault_handler/gcc/cmb_fault.S",{public = true})
    add_includedirs("Third_Party/cm_backtrace",{public = true})

    --add_files("bsp/common/*.c",{public = true})
	add_files("bsp/common/src/*.c",{public = true})
    add_includedirs("bsp/cmsis/include",{public = true})
    add_includedirs("bsp/common",{public = true})
	add_includedirs("bsp/common/include",{public = true})
    add_files("bsp/usb/**.c",{public = true})
    add_includedirs("bsp/usb/include",{public = true})
	add_includedirs("Third_Party/heap",{public = true})

    -- add files
    add_files("bsp/air105/platform/startup_full.s")
    add_files("bsp/air105/platform/bl_main.c")
    add_files("bsp/air105/hal/*.c")
    -- add_files("bsp/air105/chip/src/*.c")
    add_includedirs("bsp/air105/chip/include")
    add_includedirs("bsp/air105/include")

    add_ldflags("-Wl,-Map=./build/out/bootloader.map","-Wl,-T$(projectdir)/project/air105/bl.ld",{force = true})

	after_build(function(target)
        os.exec(sdk_dir .. "bin/arm-none-eabi-objcopy -O binary --gap-fill=0xff $(buildir)/out/bootloader.elf $(buildir)/out/bootloader.bin")
        os.exec(sdk_dir .. "bin/arm-none-eabi-objcopy -O ihex $(buildir)/out/bootloader.elf $(buildir)/out/bootloader.hex")
        io.writefile("$(buildir)/out/bootloader.list", os.iorun(sdk_dir .. "bin/arm-none-eabi-objdump -h -S $(buildir)/out/bootloader.elf"))
        io.writefile("$(buildir)/out/bootloader.size", os.iorun(sdk_dir .. "bin/arm-none-eabi-size $(buildir)/out/bootloader.elf"))
        -- os.run(sdk_dir .. "bin/arm-none-eabi-objdump -h -S $(buildir)/out/bootloader.elf > $(buildir)/out/bootloader.list")
        -- os.run(sdk_dir .. "bin/arm-none-eabi-size $(buildir)/out/bootloader.elf > $(buildir)/out/bootloader.size")
        io.cat("$(buildir)/out/bootloader.size")
    end)
target_end()

target("lvgl")
    set_kind("static")
    
    on_load(function (target)
        local conf_data = io.readfile("$(projectdir)/application/include/luat_conf_bsp.h")
        local LVGL_CONF = conf_data:find("// #define LUAT_USE_LVGL\n")
        if LVGL_CONF == nil then target:set("default", true) else target:set("default", false) end
    end)

    add_files(luatos.."components/lvgl/**.c")

    add_includedirs("application/include")
    add_includedirs("bsp/air105/include",{public = true})
    --add_includedirs("bsp/common",{public = true})
	add_includedirs("bsp/common/include",{public = true})
    add_includedirs("bsp/cmsis/include",{public = true})
    add_includedirs(luatos.."luat/packages/lfs")
    add_includedirs(luatos.."lua/include",{public = true})
    add_includedirs(luatos.."luat/include",{public = true})
    add_includedirs(luatos.."components/lcd",{public = true})
    add_includedirs(luatos.."components/lvgl",{public = true})
    add_includedirs(luatos.."components/lvgl/binding",{public = true})
    add_includedirs(luatos.."components/lvgl/gen",{public = true})
    add_includedirs(luatos.."components/lvgl/src",{public = true})
    add_includedirs(luatos.."components/lvgl/font",{public = true})
    add_includedirs(luatos.."luat/packages/u8g2",{public = true})

    set_targetdir("$(buildir)/lib")
target_end()

target("app.elf")
    -- set kind
    set_kind("binary")
    set_targetdir("$(buildir)/out")
    add_defines("__BUILD_APP__","__BUILD_OS__","CMB_USING_OS_PLATFORM","CMB_OS_PLATFORM_TYPE=CMB_OS_PLATFORM_FREERTOS")

    on_load(function (target)
        local conf_data = io.readfile("$(projectdir)/application/include/luat_conf_bsp.h")
        AIR105_VERSION = conf_data:match("#define LUAT_BSP_VERSION \"(%w+)\"")
        local LVGL_CONF = conf_data:find("// #define LUAT_USE_LVGL\n")
        if LVGL_CONF == nil then target:add("deps", "lvgl") end
    end)
    
    -- add deps
    add_files("Third_Party/cm_backtrace/*.c",{public = true})
    --add_files("Third_Party/cm_backtrace/fault_handler/gcc/cmb_fault.S",{public = true})
    add_includedirs("Third_Party/cm_backtrace",{public = true})

    add_files("Third_Party/ZBAR/src/*.c",{public = true})
    add_includedirs("Third_Party/ZBAR/include",{public = true})

    add_files("Third_Party/iconv/src/*.c",{public = true})
    add_includedirs("Third_Party/iconv/include",{public = true})
    
    --add_files("bsp/common/*.c",{public = true})
	add_files("bsp/common/src/*.c",{public = true})
    add_includedirs("bsp/cmsis/include",{public = true})
    add_includedirs("bsp/common",{public = true})
	add_includedirs("bsp/common/include",{public = true})
    add_files("bsp/usb/**.c",{public = true})
    add_includedirs("bsp/usb/include",{public = true})

    -- add files
    add_files("bsp/air105/platform/startup_full.s")
    add_files("bsp/air105/platform/app_main.c")
    add_files("bsp/air105/hal/*.c")
    -- add_files("bsp/air105/chip/src/*.c")

    add_files("os/FreeRTOS_v10/GCC/ARM_CM4F/*.c",{public = true})
    add_files("os/FreeRTOS_v10/src/*.c",{public = true})

    add_includedirs("bsp/air105/chip/include",{public = true})
    add_includedirs("bsp/air105/include",{public = true})
    add_includedirs("os/FreeRTOS_v10/GCC/ARM_CM4F",{public = true})
    add_includedirs("os/FreeRTOS_v10/include",{public = true})

    add_includedirs("bsp/cmsis/include",{public = true})
    add_includedirs("bsp/air105/chip/include",{public = true})
    add_includedirs("bsp/air105/include",{public = true})

    add_files("application/src/*.c")

    add_files(luatos.."lua/src/*.c")
    add_files(luatos.."luat/modules/*.c")
    add_files(luatos.."luat/vfs/*.c")
    del_files(luatos.."luat/vfs/luat_fs_posix.c")

    add_files(luatos.."components/lcd/*.c")
    add_files(luatos.."components/sfd/*.c")
    add_files(luatos.."components/sfud/*.c")
    add_files(luatos.."components/statem/*.c")
    add_files(luatos.."components/nr_micro_shell/*.c")
    add_files(luatos.."luat/packages/eink/*.c")
    add_files(luatos.."luat/packages/epaper/*.c")
    del_files(luatos.."luat/packages/epaper/GUI_Paint.c")
    add_files(luatos.."luat/packages/iconv/*.c")
    add_files(luatos.."luat/packages/lfs/*.c")
    add_files(luatos.."luat/packages/lua-cjson/*.c")
    add_files(luatos.."luat/packages/minmea/*.c")
    add_files(luatos.."luat/packages/qrcode/*.c")
    add_files(luatos.."luat/packages/u8g2/*.c")
    add_files(luatos.."luat/packages/fatfs/*.c")
    add_files(luatos.."luat/weak/*.c")
    add_files(luatos.."components/coremark/*.c")
    add_files(luatos.."components/cjson/*.c")
    
    add_includedirs("application/include",{public = true})
    add_includedirs(luatos.."lua/include",{public = true})
    add_includedirs(luatos.."luat/include",{public = true})
    add_includedirs(luatos.."components/lcd",{public = true})
    add_includedirs(luatos.."components/sfud",{public = true})
    add_includedirs(luatos.."components/statem",{public = true})
    add_includedirs(luatos.."components/coremark",{public = true})
    add_includedirs(luatos.."components/cjson",{public = true})
    
    add_includedirs(luatos.."luat/packages/eink")
    add_includedirs(luatos.."luat/packages/epaper")
    add_includedirs(luatos.."luat/packages/iconv")
    add_includedirs(luatos.."luat/packages/lfs")
    add_includedirs(luatos.."luat/packages/lua-cjson")
    add_includedirs(luatos.."luat/packages/minmea")
    add_includedirs(luatos.."luat/packages/qrcode")
    add_includedirs(luatos.."luat/packages/u8g2")
    add_includedirs(luatos.."luat/packages/fatfs")

    -- gtfont
    add_includedirs(luatos.."components/gtfont",{public = true})
    add_files(luatos.."components/gtfont/*.c")

    
    add_files(luatos.."components/flashdb/src/*.c")
    add_files(luatos.."components/fal/src/*.c")
    add_includedirs(luatos.."components/flashdb/inc",{public = true})
    add_includedirs(luatos.."components/fal/inc",{public = true})

    add_files(luatos.."components/mbedtls/library/*.c")
    add_includedirs(luatos.."components/mbedtls/include")

    add_files(luatos.."luat/packages/zlib/*.c")
    add_includedirs(luatos.."luat/packages/zlib")

    add_files(luatos.."components/camera/*.c")
    add_includedirs(luatos.."components/camera")

    add_ldflags("-Wl,--whole-archive -Wl,--start-group ./lib/libgt.a -Wl,--end-group -Wl,--no-whole-archive","-Wl,-Map=./build/out/app.map","-Wl,-T$(projectdir)/project/air105/app.ld",{force = true})

	after_build(function(target)
        os.exec(sdk_dir .. "bin/arm-none-eabi-objcopy -O binary --gap-fill=0xff $(buildir)/out/app.elf $(buildir)/out/app.bin")
        os.exec(sdk_dir .. "bin/arm-none-eabi-objcopy -O ihex $(buildir)/out/app.elf $(buildir)/out/app.hex")
        io.writefile("$(buildir)/out/app.list", os.iorun(sdk_dir .. "bin/arm-none-eabi-objdump -h -S $(buildir)/out/app.elf"))
        io.writefile("$(buildir)/out/app.size", os.iorun(sdk_dir .. "bin/arm-none-eabi-size $(buildir)/out/app.elf"))
        -- os.run(sdk_dir .. "bin/arm-none-eabi-objdump -h -S $(buildir)/out/app.elf > $(buildir)/out/app.list")
        -- os.run(sdk_dir .. "bin/arm-none-eabi-size $(buildir)/out/app.elf > $(buildir)/out/app.size")
        io.cat("$(buildir)/out/app.size")

        import("lib.detect.find_file")
        local path7z = nil
        if is_plat("windows") then
            path7z = find_file("7z.exe", { "C:/Program Files/7-Zip/", "D:/Program Files/7-Zip/", "E:/Program Files/7-Zip/"})
        elseif is_plat("linux") then
            path7z = find_file("7z", { "/usr/bin/"})
            if not path7z then
                path7z = find_file("7zr", { "/usr/bin/"})
            end
        else
            path7z = find_file("7z.exe", { "C:/Program Files/7-Zip/" })
        end
        if path7z then
            os.cp("./project/air105/info.json", "$(buildir)/out/info.json")
            os.cp("./project/air105/soc_download.exe", "$(buildir)/out/soc_download.exe")
            os.cd("$(buildir)/out")
            os.run(path7z.." a LuatOS-SoC_"..AIR105_VERSION.."_AIR105.soc info.json bootloader.bin app.bin soc_download.exe")
            os.rm("$(buildir)/out/info.json")
            os.rm("$(buildir)/out/soc_download.exe")
        else
            print("7z not find")
            return
        end

    end)
target_end()
