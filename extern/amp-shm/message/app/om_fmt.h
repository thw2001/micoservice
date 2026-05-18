#ifndef __OM_FMT_H__
#define __OM_FMT_H__

#include "om_core.h"

om_topic_t* om_config_topic(om_topic_t* topic, const char* format, ...);

// 注意，使用这个方式创建的订阅者只能用回调
// om_subscribe创建的订阅者只能用export
om_suber_t* om_config_suber(om_suber_t* suber, const char* format, ...);

#endif
