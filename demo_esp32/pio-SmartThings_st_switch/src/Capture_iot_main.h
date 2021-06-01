#ifndef _IOT_MAIN_H_
#define _IOT_MAIN_H_

#include "Capture_st_dev.h"

typedef struct iot_cap_handle_list iot_cap_handle_list_t;

struct iot_context {
	iot_cap_handle_list_t *cap_handle_list;		/**< @brief allocated capability handle lists */

	st_cap_noti_cb noti_cb;		/**< @brief notification handling callback for each capability */
	void *noti_usr_data;		/**< @brief notification handling callback data for user */
};


#endif // _IOT_MAIN_H_