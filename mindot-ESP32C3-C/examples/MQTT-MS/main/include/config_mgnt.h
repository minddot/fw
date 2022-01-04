#ifndef __CONFIG_MGNT_H__
#define __CONFIG_MGNT_H__

void init_config(void);

char * get_sta_ssid(void);
char * get_sta_passwd(void);
char * get_ap_ssid(void);
char * get_ap_passwd(void);
char * get_tenant(void);
char * get_client_id(void);
char * get_broker(void); 

char * read_mqtt_config_info(char * file_path);

#endif /* __