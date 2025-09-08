/* custom_slay.c - Customizable slay messages for SmaugFUSS 1.9.x / Arc Unleashed
 * Format: compatible with Taka's snippet: records with keys
 *   Snmbr/SName/PName/TVict/TChar/TRoom ... terminated with '$'
 * File: system/slay.txt (uses SYSTEM_DIR from mud.h)
 * This module lazy-loads on first use; no db.c hooks needed.
 */

#include <ctype.h>
#include "mud.h"
#include "custom_slay.h"

#define SLAY_FILE  SYSTEM_DIR "slay.txt"

CUSTOM_SLAY *first_cslay = NULL, *last_cslay = NULL;
static bool cslay_loaded = false;

/* ----- internals ----- */
static CUSTOM_SLAY *new_cslay( void )
{
   CUSTOM_SLAY *p;
   CREATE( p, CUSTOM_SLAY, 1 );
   p->number    = 0;
   p->name      = STRALLOC( "" );
   p->owner     = STRALLOC( "" );
   p->show_vict = STRALLOC( "" );
   p->show_char = STRALLOC( "" );
   p->show_room = STRALLOC( "" );
   return p;
}

static void free_cslay( CUSTOM_SLAY *p )
{
   if( !p ) return;
   if( p->name )      STRFREE( p->name );
   if( p->owner )     STRFREE( p->owner );
   if( p->show_vict ) STRFREE( p->show_vict );
   if( p->show_char ) STRFREE( p->show_char );
   if( p->show_room ) STRFREE( p->show_room );
   DISPOSE( p );
}

static void clear_all_cslays( void )
{
   CUSTOM_SLAY *p, *n;
   for( p = first_cslay; p; p = n )
   {
      n = p->next;
      UNLINK( p, first_cslay, last_cslay, next, prev );
      free_cslay( p );
   }
}

/* read a single record; return NULL at '$' */
static CUSTOM_SLAY *fread_one_cslay( FILE *fp )
{
   char *word = fread_word( fp );
   if( word[0] == '$' )
      return NULL;
   if( word[0] != '#' )
      ungetc( word[0], fp );

   CUSTOM_SLAY *p = new_cslay();

   while( !feof( fp ) )
   {
      long pos = ftell( fp );
      word = fread_word( fp );

      if( word[0] == '$' || word[0] == '#' )
      { fseek( fp, pos, SEEK_SET ); break; }

      if( !str_cmp( word, "Snmbr" ) ) { p->number    = fread_number( fp ); continue; }
      if( !str_cmp( word, "SName" ) ) { STRFREE(p->name);      p->name      = fread_string( fp ); continue; }
      if( !str_cmp( word, "PName" ) ) { STRFREE(p->owner);     p->owner     = fread_string( fp ); continue; }
      if( !str_cmp( word, "TVict" ) ) { STRFREE(p->show_vict); p->show_vict = fread_string( fp ); continue; }
      if( !str_cmp( word, "TChar" ) ) { STRFREE(p->show_char); p->show_char = fread_string( fp ); continue; }
      if( !str_cmp( word, "TRoom" ) ) { STRFREE(p->show_room); p->show_room = fread_string( fp ); continue; }

      fread_to_eol( fp ); /* be forgiving */
   }

   /* Normalize nulls */
   if( !p->name )      p->name = STRALLOC( "" );
   if( !p->owner )     p->owner = STRALLOC( "" );
   if( !p->show_vict ) p->show_vict = STRALLOC( "" );
   if( !p->show_char ) p->show_char = STRALLOC( "" );
   if( !p->show_room ) p->show_room = STRALLOC( "" );
   return p;
}

/* ----- API ----- */
void boot_custom_slays( void )
{
   FILE *fp;

   if( cslay_loaded )
      return;
   cslay_loaded = true;

   if( ( fp = fopen( SLAY_FILE, "r" ) ) == NULL )
   {
      bug( "boot_custom_slays: %s not found, creating.", SLAY_FILE );
      save_custom_slays(); /* create empty with '$' */
      return;
   }

   clear_all_cslays();

   while( !feof( fp ) )
   {
      long pos = ftell( fp );
      char *word = fread_word( fp );
      if( word[0] == '$' )
         break;
      if( word[0] != '#' )
         fseek( fp, pos, SEEK_SET );

      CUSTOM_SLAY *p = fread_one_cslay( fp );
      if( !p ) break;
      if( p->name && p->name[0] )
         LINK( p, first_cslay, last_cslay, next, prev );
      else
         free_cslay( p );
   }
   fclose( fp );
}

void save_custom_slays( void )
{
   FILE *fp;
   CUSTOM_SLAY *p;

   if( !cslay_loaded )
      boot_custom_slays();

   if( ( fp = fopen( SLAY_FILE, "w" ) ) == NULL )
   { bug( "save_custom_slays: fopen %s", SLAY_FILE ); perror( SLAY_FILE ); return; }

   for( p = first_cslay; p; p = p->next )
   {
      fprintf( fp, "#\n" );
      fprintf( fp, "Snmbr %d\n",  p->number );
      fprintf( fp, "SName %s~\n", p->name );
      fprintf( fp, "PName %s~\n", (p->owner ? p->owner : "") );
      fprintf( fp, "TVict %s~\n", (p->show_vict ? p->show_vict : "") );
      fprintf( fp, "TChar %s~\n", (p->show_char ? p->show_char : "") );
      fprintf( fp, "TRoom %s~\n", (p->show_room ? p->show_room : "") );
   }
   fprintf( fp, "$\n" );
   fclose( fp );
}

static bool cslay_accessible( CUSTOM_SLAY *p, CHAR_DATA *ch )
{
   if( !p ) return false;
   if( !p->owner || p->owner[0] == '\0' ) return true;   /* public */
   if( !str_cmp( p->owner, ch->name ) )  return true;    /* owner */
   return false;
}

void custom_slay_show_list( CHAR_DATA *ch )
{
   CUSTOM_SLAY *p; int col = 0;
   if( !cslay_loaded ) boot_custom_slays();
   if( !first_cslay )
      return;

   set_pager_color( AT_GREEN, ch );
   send_to_pager( "Custom slay types available to you:\n\r", ch );
   for( p = first_cslay; p; p = p->next )
   {
      if( !cslay_accessible( p, ch ) ) continue;
      pager_printf( ch, " %-18.18s", p->name && p->name[0] ? p->name : "(unnamed)" );
      if( ++col % 4 == 0 ) send_to_pager( "\n\r", ch );
   }
   if( col % 4 ) send_to_pager( "\n\r", ch );
}

static CUSTOM_SLAY *find_cslay( const char *name, CHAR_DATA *ch )
{
   CUSTOM_SLAY *p;
   if( !name || !*name ) return NULL;
   if( !cslay_loaded ) boot_custom_slays();

   for( p = first_cslay; p; p = p->next )
      if( !str_prefix( name, p->name ) && cslay_accessible( p, ch ) )
         return p;
   return NULL;
}

bool apply_custom_slay( CHAR_DATA *ch, CHAR_DATA *victim, const char *type )
{
   CUSTOM_SLAY *p = find_cslay( type, ch );
   if( !p ) return false;

   act( AT_IMMORT, (p->show_char && *p->show_char) ? p->show_char : "You brutally slay $N!", ch, NULL, victim, TO_CHAR );
   act( AT_IMMORT, (p->show_vict && *p->show_vict) ? p->show_vict : "$n brutally slays you!", ch, NULL, victim, TO_VICT );
   act( AT_IMMORT, (p->show_room && *p->show_room) ? p->show_room : "$n brutally slays $N!", ch, NULL, victim, TO_NOTVICT );

   set_cur_char( victim );
   raw_kill( ch, victim ); /* keep same kill path as base do_slay */
   return true;
}

/* ----------------- OLC-style editor command ----------------- */
/* Syntax:
 *   editslay <name> name new        (create)
 *   editslay <name> name <newname>  (rename)
 *   editslay <name> general 0|1     (public or owner-only)
 *   editslay <name> victim <text>
 *   editslay <name> char   <text>
 *   editslay <name> room   <text>
 *   editslay list
 */
void do_editslay( CHAR_DATA *ch, const char *argument )
{
   char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

   if( IS_NPC( ch ) )
   { send_to_char( "Mobs have no need for slay messages.\n\r", ch ); return; }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' )
   {
      set_char_color( AT_RED, ch );
      send_to_char( "Syntax: editslay <name> <field> <value>\n\r", ch );
      send_to_char( "Fields: name (new), general (0|1), victim, char, room\n\r", ch );
      send_to_char( "        editslay list\n\r", ch );
      return;
   }

   if( !cslay_loaded ) boot_custom_slays();

   if( !str_cmp( arg1, "list" ) )
   { custom_slay_show_list( ch ); return; }

   /* create new: editslay <name> name new */
   if( !str_cmp( arg2, "name" ) )
   {
      char arg3[MAX_INPUT_LENGTH]; one_argument( argument, arg3 );

      if( arg3[0] && !str_cmp( arg3, "new" ) )
      {
         CUSTOM_SLAY *p = new_cslay();
         STRFREE( p->name ); p->name = STRALLOC( arg1 );
         p->number = ( last_cslay ? last_cslay->number + 1 : 0 );
         LINK( p, first_cslay, last_cslay, next, prev );
         save_custom_slays();
         ch_printf( ch, "Created new slay '%s'.\n\r", p->name );
         return;
      }

      /* rename */
      if( arg3[0] )
      {
         CUSTOM_SLAY *p = find_cslay( arg1, ch );
         if( !p ) { ch_printf( ch, "No slay named '%s'.\n\r", arg1 ); return; }
         STRFREE( p->name ); p->name = STRALLOC( arg3 );
         save_custom_slays();
         ch_printf( ch, "Renamed to '%s'.\n\r", p->name );
         return;
      }

      send_to_char( "Usage: editslay <old> name <new>\n\r", ch );
      return;
   }

   /* general (public vs owner-only) */
   if( !str_cmp( arg2, "general" ) )
   {
      int v = is_number( argument ) ? atoi( argument ) : -1;
      if( v < 0 ) { send_to_char( "Usage: editslay <name> general 0|1\n\r", ch ); return; }
      CUSTOM_SLAY *p = find_cslay( arg1, ch );
      if( !p ) { ch_printf( ch, "No slay named '%s'.\n\r", arg1 ); return; }
      STRFREE( p->owner ); p->owner = STRALLOC( v == 1 ? ch->name : "" );
      save_custom_slays();
      ch_printf( ch, "Owner set to %s.\n\r", *p->owner ? p->owner : "public" );
      return;
   }

   /* victim/char/room text */
   if( !str_cmp( arg2, "victim" ) || !str_cmp( arg2, "char" ) || !str_cmp( arg2, "room" ) )
   {
      if( !*argument ) { send_to_char( "Provide the message text.\n\r", ch ); return; }
      CUSTOM_SLAY *p = find_cslay( arg1, ch );
      if( !p ) { ch_printf( ch, "No slay named '%s'.\n\r", arg1 ); return; }

      if( !str_cmp( arg2, "victim" ) ) { STRFREE( p->show_vict ); p->show_vict = STRALLOC( argument ); }
      else if( !str_cmp( arg2, "char" ) ) { STRFREE( p->show_char ); p->show_char = STRALLOC( argument ); }
      else { STRFREE( p->show_room ); p->show_room = STRALLOC( argument ); }
      save_custom_slays(); send_to_char( "Updated.\n\r", ch ); return;
   }

   send_to_char( "Unknown field. Use: name, general, victim, char, room, or 'editslay list'.\n\r", ch );
}
