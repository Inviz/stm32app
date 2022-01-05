#include "bytes.h"

uint16_t swap_bytes_ba(uint16_t AB) {
    return (AB>>8)|(AB<<8);
}
uint16_t swap_bytes_ab(uint16_t AB) {
    return AB;
}
uint32_t swap_bytes_abcd(uint32_t ABCD) {
    return ABCD;
}
uint32_t swap_bytes_badc(uint32_t ABCD) {
    return ((ABCD>>8)&0xff0000) | ((ABCD<<8)&0xff000000) | ((ABCD<<8)&0xff00) | ((ABCD>>8)&0xff);
}
uint32_t swap_bytes_cdab(uint32_t ABCD) {
    return ((ABCD>>16)&0xff00) | ((ABCD>>16)&0xff) | ((ABCD<<16)&0xff000000) | ((ABCD<<16)&0xff0000);
}
uint32_t swap_bytes_dcba(uint32_t ABCD) {
    return ((ABCD>>24)&0xff) | ((ABCD<<8)&0xff0000) | ((ABCD>>8)&0xff00) | ((ABCD<<24)&0xff000000);
}
