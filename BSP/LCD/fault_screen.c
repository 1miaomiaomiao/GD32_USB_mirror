#include <rtthread.h>
#include <rthw.h>
#include <string.h>
#include "lcd.h"
#include "fault_screen.h"

typedef struct
{
    rt_uint32_t r0, r1, r2, r3, r12, lr, pc, psr;
} exc_frame_t;

#define REG_CFSR   (*(volatile rt_uint32_t *)0xE000ED28UL)
#define REG_HFSR   (*(volatile rt_uint32_t *)0xE000ED2CUL)
#define REG_BFAR   (*(volatile rt_uint32_t *)0xE000ED38UL)

#define FG  0xFFFF
#define ERR 0xF800

static const rt_uint8_t font_rows(const char c, rt_uint8_t row)
{
    switch(c) {
    case '0': return "\007\005\005\005\007"[row];
    case '1': return "\002\006\002\002\007"[row];
    case '2': return "\007\001\007\004\007"[row];
    case '3': return "\007\001\003\001\007"[row];
    case '4': return "\005\005\007\001\001"[row];
    case '5': return "\007\004\007\001\007"[row];
    case '6': return "\007\004\007\005\007"[row];
    case '7': return "\007\001\001\002\002"[row];
    case '8': return "\007\005\007\005\007"[row];
    case '9': return "\007\005\007\001\007"[row];
    case 'A': return "\007\005\007\005\005"[row];
    case 'B': return "\006\005\006\005\006"[row];
    case 'C': return "\003\004\004\004\003"[row];
    case 'D': return "\006\005\005\005\006"[row];
    case 'E': return "\007\004\006\004\007"[row];
    case 'F': return "\007\004\006\004\004"[row];
    case 'H': return "\005\005\007\005\005"[row];
    case 'L': return "\004\004\004\004\007"[row];
    case 'M': return "\005\007\007\005\005"[row];
    case 'P': return "\006\005\006\004\004"[row];
    case 'R': return "\006\005\006\005\005"[row];
    case 'S': return "\003\004\002\001\006"[row];
    case 'T': return "\007\002\002\002\002"[row];
    case 'U': return "\005\005\005\005\007"[row];
    default:  return 0;
    }
}

static void draw_char(int x, int y, char c, int scale, rt_uint16_t color)
{
    int row, col, sx, sy;

    for(row=0; row < 5; row++) {
        rt_uint8_t bits=font_rows(c, row);
        for(col=0; col < 3; col++) {
            if(bits & (4 >> col)) {
                for(sy=0; sy < scale; sy++) {
                    for(sx=0; sx < scale; sx++) {
                        lcd_draw_point((rt_uint16_t)(x + col * scale + sx),
                                       (rt_uint16_t)(y + row * scale + sy),
                                       color);
                    }
                }
            }
        }
    }
}

static void draw_text(int x, int y, const char *s, int scale, rt_uint16_t color)
{
    while(*s) {
        draw_char(x, y, *s++, scale, color);
        x += 4 * scale;
    }
}

static void draw_hex32(int x, int y, rt_uint32_t v, int scale, rt_uint16_t color)
{
    char buf[11];
    int i;
    static const char hexd[]="0123456789ABCDEF";

    buf[0]='0'; buf[1]='X';
    for(i=0; i < 8; i++) {
        buf[2 + i]=hexd[(v >> (28 - 4 * i)) & 0xF];
    }
    buf[10]=0;
    draw_text(x, y, buf, scale, color);
}

static void draw_line(int x, int y, const char *label, rt_uint32_t v)
{
    draw_text(x, y, label, 3, FG);
    draw_hex32(x + (int)(strlen(label) + 1) * 12, y, v, 3, FG);
}

static rt_err_t fault_screen_hook(void *context)
{
    const exc_frame_t *f=(const exc_frame_t *)context;
    rt_thread_t self=rt_thread_self();
    const char *name=(self != RT_NULL) ? self->name : "QUESTION";
    int y=110;

    lcd_flip(LCD_FB_A_ADDR);
    lcd_clear(0x001F);

    draw_text(56, 24, "HARD FAULT", 8, ERR);

    draw_text(16, y, "THREAD", 3, FG);
    draw_text(16 + 7 * 12, y, name, 3, FG);
    if(f->psr & 0x1FF) {
        draw_text(400, y, "INISR", 3, ERR);
    }
    y += 20;

    draw_line(16, y, "PC",  f->pc);  y += 20;
    draw_line(16, y, "LR",  f->lr);  y += 20;
    draw_line(16, y, "PSR", f->psr); y += 20;
    draw_line(16, y, "CFSR", REG_CFSR); y += 20;
    draw_line(16, y, "HFSR", REG_HFSR); y += 20;
    draw_line(16, y, "BFAR", REG_BFAR);

    lcd_flush();

    return -RT_ERROR;
}

void fault_screen_init(void)
{
    rt_hw_exception_install(fault_screen_hook);
}
