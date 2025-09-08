/*
 * ArcU: Cybernetics (minimal, safe version)
 *
 * - Stores installed implants as PC bitflags (pcdata->cyber).
 * - "cyber" command lets players list/install/remove implants for gold.
 * - Score will display installed implants; REACTOR suppresses hungry/thirsty warnings.
 */
#include "mud.h"

typedef struct cyber_info CYBER_INFO;
struct cyber_info
{
   const char *name;    /* command keyword */
   int         flag;    /* CYBER_* bit */
   int         cost;    /* gold cost to install */
   const char *desc;    /* short description */
};

static const CYBER_INFO cyber_table[] =
{
   { "comm",     CYBER_COMM,      5000,  "Commlink (RP for now)" },
   { "eyes",     CYBER_EYES,     12000,  "Optics package" },
   { "legs",     CYBER_LEGS,     15000,  "Reinforced legs" },
   { "chest",    CYBER_CHEST,    18000,  "Plated chest" },
   { "reflexes", CYBER_REFLEXES, 20000,  "Reflex booster" },
   { "mind",     CYBER_MIND,     22000,  "Cognitive suite" },
   { "strength", CYBER_STRENGTH, 20000,  "Muscle weaves" },
   { "reactor",  CYBER_REACTOR,  25000,  "Micro-reactor (suppresses hungry/thirsty warnings)" },
   { "sterile",  CYBER_STERILE,   2500,  "Sterile body mods (RP)" },
   { NULL, 0, 0, NULL }
};

static const CYBER_INFO *lookup_cyber( const char *name )
{
   if( !name || name[0] == '\0' )
      return NULL;
   for( const CYBER_INFO *ci = cyber_table; ci->name; ++ci )
      if( !str_cmp( name, ci->name ) )
         return ci;
   return NULL;
}

/* Pretty list of installed flags; used by score too */
const char *cyber_bit_name( int flags )
{
   static char buf[256];
   buf[0] = '\0';

   if( flags == 0 )
      return "none";

   for( const CYBER_INFO *ci = cyber_table; ci->name; ++ci )
      if( flags & ci->flag )
      {
         if( buf[0] ) mudstrlcat( buf, " ", sizeof(buf) );
         mudstrlcat( buf, ci->name, sizeof(buf) );
      }
   return buf[0] ? buf : "unknown";
}

void do_cyber( CHAR_DATA *ch, const char *argument )
{
   char cmd[MAX_INPUT_LENGTH], what[MAX_INPUT_LENGTH];
   const char *rest = one_argument( argument, cmd ); //cmd = first word
   one_argument( rest, what ); // what = second

   if( IS_NPC(ch) )
   {
      send_to_char( "NPCs cannot use cybernetics.\r\n", ch );
      return;
   }

   if( cmd[0] == '\0' || !str_cmp(cmd, "list") )
   {
      ch_printf( ch, "&W%-12s %-8s %s\r\n", "Implant", "Cost", "Description" );
      for( const CYBER_INFO *ci = cyber_table; ci->name; ++ci )
      {
         bool have = (ch->pcdata->cyber & ci->flag) != 0;
         ch_printf( ch, " &c%-12s &Y%7d &w%s%s\r\n",
                    ci->name, ci->cost, ci->desc, have ? " &G[installed]" : "" );
      }
      send_to_char( "&wUse: &Wcyber install <name>&w or &Wcyber remove <name>&w.\r\n", ch );
      return;
   }

   if( !str_cmp(cmd, "installed") )
   {
      ch_printf( ch, "Installed cybernetics: &W%s&w.\r\n", cyber_bit_name( ch->pcdata->cyber ) );
      return;
   }

   if( !str_cmp(cmd, "install") )
   {
      if( what[0] == '\0' )
         { send_to_char( "Install which implant? Try: cyber list\r\n", ch ); return; }

      const CYBER_INFO *ci = lookup_cyber( what );
      if( !ci ) { send_to_char( "Unknown implant. Try: cyber list\r\n", ch ); return; }
      if( ch->pcdata->cyber & ci->flag )
         { ch_printf( ch, "You already have %s installed.\r\n", ci->name ); return; }
      if( ch->gold < ci->cost )
         { ch_printf( ch, "You need %d gold to install %s.\r\n", ci->cost, ci->name ); return; }

      ch->gold -= ci->cost;
      ch->pcdata->cyber |= ci->flag;

      act( AT_IMMORT, "Technicians install a $t.", ch, (void*)ci->name, NULL, TO_CHAR );
      act( AT_IMMORT, "$n now bears a new cybernetic upgrade.", ch, NULL, NULL, TO_ROOM );
      return;
   }

   if( !str_cmp(cmd, "remove") || !str_cmp(cmd, "uninstall") )
   {
      if( what[0] == '\0' )
         { send_to_char( "Remove which implant? Try: cyber installed\r\n", ch ); return; }

      const CYBER_INFO *ci = lookup_cyber( what );
      if( !ci ) { send_to_char( "Unknown implant name.\r\n", ch ); return; }
      if( !(ch->pcdata->cyber & ci->flag) )
         { ch_printf( ch, "You don't have %s installed.\r\n", ci->name ); return; }

      ch->pcdata->cyber &= ~ci->flag;
      act( AT_IMMORT, "Technicians remove your $t.", ch, (void*)ci->name, NULL, TO_CHAR );
      act( AT_IMMORT, "$n has a cybernetic implant removed.", ch, NULL, NULL, TO_ROOM );
      return;
   }

   send_to_char( "Syntax: cyber [list|installed|install <name>|remove <name>]\r\n", ch );
}
