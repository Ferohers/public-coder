#ifndef JSON_MINI_H
#define JSON_MINI_H

const char* json_find_value(const char* json, const char* key);
int json_extract_string(const char* json, const char* key, char* out, int maxLen);

#endif
