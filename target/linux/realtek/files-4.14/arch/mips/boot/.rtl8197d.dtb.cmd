cmd_arch/mips/boot/dts/realtek/rtl8197d.dtb := mkdir -p arch/mips/boot/dts/realtek/ ; mips-openwrt-linux-musl-gcc -E -Wp,-MD,arch/mips/boot/dts/realtek/.rtl8197d.dtb.d.pre.tmp -nostdinc -I./scripts/dtc/include-prefixes -undef -D__DTS__ -x assembler-with-cpp -o arch/mips/boot/dts/realtek/.rtl8197d.dtb.dts.tmp arch/mips/boot/dts/realtek/rtl8197d.dts ; ./scripts/dtc/dtc -O dtb -o arch/mips/boot/dts/realtek/rtl8197d.dtb -b 0 -iarch/mips/boot/dts/realtek/ -i./scripts/dtc/include-prefixes -Wno-unit_address_vs_reg -Wno-simple_bus_reg -Wno-unit_address_format -Wno-pci_bridge -Wno-pci_device_bus_num -Wno-pci_device_reg  -d arch/mips/boot/dts/realtek/.rtl8197d.dtb.d.dtc.tmp arch/mips/boot/dts/realtek/.rtl8197d.dtb.dts.tmp ; cat arch/mips/boot/dts/realtek/.rtl8197d.dtb.d.pre.tmp arch/mips/boot/dts/realtek/.rtl8197d.dtb.d.dtc.tmp > arch/mips/boot/dts/realtek/.rtl8197d.dtb.d

source_arch/mips/boot/dts/realtek/rtl8197d.dtb := arch/mips/boot/dts/realtek/rtl8197d.dts

deps_arch/mips/boot/dts/realtek/rtl8197d.dtb := \
  arch/mips/boot/dts/realtek/rtl819x.dtsi \

arch/mips/boot/dts/realtek/rtl8197d.dtb: $(deps_arch/mips/boot/dts/realtek/rtl8197d.dtb)

$(deps_arch/mips/boot/dts/realtek/rtl8197d.dtb):
