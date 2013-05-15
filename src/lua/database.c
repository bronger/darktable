/*
   This file is part of darktable,
   copyright (c) 2012 Jeremy Rosen

   darktable is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   darktable is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with darktable.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "lua/database.h"
#include "lua/image.h"
#include "lua/types.h"
#include "common/debug.h"
#include "common/darktable.h"
#include "common/image.h"
#include "common/film.h"

/***********************************************************************
  Creating the images global variable
 **********************************************************************/

int dt_lua_duplicate_image(lua_State *L)
{
  int imgid;
  luaA_to(L,dt_lua_image_t,&imgid,-1);
  imgid = dt_image_duplicate(imgid);
  luaA_push(L,dt_lua_image_t,&imgid);
  return 1;
}

static int import_images(lua_State *L)
{
  char* full_name= realpath(luaL_checkstring(L,-1), NULL);
  int result;

  if (g_file_test(full_name, G_FILE_TEST_IS_DIR))
  {
    result =dt_film_import_blocking(full_name);
    if(result == 0)
    {
      free(full_name);
      return luaL_error(L,"error while importing");
    }
  }
  else
  {
    dt_film_t new_film;
    dt_film_init(&new_film);
    char* dirname =g_path_get_dirname(full_name);
    result = dt_film_new(&new_film,dirname);
    if(result == 0)
    {
      free(full_name);
      dt_film_cleanup(&new_film);
      free(dirname);
      return luaL_error(L,"error while importing");
    }

    result =dt_image_import(new_film.id,full_name,TRUE);
    free(dirname);
    dt_film_cleanup(&new_film);
    if(result == 0)
    {
      free(full_name);
      return luaL_error(L,"error while importing");
    }
  }
  free(full_name);
  return 0;
}

static int database_len(lua_State*L)
{
  sqlite3_stmt *stmt = NULL;
  DT_DEBUG_SQLITE3_PREPARE_V2(dt_database_get(darktable.db),"select count(*) from images ", -1, &stmt, NULL);
  if(sqlite3_step(stmt) == SQLITE_ROW)
    lua_pushnumber(L,sqlite3_column_int(stmt, 0));
  else
    lua_pushnumber(L,0);
  sqlite3_finalize(stmt);
  return 1;
}

static int database_index(lua_State*L)
{
  int index = luaL_checkinteger(L,-1);
  sqlite3_stmt *stmt = NULL;
  char query[1024];
  sprintf(query,"select images.id from images order by images.id limit 1 offset %d",index -1);
  DT_DEBUG_SQLITE3_PREPARE_V2(dt_database_get(darktable.db),query, -1, &stmt, NULL);
  if(sqlite3_step(stmt) == SQLITE_ROW)
  {
    int imgid = sqlite3_column_int(stmt, 0);
    luaA_push(L,dt_lua_image_t,&imgid);
  }
  else
  {
    sqlite3_finalize(stmt);
    luaL_error(L,"incorrect index in database");
  }
  return 1;
}

int dt_lua_init_database(lua_State * L)
{

  /* database type */
  dt_lua_init_type(L,dt_lua_database_t);
  dt_lua_register_type_callback_number(L,dt_lua_database_t,database_index,NULL,database_len);
  lua_pushcfunction(L,dt_lua_duplicate_image);
  dt_lua_register_type_callback_stack(L,dt_lua_database_t,"duplicate");
  lua_pushcfunction(L,import_images);
  dt_lua_register_type_callback_stack(L,dt_lua_database_t,"import");

  /* darktable.images() */
  dt_lua_push_darktable_lib(L);
  void * tmp= NULL;
  luaA_push(L,dt_lua_database_t,&tmp);
  lua_setfield(L,-2,"database");
  return 0;
}

// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.sh
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-space on;
