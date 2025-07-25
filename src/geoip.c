/* ncdc - NCurses Direct Connect client

  Copyright (c) Yorhel

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/


#include "ncdc.h"
#include "geoip.h"

gboolean geoip_available = FALSE;
gboolean loc_available = FALSE;

#ifdef USE_GEOIP

#include <maxminddb.h>

static MMDB_s *db;

void geoip_reinit(void) {
  if(db) {
    MMDB_close(db);
    free(db);
    db = NULL;
    geoip_available = FALSE;
  }

  const char *fn = var_get(0, VAR_geoip_cc);
  if(!fn || strcmp(fn, "disabled") == 0)
    return;

  db = malloc(sizeof(MMDB_s));
  int status = MMDB_open(fn, 0, db);

  if(status != MMDB_SUCCESS) {
    ui_mf(NULL, 0, "Can't open '%s' (%s), no country codes will be displayed.", fn, MMDB_strerror(status));
    free(db);
    db = NULL;
  } else {
    geoip_available = TRUE;
  }
}

const char *geoip_country(const struct sockaddr *const ip) {
  if(!db)
    return NULL;

  int status;
  MMDB_lookup_result_s result = MMDB_lookup_sockaddr(db, ip, &status);
  if(status != MMDB_SUCCESS) {
    ui_mf(NULL, 0, "Error looking up IP address: %s", MMDB_strerror(status));
    return NULL;
  }
  if(!result.found_entry)
    return NULL;

  MMDB_entry_data_s data;
  status = MMDB_get_value(&result.entry, &data, "country", "iso_code", (void*)NULL);
  if(status != MMDB_SUCCESS) {
    ui_mf(NULL, 0, "Error looking up IP address: %s", MMDB_strerror(status));
    return NULL;
  }
  if(!data.has_data || data.type != MMDB_DATA_TYPE_UTF8_STRING)
    return NULL;

  static char buf[8];
  memset(buf, 0, sizeof(buf));
  memcpy(buf, data.utf8_string, data.data_size);
  return buf;
}


#else /* No GEOIP */

void geoip_reinit(void) {}
const char *geoip_country(const struct sockaddr *const ip) { return NULL; }

#endif



#ifdef USE_LIBLOC

#include <libloc/libloc.h>
#include <libloc/database.h>
#include <libloc/country.h>

static struct loc_database *locdb;

void loc_reinit(void) {
  if(locdb) loc_database_unref(locdb);
  locdb = NULL;
  loc_available = FALSE;

  const char *fn = var_get(0, VAR_location_db);
  if(!fn || strcmp(fn, "disabled") == 0)
    return;

  struct loc_ctx *ctx = NULL;
  if(loc_new(&ctx)) {
    ui_mf(NULL, 0, "Error initializing libloc (%s), no country codes will be displayed.", strerror(errno));
    return;
  }

  FILE *fh = fopen(fn, "r");
  if(!fh) {
    ui_mf(NULL, 0, "Error opening location database '%s' (%s), no country codes will be displayed.", fn, strerror(errno));
    loc_unref(ctx);
    return;
  }

  int r = loc_database_new(ctx, &locdb, fh);
  fclose(fh);
  loc_unref(ctx);
  if(r) ui_mf(NULL, 0, "Error reading location database '%s' (%s), no country codes will be displayed.", fn, strerror(errno));
  else loc_available = TRUE;
}

const char *loc_country(const struct in6_addr *ip) {
  if(!locdb) return NULL;

  struct loc_network *net = NULL;
  if(loc_database_lookup(locdb, ip, &net)) return NULL;

  static char buf[8], *ret = NULL;
  if(net) {
    const char *cc = loc_network_get_country_code(net);
    if(cc && *cc) strcpy(ret = buf, cc);
    loc_network_unref(net);
  }
  return ret;
}

#else /* No LIBLOC */

void loc_reinit(void) {};
const char *loc_country(const struct in6_addr *ip) { return NULL; };

#endif
