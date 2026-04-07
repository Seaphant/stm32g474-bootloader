#ifndef BL_JUMP_H
#define BL_JUMP_H

#include <stdint.h>
#include "image_header.h"

int  validate_image(const image_header_t *hdr);
void jump_to_application(uint32_t vector_table_addr) __attribute__((noreturn));

#endif /* BL_JUMP_H */
