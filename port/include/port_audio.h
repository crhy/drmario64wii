#ifndef PORT_AUDIO_H
#define PORT_AUDIO_H

/* Task 15c2: audio tasks run SYNCHRONOUSLY on the calling thread (naudio is
 * plain C and never touches GX), so the scheduler never has to yield the gfx
 * task to the RSP LWP for audio work. */
void port_audio_task_run(OSTask *t);

#endif
