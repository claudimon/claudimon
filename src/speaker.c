#include "speaker.h"

/* ============================================
   PC SPEAKER DRIVER
   
   The PC speaker is controlled via:
   - PIT channel 2 (ports 0x42, 0x43) sets frequency
   - Port 0x61 (keyboard controller) gates the speaker
   ============================================ */

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

#define PIT_BASE_FREQ 1193180  /* PIT oscillator frequency in Hz */

/* ---- Simple busy-wait delay (approximate milliseconds) ---- */
static void delay_ms(uint32_t ms) {
    /* Each iteration is roughly 1 microsecond on a ~1GHz CPU in QEMU */
    for (uint32_t i = 0; i < ms * 400; i++) {
        __asm__ volatile ("nop");
    }
}

/* ---- Start playing a frequency ---- */
void speaker_play(uint32_t frequency) {
    if (frequency == 0) { speaker_stop(); return; }

    uint32_t divisor = PIT_BASE_FREQ / frequency;

    /* Set PIT channel 2 to square wave mode */
    outb(0x43, 0xB6);                        /* channel 2, square wave */
    outb(0x42, (uint8_t)(divisor & 0xFF));   /* low byte */
    outb(0x42, (uint8_t)(divisor >> 8));     /* high byte */

    /* Enable speaker via port 0x61 (bits 0 and 1) */
    uint8_t tmp = inb(0x61);
    outb(0x61, tmp | 0x03);
}

/* ---- Stop the speaker ---- */
void speaker_stop(void) {
    uint8_t tmp = inb(0x61);
    outb(0x61, tmp & ~0x03);  /* Clear bits 0 and 1 */
}

/* ---- Short beep ---- */
void speaker_beep(void) {
    speaker_play(1000);
    delay_ms(100);
    speaker_stop();
}

/* ---- Error sound (low descending tone) ---- */
void speaker_error_sound(void) {
    speaker_play(400);
    delay_ms(150);
    speaker_play(300);
    delay_ms(150);
    speaker_stop();
}

/* ---- Boot jingle (ascending notes) ---- */
void speaker_boot_sound(void) {
    uint32_t notes[] = { 523, 659, 784, 1047 }; /* C, E, G, C (major chord) */
    for (int i = 0; i < 4; i++) {
        speaker_play(notes[i]);
        delay_ms(120);
        speaker_stop();
        delay_ms(30);
    }
}
