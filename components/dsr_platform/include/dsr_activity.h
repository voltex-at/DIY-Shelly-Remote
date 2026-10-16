#ifndef DSR_ACTIVITY_H
#define DSR_ACTIVITY_H
#include <stdbool.h>
/* Application-operation admission, not a storage mutex. Balanced begin/end
 * pairs span QUEUED -> running -> pending result/follow-up. Nested independent
 * operations are allowed. No lock is held during I/O. Begin refuses after
 * restart reservation, so checking an unrelated busy snapshot is unnecessary.
 * Only end an operation whose begin succeeded, exactly once. No ISR callers.
 * Readers need no admission unless they hand off a persistent follow-up. */
bool dsr_activity_begin(void);
void dsr_activity_end(void);
bool dsr_activity_restart_pending(void);
#endif
