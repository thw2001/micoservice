#include "om_fmt.h"

#include "om_log.h"
#include "om_msg.h"

#define GET_CAPITAL(_c) (isupper(_c) ? _c : toupper(_c))
/* MSG */
#define ADD2LIST ('A')
#define FILTER_FLAG ('F')
#define SUBER_CB_FLAG ('D')
#define LINK_FLAG ('L')
#define STATIC_LINK_FLAG ('K')
#define SUBER_FLAG ('S')
#define TOPIC_FLAG ('T')
#define STATIC_TOPIC_FLAG ('E')
#define CACHE_FLAG ('C')
#define STATIC_CACHE_FLAG ('X')

/* AFL */
#define DECOMPOSE_FLAG ('D')
#define LIST_FLAG ('L')
#define RANGE_FLAG ('R')

om_topic_t* om_config_topic(om_topic_t* topic, const char* format, ...) {
  va_list valist;
  va_start(valist, format);

  if (!topic) {
    const char* name = va_arg(valist, const char*);
    uint32_t len = va_arg(valist, uint32_t);
    topic = om_core_topic_create(name, len);
  }

  if (format == NULL) {
    va_end(valist);
    return topic;
  }

  const uint8_t* index;

  for (index = (const uint8_t*)format; *index != '\0'; index++) {
    OM_ASSERT(isalpha(*index));

    switch (GET_CAPITAL(*index)) {
      case FILTER_FLAG:
        topic->user_fun.filter = va_arg(valist, om_user_fun_t);
        topic->user_fun.filter_arg = va_arg(valist, void*);

        OM_ASSERT(topic->user_fun.filter);
        break;
      case LINK_FLAG:
        om_core_link(topic, va_arg(valist, om_topic_t*));
        break;
      case STATIC_LINK_FLAG: {
        om_suber_t* suber = va_arg(valist, om_suber_t*);
        om_link_t* link = va_arg(valist, om_link_t*);
        om_topic_t* target = va_arg(valist, om_topic_t*);

        om_core_link_static(suber, link, topic, target);
        break;
      }
      case SUBER_FLAG:
        om_core_add_suber(topic, va_arg(valist, om_suber_t*));
        break;
      case TOPIC_FLAG:
        om_core_link(va_arg(valist, om_topic_t*), topic);
        break;
      case STATIC_TOPIC_FLAG: {
        om_suber_t* suber = va_arg(valist, om_suber_t*);
        om_link_t* link = va_arg(valist, om_link_t*);
        om_topic_t* source = va_arg(valist, om_topic_t*);
        om_core_link_static(suber, link, source, topic);
        break;
      }
      case CACHE_FLAG:
        topic->msg.buff = om_malloc(topic->buff_len);
        topic->virtual_mode = false;
        break;
      case STATIC_CACHE_FLAG:
        topic->msg.buff = va_arg(valist, void*);
        topic->virtual_mode = false;
        break;
      case ADD2LIST: {
        om_add_topic(topic);
        break;
      }
      case SUBER_CB_FLAG: {
        om_user_fun_t fun = va_arg(valist, om_user_fun_t);
        void* arg = va_arg(valist, void*);
        om_config_suber(NULL, "DT", fun, arg, topic);
      } break;
      default:
        OM_ASSERT(false);
        va_end(valist);
        return NULL;
    }
  }
  va_end(valist);

  return topic;
}

om_suber_t* om_config_suber(om_suber_t* suber, const char* format, ...) {
  if (!suber)
    suber = om_core_suber_create(NULL);
  else
    om_create_suber_static(suber, NULL);
  if (format == NULL) return suber;

  va_list valist;
  va_start(valist, format);

  const uint8_t* index;

  for (index = (const uint8_t*)format; *index != '\0'; index++) {
    OM_ASSERT(isalpha(*index));
    switch (GET_CAPITAL(*index)) {
      case SUBER_CB_FLAG:
        suber->mode = OM_SUBER_MODE_DEFAULT;

        suber->data.as_suber.sub_callback = va_arg(valist, om_user_fun_t);
        suber->data.as_suber.sub_cb_arg = va_arg(valist, void*);

        OM_ASSERT(suber->data.as_suber.sub_callback);
        break;
      case TOPIC_FLAG:
        om_core_add_suber(va_arg(valist, om_topic_t*), suber);
        break;
      default:
        OM_ASSERT(false);
        va_end(valist);
        return NULL;
    }
  }
  va_end(valist);

  return suber;
}
