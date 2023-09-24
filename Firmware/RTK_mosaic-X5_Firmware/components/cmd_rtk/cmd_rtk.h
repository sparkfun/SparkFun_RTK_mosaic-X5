/* Console example — various RTK commands

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Globals
extern int* mode;
extern char* ssid;
extern char* password;
extern char* esp_log_level;

void param_set_value_str(char **param, const char* val);
void param_set_value_int(int **param, const int val);
int set_log_level_by_str(const char *level_str);

// Register router functions
void register_rtk(void);
esp_err_t get_config_param_str(char* name, char** param);
esp_err_t get_config_param_int(char* name, int** param);

#ifdef __cplusplus
}
#endif