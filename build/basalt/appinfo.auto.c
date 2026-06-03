#include "pebble_process_info.h"
#include "src/resource_ids.auto.h"

const PebbleProcessInfo __pbl_app_info __attribute__ ((section (".pbl_header"))) = {
  .header = "PBLAPP",
  .struct_version = { PROCESS_INFO_CURRENT_STRUCT_VERSION_MAJOR, PROCESS_INFO_CURRENT_STRUCT_VERSION_MINOR },
  .sdk_version = { PROCESS_INFO_CURRENT_SDK_VERSION_MAJOR, PROCESS_INFO_CURRENT_SDK_VERSION_MINOR },
  .process_version = { 1, 0 },
  .load_size = 0xb6b6,
  .offset = 0xb6b6b6b6,
  .crc = 0xb6b6b6b6,
  .name = "Iron",
  .company = "1987haaa",
  .icon_resource_id = RESOURCE_ID_IMAGE_APP_ICON,
  .sym_table_addr = 0xA7A7A7A7,
  .flags = PROCESS_INFO_PLATFORM_BASALT,
  .num_reloc_entries = 0xdeadcafe,
  .uuid = { 0x0E, 0xC7, 0x19, 0x71, 0x11, 0x91, 0x4E, 0x05, 0x87, 0xF5, 0x27, 0xA3, 0xC7, 0x49, 0x02, 0x3C },
  .virtual_size = 0xb6b6
};
