#define MAX_THREAD_SIZE 1024 
#define DEFAULT_STACK_SIZE 16384 // 16KB


/* Thread Status */
#define ACTIVE          0x00
#define INTERRUPTED     0x01
#define KILLED          0x10

typedef int lwt_t;


typedef struct lwt_tcb_struct {
        int     tid;
        int     status;
	void *  stack;
	struct lwt_tcb_struct *parent; //parent tcb
	struct lwt_tcb_struct *next; //run queue next pointer
        } lwt_tcb;


typedef void *(*lwt_fn_t)(void *);

/* lwt functions */
lwt_t lwt_create(lwt_fn_t fn, void *data);
void *lwt_join(lwt_t lwt);
void lwt_die(void *val);
int lwt_yield(lwt_t lwt);
lwt_t lwt_current(void);
int lwt_id(lwt_t lwt);

/* private lwt functions */
void __lwt_schedule(void);
void __lwt_dispatch(lwt_t next, lwt_t current);
extern void __lwt_trampoline();
extern void __lwt_trampoline_test();
void __lwt_start();
void __lwt_start_test();

