#include <stdint.h>
#include <stddef.h>

uint32_t get_number_of_bytes_intesecting_page(uint32_t address, size_t page_size);

uint16_t swap_bytes_ba(uint16_t AB);
uint16_t swap_bytes_ab(uint16_t AB);
uint32_t swap_bytes_abcd(uint32_t ABCD);
uint32_t swap_bytes_badc(uint32_t ABCD);
uint32_t swap_bytes_cdab(uint32_t ABCD);
uint32_t swap_bytes_dcba(uint32_t ABCD);