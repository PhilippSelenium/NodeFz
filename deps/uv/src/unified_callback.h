#ifndef UV_UNIFIED_CALLBACK_H_
#define UV_UNIFIED_CALLBACK_H_

#include "list.h"
#include <sys/types.h> /* struct sockaddr_storage */
#include <sys/socket.h>

/* Unified callback queue. */
#define UNIFIED_CALLBACK 1
#define GRAPHVIZ 1

#ifndef NOT_REACHED
#define NOT_REACHED assert (0 == 1);
#endif

#define MAX_CALLBACK_NARGS 5
enum callback_type
{
  /* include/uv.h */
  CALLBACK_TYPE_MIN = 0,
  UV_ALLOC_CB = CALLBACK_TYPE_MIN,
  UV_READ_CB,
  UV_WRITE_CB,
  UV_CONNECT_CB,
  UV_SHUTDOWN_CB,
  UV_CONNECTION_CB,
  UV_CLOSE_CB,
  UV_POLL_CB,
  UV_TIMER_CB,
  UV_ASYNC_CB,
  UV_PREPARE_CB,
  UV_CHECK_CB,
  UV_IDLE_CB,
  UV_EXIT_CB,
  UV_WALK_CB,
  UV_FS_CB,
  UV_WORK_CB,
  UV_AFTER_WORK_CB,
  UV_GETADDRINFO_CB,
  UV_GETNAMEINFO_CB,
  UV_FS_EVENT_CB,
  UV_FS_POLL_CB,
  UV_SIGNAL_CB,
  UV_UDP_SEND_CB,
  UV_UDP_RECV_CB,
  UV_THREAD_CB,

  /* include/uv-unix.h */
  UV__IO_CB,
  UV__ASYNC_CB,

  /* include/uv-threadpool.h */
  UV__WORK_WORK,
  UV__WORK_DONE,
  CALLBACK_TYPE_MAX
};

/* Nodes that comprise a callback tree. */
struct callback_node
{
  struct callback_info *info; /* Description of this callback. */
  int level; /* What level in the callback tree is it? For root nodes this is 0. */
  struct callback_node *parent; /* Who started us? For root nodes this is NULL. */

  int client_id; /* My internal ID of client incurring this CB. -1 if unknown. -2 if init stack node. */
  struct sockaddr_storage *client_addr; /* Info about the client. */

  time_t relative_start; /* Seconds since program began. */
  struct timeval start;
  struct timeval stop;
  long long duration; /* Number of microseconds elapsed. */
  int active; /* 1 if callback active, 0 if finished. */

  int id; /* Unique ID for this node. */

  struct list children; /* Linked list of children. */
  
  struct list_elem global_order_elem; /* For inclusion in the global callback order. */
  struct list_elem child_elem; /* For inclusion in parent's list of children. */
  struct list_elem root_elem; /* For root nodes: inclusion in list of root nodes. */
};

void current_callback_node_set (struct callback_node *);
struct callback_node * current_callback_node_get (void);
struct callback_node * get_init_stack_callback_node (void);
struct callback_node * invoke_callback (struct callback_info *);

char * callback_type_to_string (enum callback_type);

#define INIT_CBI(cb_type, cb_p) \
  struct callback_info *cbi_p = malloc(sizeof *cbi_p); \
  assert(cbi_p != NULL); \
  memset(cbi_p, 0, sizeof(*cbi_p)); \
  cbi_p->type = cb_type; \
  cbi_p->cb = cb_p;

/* Macros to prep a CBI for invoke_callback, with 0-5 args. */
#define PREP_CBI_0(type, cb) \
  INIT_CBI(type, cb)
#define PREP_CBI_1(type, cb, arg0) \
  PREP_CBI_0(type, cb) \
  cbi_p->args[0] = (long) arg0;
#define PREP_CBI_2(type, cb, arg0, arg1) \
  PREP_CBI_1(type, cb, arg0) \
  cbi_p->args[1] = (long) arg1;
#define PREP_CBI_3(type, cb, arg0, arg1, arg2) \
  PREP_CBI_2(type, cb, arg0, arg1) \
  cbi_p->args[2] = (long) arg2;
#define PREP_CBI_4(type, cb, arg0, arg1, arg2, arg3) \
  PREP_CBI_3(type, cb, arg0, arg1, arg2) \
  cbi_p->args[3] = (long) arg3;
#define PREP_CBI_5(type, cb, arg0, arg1, arg2, arg3, arg4) \
  PREP_CBI_4(type, cb, arg0, arg1, arg2, arg3) \
  cbi_p->args[4] = (long) arg4;

/* Macros to invoke a callback, with 0-5 args.
   The internally-generated callback node describing the 
   invoked callback is set to callback_cbn. */
#define INVOKE_CALLBACK_0(type, cb) \
  PREP_CBI_0(type, cb) \
  struct callback_node *callback_cbn = invoke_callback(cbi_p);
#define INVOKE_CALLBACK_1(type, cb, arg0) \
  PREP_CBI_1(type, cb, arg0) \
  struct callback_node *callback_cbn = invoke_callback(cbi_p);
#define INVOKE_CALLBACK_2(type, cb, arg0, arg1) \
  PREP_CBI_2(type, cb, arg0, arg1) \
  struct callback_node *callback_cbn = invoke_callback(cbi_p);
#define INVOKE_CALLBACK_3(type, cb, arg0, arg1, arg2) \
  PREP_CBI_3(type, cb, arg0, arg1, arg2) \
  struct callback_node *callback_cbn = invoke_callback(cbi_p);
#define INVOKE_CALLBACK_4(type, cb, arg0, arg1, arg2, arg3) \
  PREP_CBI_4(type, cb, arg0, arg1, arg2, arg3) \
  struct callback_node *callback_cbn = invoke_callback(cbi_p);
#define INVOKE_CALLBACK_5(type, cb, arg0, arg1, arg2, arg3, arg4) \
  PREP_CBI_5(type, cb, arg0, arg1, arg2, arg3, arg4) \
  struct callback_node *callback_cbn = invoke_callback(cbi_p);

/* Description of a callback. */
struct callback_info
{
  enum callback_type type;
  void (*cb)();
  long args[MAX_CALLBACK_NARGS]; /* Must be wide enough for the widest arg type. Seems to be 8 bytes. */
};

time_t get_relative_time (void);

void dump_callback_global_order (void);
void dump_callback_trees (void);

void dump_callback_global_order_sighandler (int);
void dump_callback_trees_sighandler (int);
void dump_all_trees_and_exit_sighandler (int);

#endif /* UV_UNIFIED_CALLBACK_H_ */
