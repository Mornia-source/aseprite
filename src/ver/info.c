/* Aseprite
   Copyright (C) 2020-2024  Igara Studio S.A.

   This program is distributed under the terms of
   the End-User License Agreement for Aseprite.  */

#include "ver/info.h"
#include "generated_version.h" /* It defines the VERSION macro */

#define PACKAGE   "Aseprite"
#define COPYRIGHT "Copyright (C) 2001-2025 Igara Studio S.A."

#if defined(_WIN32) || defined(__APPLE__)
  #define HTTP "https"
#else
  #define HTTP "http"
#endif

#ifdef CUSTOM_WEBSITE_URL
  #define WEBSITE CUSTOM_WEBSITE_URL /* To test web server */
#else
  #define WEBSITE HTTP "://www.aseprite.org/"
#endif
#define WEBSITE_DOWNLOAD     WEBSITE "download/"
#define WEBSITE_CONTRIBUTORS WEBSITE "contributors/"
#define WEBSITE_NEWS_RSS     HTTP "://blog.aseprite.org/rss"
#define WEBSITE_UPDATE       WEBSITE "update/?xml=1"

/* MODS: seam S22 -- this build checks for updates on our own server, but the
   download and contributors links must keep pointing at aseprite.org.
   CUSTOM_WEBSITE_URL cannot express that: it replaces WEBSITE itself, which
   every other link is built from. */
#ifdef MODS_UPDATE_URL
  #undef WEBSITE_UPDATE
  #define WEBSITE_UPDATE MODS_UPDATE_URL
#endif

const char* get_app_name()
{
  return PACKAGE;
}
const char* get_app_version()
{
  return VERSION;
}
const char* get_app_copyright()
{
  return COPYRIGHT;
}

const char* get_app_url()
{
  return WEBSITE;
}
const char* get_app_download_url()
{
  return WEBSITE_DOWNLOAD;
}
const char* get_app_contributors_url()
{
  return WEBSITE_CONTRIBUTORS;
}
const char* get_app_news_rss_url()
{
  return WEBSITE_NEWS_RSS;
}
const char* get_app_update_url()
{
  return WEBSITE_UPDATE;
}
