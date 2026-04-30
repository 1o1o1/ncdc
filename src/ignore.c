/* ncdc - NCurses Direct Connect client

  Ignore list support: /ignore, /unignore, /ignorelist
  Patterns support wildcards: *bot*, Spam_*, etc.
  Modes: chat (public), pm (private), all (both)

*/

#include "ncdc.h"
#include "db.h"
#include "ui.h"

#if INTERFACE

#define IGNORE_CHAT 1
#define IGNORE_PM   2
#define IGNORE_ALL  (IGNORE_CHAT|IGNORE_PM)

#endif
// Ignore entry
typedef struct {
  char *pattern;    // raw pattern (may contain * ?)
  GPatternSpec *ps; // compiled pattern
  int mode;         // IGNORE_CHAT | IGNORE_PM
} ignore_entry_t;

#define IGNORE_CHAT 1
#define IGNORE_PM   2
#define IGNORE_ALL  (IGNORE_CHAT|IGNORE_PM)

// List of ignore_entry_t*
static GList *ignore_list = NULL;
static gboolean ignore_loaded = FALSE;

#define IGNORE_DB_KEY "ignore_list"

// Serialize list to "pattern:mode,pattern:mode,..."
static char *ignore_serialize(void) {
  GString *s = g_string_new(NULL);
  for(GList *l = ignore_list; l; l = l->next) {
    ignore_entry_t *e = l->data;
    if(s->len) g_string_append_c(s, ',');
    g_string_append_printf(s, "%s:%d", e->pattern, e->mode);
  }
  return g_string_free(s, FALSE);
}

static void ignore_entry_free(ignore_entry_t *e) {
  g_free(e->pattern);
  g_pattern_spec_free(e->ps);
  g_free(e);
}

static ignore_entry_t *ignore_entry_new(const char *pattern, int mode) {
  ignore_entry_t *e = g_new0(ignore_entry_t, 1);
  e->pattern = g_strdup(pattern);
  e->ps = g_pattern_spec_new(pattern);
  e->mode = mode;
  return e;
}

static void ignore_save(void) {
  char *s = ignore_serialize();
  if(s && *s)
    db_vars_set(0, IGNORE_DB_KEY, s);
  else
    db_vars_rm(0, IGNORE_DB_KEY);
  g_free(s);
}

static void ignore_load(void) {
  if(ignore_loaded) return;
  ignore_loaded = TRUE;

  char *raw = db_vars_get(0, IGNORE_DB_KEY);
  if(!raw) return;

  char **entries = g_strsplit(raw, ",", -1);
  for(int i = 0; entries[i]; i++) {
    char *colon = strrchr(entries[i], ':');
    if(!colon) continue;
    *colon = '\0';
    int mode = atoi(colon+1);
    if(mode < 1 || mode > IGNORE_ALL) continue;
    if(!entries[i][0]) continue;
    ignore_list = g_list_append(ignore_list, ignore_entry_new(entries[i], mode));
  }
  g_strfreev(entries);
}

// Returns ignore mode bits if nick matches, 0 otherwise
int ignore_check(const char *nick, int mode) {
  ignore_load();
  char *lnick = g_utf8_casefold(nick, -1);
  for(GList *l = ignore_list; l; l = l->next) {
    ignore_entry_t *e = l->data;
    char *lpat = g_utf8_casefold(e->pattern, -1);
    GPatternSpec *ps = g_pattern_spec_new(lpat);
    gboolean match = g_pattern_spec_match_string(ps, lnick);
    g_pattern_spec_free(ps);
    g_free(lpat);
    if(match && (e->mode & mode))
      { g_free(lnick); return e->mode & mode; }
  }
  g_free(lnick);
  return 0;
}

// Add or update ignore entry. Returns FALSE if already identical.
gboolean ignore_add(const char *pattern, int mode) {
  ignore_load();
  // Update if exists
  for(GList *l = ignore_list; l; l = l->next) {
    ignore_entry_t *e = l->data;
    if(g_ascii_strcasecmp(e->pattern, pattern) == 0) {
      if(e->mode == mode) return FALSE;
      e->mode = mode;
      ignore_save();
      return TRUE;
    }
  }
  ignore_list = g_list_append(ignore_list, ignore_entry_new(pattern, mode));
  ignore_save();
  return TRUE;
}

// Remove entry by pattern. Returns FALSE if not found.
gboolean ignore_remove(const char *pattern) {
  ignore_load();
  for(GList *l = ignore_list; l; l = l->next) {
    ignore_entry_t *e = l->data;
    if(g_ascii_strcasecmp(e->pattern, pattern) == 0) {
      ignore_list = g_list_delete_link(ignore_list, l);
      ignore_entry_free(e);
      ignore_save();
      return TRUE;
    }
  }
  return FALSE;
}

// Print full list to current tab
void ignore_list_print(void) {
  ui_tab_t *tab = ui_tab_cur ? ui_tab_cur->data : NULL;
  ignore_load();
  if(!ignore_list) {
    ui_m(tab, 0, "Ignore list is empty.");
    return;
  }
  ui_m(tab, 0, "Ignore list:");
  for(GList *l = ignore_list; l; l = l->next) {
    ignore_entry_t *e = l->data;
    const char *mode = e->mode == IGNORE_ALL  ? "all"  :
                       e->mode == IGNORE_PM   ? "pm"   : "chat";
    ui_mf(tab, 0, "  %s  (%s)", e->pattern, mode);
  }
}

// Parse mode string → bits. Returns -1 on error.
int ignore_parse_mode(const char *s) {
  if(!s || !*s || strcmp(s, "all")  == 0) return IGNORE_ALL;
  if(strcmp(s, "chat") == 0) return IGNORE_CHAT;
  if(strcmp(s, "pm")   == 0) return IGNORE_PM;
  return -1;
}
