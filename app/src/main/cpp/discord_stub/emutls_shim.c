/* Modified by EoN Launcher V2.
 * Working implementation of __emutls_get_address (emulated thread-local storage).
  * The prebuilt libgxcore.so expects this symbol from libdiscord_partner_sdk.so,
   * which we replace with a stand-in. Behaviour follows the LLVM compiler-rt emutls contract. */
   #include <pthread.h>
   #include <stddef.h>
   #include <stdint.h>
   #include <stdlib.h>
   #include <string.h>

   typedef struct emutls_control {
        size_t size;
            size_t align;
                union { uintptr_t index; void *address; } object;
                    void *value;
   } emutls_control;

   typedef struct { size_t count; void **slots; } emutls_thread;

   static pthread_key_t emutls_key;
   static pthread_once_t emutls_once = PTHREAD_ONCE_INIT;
   static pthread_mutex_t emutls_mutex = PTHREAD_MUTEX_INITIALIZER;
   static uintptr_t emutls_next_index = 0;

   static void emutls_destroy(void *p) {
        emutls_thread *t = (emutls_thread *)p;
            if (!t) return;
                for (size_t i = 0; i < t->count; i++) free(t->slots[i]);
                    free(t->slots);
                        free(t);
   }

   static void emutls_make_key(void) { pthread_key_create(&emutls_key, emutls_destroy); }

   __attribute__((visibility("default")))
   void *__emutls_get_address(emutls_control *c) {
        uintptr_t idx = __atomic_load_n(&c->object.index, __ATOMIC_ACQUIRE);
            if (!idx) {
                        pthread_mutex_lock(&emutls_mutex);
                                idx = __atomic_load_n(&c->object.index, __ATOMIC_ACQUIRE);
                                        if (!idx) {
                                                        idx = ++emutls_next_index;
                                                                    __atomic_store_n(&c->object.index, idx, __ATOMIC_RELEASE);
                                        }
                                                pthread_mutex_unlock(&emutls_mutex);
            }
                pthread_once(&emutls_once, emutls_make_key);
                    emutls_thread *t = (emutls_thread *)pthread_getspecific(emutls_key);
                        if (!t) {
                                    t = (emutls_thread *)calloc(1, sizeof(*t));
                                            if (!t) abort();
                                                    pthread_setspecific(emutls_key, t);
                        }
                            if (idx > t->count) {
                                        size_t n = (size_t)idx + 8;
                                                void **s = (void **)realloc(t->slots, n * sizeof(void *));
                                                        if (!s) abort();
                                                                memset(s + t->count, 0, (n - t->count) * sizeof(void *));
                                                                        t->slots = s;
                                                                                t->count = n;
                            }
                                void **slot = &t->slots[idx - 1];
                                    if (!*slot) {
                                                size_t a = c->align < sizeof(void *) ? sizeof(void *) : c->align;
                                                        size_t sz = c->size ? c->size : 1;
                                                                void *p = NULL;
                                                                        if (posix_memalign(&p, a, sz)) abort();
                                                                                if (c->value) memcpy(p, c->value, c->size); else memset(p, 0, sz);
                                                                                        *slot = p;
                                    }
                                        return *slot;
   }
