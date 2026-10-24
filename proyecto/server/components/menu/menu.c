#include "menu.h"

#include <stdio.h>
#include <string.h>

#include "display.h"

void menu_init(menu_t *m)
{
    memset(m, 0, sizeof(*m));
    m->dirty = true;
}

bool menu_add(menu_t *m, const char *text)
{
    if (!text || m->count >= MENU_MAX_ROWS) return false;

    strncpy(m->items[m->count], text, MAX_SCREEN_CHARS - 1);
    m->items[m->count][MAX_SCREEN_CHARS - 1] = '\0';
    m->count++;
    m->dirty = true;
    return true;
}

void menu_move_up(menu_t *m)
{
    if (m->selected > 0) {
        m->selected--;
        m->dirty = true;
    }
}

void menu_move_down(menu_t *m)
{
    if (m->selected < m->count - 1) {
        m->selected++;
        m->dirty = true;
    }
}

const char *menu_selected(const menu_t *m)
{
    return m->count > 0 ? m->items[m->selected] : NULL;
}

bool menu_remove_selected(menu_t *m, char *out, size_t out_len)
{
    if (m->count == 0) return false;

    if (out && out_len > 0) {
        strncpy(out, m->items[m->selected], out_len - 1);
        out[out_len - 1] = '\0';
    }

    // Shift everything below the selected row up by one
    memmove(m->items[m->selected], m->items[m->selected + 1],
            (size_t)(m->count - m->selected - 1) * MAX_SCREEN_CHARS);
    m->count--;

    // If the last row was removed, move the cursor up with it
    if (m->selected >= m->count && m->selected > 0) {
        m->selected = m->count - 1;
    }
    m->dirty = true;
    return true;
}

void menu_invalidate(menu_t *m)
{
    m->dirty = true;
}

void menu_draw(menu_t *m)
{
    if (!m->dirty) return;

    display_clear();

    if (m->count == 0) {
        display_text(5, 0, " (no options)", false);
    }

    for (int i = 0; i < m->count; i++) {
        bool highlighted = (i == m->selected);

        char line[MAX_SCREEN_CHARS + 3];
        snprintf(line, sizeof(line), "%c%s", highlighted ? '>' : ' ', m->items[i]);

        display_text(5, i * 8, line, highlighted);
    }

    display_flush();
    m->dirty = false;
}