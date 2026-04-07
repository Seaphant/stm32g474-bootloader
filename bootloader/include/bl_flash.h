#ifndef BL_FLASH_H
#define BL_FLASH_H

#include <stdint.h>

int  flash_unlock(void);
void flash_lock(void);
int  flash_erase_page(uint32_t page);
int  flash_erase_app_region(void);
int  flash_write(uint32_t address, const uint8_t *data, uint32_t len);

#endif /* BL_FLASH_H */
