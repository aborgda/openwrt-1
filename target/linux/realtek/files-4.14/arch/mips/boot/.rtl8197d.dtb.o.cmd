cmd_arch/mips/boot/dts/realtek/rtl8197d.dtb.o := mips-openwrt-linux-musl-gcc -Wp,-MD,arch/mips/boot/dts/realtek/.rtl8197d.dtb.o.d  -nostdinc -isystem /Users/gaspare/anlix/openwrt/staging_dir/toolchain-mips_lx53_gcc-7.3.0_musl/lib/gcc/mips-openwrt-linux-musl/7.3.0/include -I./arch/mips/include -I./arch/mips/include/generated  -I./include -I./arch/mips/include/uapi -I./arch/mips/include/generated/uapi -I./include/uapi -I./include/generated/uapi -include ./include/linux/kconfig.h -D__KERNEL__ -DVMLINUX_LOAD_ADDRESS=0xffffffff80000000 -DDATAOFFSET=0 -D__ASSEMBLY__ -fno-PIE -mno-check-zero-division -mabi=32 -G 0 -mno-abicalls -fno-pic -pipe -mno-branch-likely -msoft-float -DGAS_HAS_SET_HARDFLOAT -Wa,-msoft-float -ffreestanding -fno-stack-check -march=lx5380 -DTOOLCHAIN_SUPPORTS_VIRT -I./arch/mips/include/asm/mach-realtek/rtl8197d -I./arch/mips/include/asm/mach-generic -DCC_HAVE_ASM_GOTO   -c -o arch/mips/boot/dts/realtek/rtl8197d.dtb.o arch/mips/boot/dts/realtek/rtl8197d.dtb.S

source_arch/mips/boot/dts/realtek/rtl8197d.dtb.o := arch/mips/boot/dts/realtek/rtl8197d.dtb.S

deps_arch/mips/boot/dts/realtek/rtl8197d.dtb.o := \
  include/linux/compiler_types.h \
    $(wildcard include/config/have/arch/compiler/h.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  include/asm-generic/vmlinux.lds.h \
    $(wildcard include/config/ld/dead/code/data/elimination.h) \
    $(wildcard include/config/hotplug/cpu.h) \
    $(wildcard include/config/memory/hotplug.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/kprobes.h) \
    $(wildcard include/config/event/tracing.h) \
    $(wildcard include/config/tracing.h) \
    $(wildcard include/config/ftrace/syscalls.h) \
    $(wildcard include/config/serial/earlycon.h) \
    $(wildcard include/config/timer/of.h) \
    $(wildcard include/config/irqchip.h) \
    $(wildcard include/config/common/clk.h) \
    $(wildcard include/config/of/iommu.h) \
    $(wildcard include/config/of/reserved/mem.h) \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/cpu/idle.h) \
    $(wildcard include/config/acpi.h) \
    $(wildcard include/config/constructors.h) \
    $(wildcard include/config/generic/bug.h) \
    $(wildcard include/config/unwinder/orc.h) \
    $(wildcard include/config/pm/trace.h) \
    $(wildcard include/config/blk/dev/initrd.h) \
  include/linux/export.h \
    $(wildcard include/config/have/underscore/symbol/prefix.h) \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/module/rel/crcs.h) \
    $(wildcard include/config/trim/unused/ksyms.h) \
    $(wildcard include/config/unused/symbols.h) \

arch/mips/boot/dts/realtek/rtl8197d.dtb.o: $(deps_arch/mips/boot/dts/realtek/rtl8197d.dtb.o)

$(deps_arch/mips/boot/dts/realtek/rtl8197d.dtb.o):
