// ============================================================
//  third_party/stb/stb_image.h
//  Placeholder - actual stb_image.h must be cloned from
//  https://github.com/nothings/stb into this directory
//
//  Setup script will clone this automatically.
// ============================================================
#pragma once

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION

// Real implementation - download from https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
// The setup_debian.sh script will fetch this.

#ifndef STBI_INCLUDE_STB_IMAGE_H
#define STBI_INCLUDE_STB_IMAGE_H

#include <stdlib.h>

typedef unsigned char stbi_uc;

#ifdef __cplusplus
extern "C" {
#endif

extern stbi_uc *stbi_load(const char *filename, int *x, int *y, int *channels_in_file, int desired_channels);
extern stbi_uc *stbi_load_from_memory(const stbi_uc *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);
extern void     stbi_image_free(void *retval_from_stbi_load);
extern void     stbi_set_flip_vertically_on_load(int flag_true_if_should_flip);
extern const char *stbi_failure_reason(void);

#ifdef __cplusplus
}
#endif

#endif // STBI_INCLUDE_STB_IMAGE_H
