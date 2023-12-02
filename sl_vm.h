/**
 * (c) 2023 David Rowbotham thedaver64@gmail.com
*/

#pragma once

void *virtual_alloc(const size_t numbytes);
void virtual_free(void *pointer);
void virtual_stats();
