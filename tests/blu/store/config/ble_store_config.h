#pragma once
union ble_store_value;
union ble_store_key;
void ble_store_config_init(void);
int ble_store_config_write(int,const union ble_store_value *);
int ble_store_config_delete(int,const union ble_store_key *);
