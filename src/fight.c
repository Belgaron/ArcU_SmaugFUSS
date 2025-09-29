/****************************************************************************
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
 * -----------------------------------------------------------|   (0...0)   *
 * SMAUG 1.8 (C) 1994, 1995, 1996, 1998  by Derek Snider      |    ).:.(    *
 * -----------------------------------------------------------|    {o o}    *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,      |~'~.VxvxV.~'~*
 * Tricops, Fireblade, Edmond, Conran                         |             *
 * ------------------------------------------------------------------------ *
 * Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
 * Chastain, Michael Quan, and Mitchell Tse.                                *
 * Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
 * ------------------------------------------------------------------------ *
 *                            Battle & death module                         *
 ****************************************************************************/

#include <stdio.h>
#include "mud.h"
#include "custom_slay.h"

/* Map SMAUG AC to small DefenseRoll bonus: [-150..+100] -> [+20..-5] */
int armor_bonus_for_roll( CHAR_DATA *v )
{
   int ac = GET_AC( v );
   if( ac < -150 )
      ac = -150;
   if( ac > 100 )
      ac = 100;
   double t = ( ac + 150 ) / 250.0;
   int bonus = ( int )( 20 - 25 * t );
   return bonus;
}

/* Map SMAUG AC to mitigation %: [-150..+100] -> [50%..0%], hard cap 80 */
int armor_mitigation_percent( CHAR_DATA *v )
{
   int ac = GET_AC( v );
   if( ac < -150 )
      ac = -150;
   if( ac > 100 )
      ac = 100;
   double t = ( ac + 150 ) / 250.0;
   int pct = ( int )( 50 - 50 * t );
   if( pct < 0 )
      pct = 0;
   if( pct > 80 )
      pct = 80;
   return pct;
}

/* Bell-ish RNG: -19..+19 */
static inline int rng_bell( void )
{
   return number_range( 1, 20 ) + number_range( 1, 20 ) - 21;
}

/* Replace these with your real stat getters if different */
static inline int stat_STR( CHAR_DATA *ch )
{
   return ch->perm_str;
}
static inline int stat_DEX( CHAR_DATA *ch )
{
   return ch->perm_dex;
}
static inline int stat_CON( CHAR_DATA *ch )
{
   return ch->perm_con;
}
static inline int stat_INT( CHAR_DATA *ch )
{
   return ch->perm_int;
}
/* TEMP: if you don't have Focus yet, point to CHA or a temp */
static inline int stat_FOC( CHAR_DATA *ch )
{
   return ch->perm_spr;
}

/* Attack type tags */
#define ATK_MELEE  1
#define ATK_RANGED 2
#define ATK_KI     3

/* Cache skill numbers */
static int sn_unarmed = -1, sn_melee = -1, sn_ranged = -1, sn_kiblast = -1;
static int sn_dodge = -1, sn_parry = -1, sn_block = -1;

static void init_mos_skill_sns( void )
{
   if( sn_unarmed != -1 )
      return;
   sn_unarmed = skill_lookup( "unarmed" );
   sn_melee = skill_lookup( "melee" );
   sn_ranged = skill_lookup( "ranged" );
   sn_kiblast = skill_lookup( "ki blast" );
   sn_dodge = skill_lookup( "dodge" );
   sn_parry = skill_lookup( "parry" );
   sn_block = skill_lookup( "block" );
}

static int attack_sn_for( CHAR_DATA *ch, int atk_type )
{
   switch ( atk_type )
   {
      default:
      case ATK_MELEE:
         return ( get_eq_char( ch, WEAR_WIELD ) ? sn_melee : sn_unarmed );
      case ATK_RANGED:
         return sn_ranged;
      case ATK_KI:
         return sn_kiblast;
   }
}

int attack_roll( CHAR_DATA *att, int sn, int atk_type )
{
   init_mos_skill_sns( );

   double skill = get_skill( att, sn );
   int statAtk = 0;
   int weaponAcc = 0;

   if( atk_type == ATK_MELEE )
      statAtk = ( stat_STR( att ) + stat_DEX( att ) ) / 4;
   else if( atk_type == ATK_RANGED )
      statAtk = stat_DEX( att ) / 2;
   else
      statAtk = ( stat_FOC( att ) + stat_INT( att ) ) / 4;

   return ( int )( skill * 1.0 ) + statAtk + weaponAcc + rng_bell( );
}

avoid_t avoid_reason_from_stance( CHAR_DATA *victim, int atk_type )
{
   switch ( victim->active_defense )
   {
      case DEF_DODGE:
         return AVOID_DODGE;
      case DEF_PARRY:
         if( atk_type == ATK_RANGED || atk_type == ATK_KI )
            return AVOID_DEFLECT;
         return AVOID_PARRY;
      case DEF_BLOCK:
         return AVOID_BLOCK;
      default:
         return AVOID_NONE;
   }
}

int defense_roll( CHAR_DATA *def, int atk_type )
{
   init_mos_skill_sns( );
   int stance = def->active_defense;
   int statDef = 0, stanceMod = 0, dodgePenalty = 0;
   double defskill = 0.0;

   switch ( stance )
   {
      case DEF_DODGE:
      {
         defskill = get_skill( def, sn_dodge );
         statDef = stat_DEX( def ) / 2;
         OBJ_DATA *w;
         for( int wear = 0; wear < MAX_WEAR; ++wear )
            if( ( w = get_eq_char( def, wear ) ) != NULL )
               dodgePenalty += w->armor_penalty;
         break;
      }
      case DEF_PARRY:
         defskill = get_skill( def, sn_parry );
         statDef = ( stat_DEX( def ) + stat_STR( def ) ) / 4;
         if( atk_type == ATK_RANGED || atk_type == ATK_KI )
            stanceMod -= 5;
         break;

      case DEF_BLOCK:
         defskill = get_skill( def, sn_block );
         statDef = ( stat_STR( def ) + stat_CON( def ) ) / 4;
         if( atk_type == ATK_KI )
            stanceMod -= 3;
         break;

      default:
         defskill = 0.0;
         statDef = 0;
         break;
   }

   int armBonus = armor_bonus_for_roll( def );
   return ( int )( defskill * 1.0 ) + statDef + armBonus + stanceMod - dodgePenalty + rng_bell( );
}

hit_band_t classify_band( int MoS )
{
   if( MoS <= 0 )
      return HIT_AVOIDED;
   if( MoS <= 10 )
      return HIT_GLANCING;
   if( MoS <= 25 )
      return HIT_SOLID;
   return HIT_CLEAN;
}

int compute_final_mitigation_percent( CHAR_DATA *victim, int MoS, hit_band_t band, int atk_type )
{
   int mit = armor_mitigation_percent( victim );
   mit -= MoS;
   if( ( band == HIT_GLANCING || band == HIT_SOLID ) && victim->active_defense == DEF_BLOCK )
      mit += 10;
   if( mit < 0 )
      mit = 0;
   if( mit > 80 )
      mit = 80;
   return mit;
}

extern char lastplayercmd[MAX_INPUT_LENGTH];

OBJ_DATA *used_weapon;  /* Used to figure out which weapon later */

/*
 * Local functions.
 */
void new_dam_message( CHAR_DATA * ch, CHAR_DATA * victim, int dam, unsigned int dt, OBJ_DATA * obj, long long pl_gained );
int xp_compute( CHAR_DATA * gch, CHAR_DATA * victim );
int align_compute( CHAR_DATA * gch, CHAR_DATA * victim );
ch_ret one_hit( CHAR_DATA * ch, CHAR_DATA * victim, int dt );
int obj_hitroll( OBJ_DATA * obj );
void show_condition( CHAR_DATA * ch, CHAR_DATA * victim );

bool loot_coins_from_corpse( CHAR_DATA * ch, OBJ_DATA * corpse )
{
   OBJ_DATA *content, *content_next;
   int oldgold = ch->gold;

   if( !corpse )
   {
      bug( "%s: NULL corpse!", __func__ );
      return FALSE;
   }

   for( content = corpse->first_content; content; content = content_next )
   {
      content_next = content->next_content;

      if( content->item_type != ITEM_MONEY )
         continue;
      if( !can_see_obj( ch, content ) )
         continue;
      if( !CAN_WEAR( content, ITEM_TAKE ) && ch->level < sysdata.level_getobjnotake )
         continue;
      if( IS_OBJ_STAT( content, ITEM_PROTOTYPE ) && !can_take_proto( ch ) )
         continue;

      act( AT_ACTION, "You get $p from $P", ch, content, corpse, TO_CHAR );
      act( AT_ACTION, "$n gets $p from $P", ch, content, corpse, TO_ROOM );
      obj_from_obj( content );
      check_for_trap( ch, content, TRAP_GET );
      if( char_died( ch ) )
         return FALSE;

      oprog_get_trigger( ch, content );
      if( char_died( ch ) )
         return FALSE;

      ch->gold += content->value[0] * content->count;
      extract_obj( content );
   }

   if( ch->gold - oldgold > 1 && ch->position > POS_SLEEPING )
   {
      char buf[MAX_INPUT_LENGTH];

      snprintf( buf, MAX_INPUT_LENGTH, "%d", ch->gold - oldgold );
      do_split( ch, buf );
   }
   return TRUE;
}

/*
 * Generate focus during combat rounds
 */
void generate_focus( CHAR_DATA *ch )
{
    int base_focus, penalty = 0;
    
    if( IS_NPC(ch) || !who_fighting(ch) )
        return;
    
    // Base focus generation: 1 to (intelligence/5)
    base_focus = number_range( 1, UMAX(1, get_curr_int(ch)) / 5 );
    
    // Apply defensive skill penalties (like DBSC)
    if( ch->pcdata->skills[gsn_dodge].value_tenths > 0 )
        penalty += 5;
    if( ch->pcdata->skills[gsn_parry].value_tenths > 0 )  // Using parry instead of block
        penalty += 5;
    // Add other defensive skills as needed
    
    // Reduce focus generation by penalty percentage
    if( penalty > 0 )
    {
        base_focus = base_focus - ((base_focus * penalty) / 100);
    }
    
    if( base_focus < 0 )
        base_focus = 0;
        
    ch->focus += base_focus;
    ch->focus = URANGE( 0, ch->focus, get_curr_int(ch) );
}

/*
 * Reset focus (called when combat starts, position changes, etc.)
 */
void reset_focus( CHAR_DATA *ch )
{
    if( !ch )
        return;
        
    ch->focus = 0;
}

/*
 * Decay focus when not in combat
 */
void decay_focus( CHAR_DATA *ch )
{
    int decay_amount;
    
    if( IS_NPC(ch) || who_fighting(ch) || ch->focus <= 0 )
        return;
        
    decay_amount = number_range( 1, 3 );
    ch->focus = UMAX( 0, ch->focus - decay_amount );
}

/*
 * Check to see if player's attacks are (still?) suppressed
 * #ifdef TRI
 */
bool is_attack_supressed( CHAR_DATA * ch )
{
   TIMER *timer;

   if( IS_NPC( ch ) )
      return FALSE;

   if( IS_AFFECTED( ch, AFF_GRAPPLE ) )
      return TRUE;

   timer = get_timerptr( ch, TIMER_ASUPRESSED );

   if( !timer )
      return FALSE;

   /*
    * perma-supression -- bard? (can be reset at end of fight, or spell, etc) 
    */
   if( timer->value == -1 )
      return TRUE;

   /*
    * this is for timed supressions 
    */
   if( timer->count >= 1 )
      return TRUE;

   return FALSE;
}

/*
 * Check to see if weapon is poisoned.
 */
bool is_wielding_poisoned( CHAR_DATA * ch )
{
   OBJ_DATA *obj;

   if( !used_weapon )
      return FALSE;

   if( ( obj = get_eq_char( ch, WEAR_WIELD ) ) != NULL && used_weapon == obj && IS_OBJ_STAT( obj, ITEM_POISONED ) )
      return TRUE;
   if( ( obj = get_eq_char( ch, WEAR_DUAL_WIELD ) ) != NULL && used_weapon == obj && IS_OBJ_STAT( obj, ITEM_POISONED ) )
      return TRUE;

   return FALSE;
}

/*
 * hunting, hating and fearing code				-Thoric
 */
bool is_hunting( CHAR_DATA * ch, CHAR_DATA * victim )
{
   if( !ch->hunting || ch->hunting->who != victim )
      return FALSE;

   return TRUE;
}

bool is_hating( CHAR_DATA * ch, CHAR_DATA * victim )
{
   if( !ch->hating || ch->hating->who != victim )
      return FALSE;

   return TRUE;
}

bool is_fearing( CHAR_DATA * ch, CHAR_DATA * victim )
{
   if( !ch->fearing || ch->fearing->who != victim )
      return FALSE;

   return TRUE;
}

void stop_hunting( CHAR_DATA * ch )
{
   if( ch->hunting )
   {
      STRFREE( ch->hunting->name );
      DISPOSE( ch->hunting );
      ch->hunting = NULL;
   }
}

void stop_hating( CHAR_DATA * ch )
{
   if( ch->hating )
   {
      STRFREE( ch->hating->name );
      DISPOSE( ch->hating );
      ch->hating = NULL;
   }
}

void stop_fearing( CHAR_DATA * ch )
{
   if( ch->fearing )
   {
      STRFREE( ch->fearing->name );
      DISPOSE( ch->fearing );
      ch->fearing = NULL;
   }
}

void start_hunting( CHAR_DATA * ch, CHAR_DATA * victim )
{
   if( ch->hunting )
      stop_hunting( ch );

   CREATE( ch->hunting, HHF_DATA, 1 );
   ch->hunting->name = QUICKLINK( victim->name );
   ch->hunting->who = victim;
}

void start_hating( CHAR_DATA * ch, CHAR_DATA * victim )
{
   if( ch->hating )
      stop_hating( ch );

   CREATE( ch->hating, HHF_DATA, 1 );
   ch->hating->name = QUICKLINK( victim->name );
   ch->hating->who = victim;
}

void start_fearing( CHAR_DATA * ch, CHAR_DATA * victim )
{
   if( ch->fearing )
      stop_fearing( ch );

   CREATE( ch->fearing, HHF_DATA, 1 );
   ch->fearing->name = QUICKLINK( victim->name );
   ch->fearing->who = victim;
}

void strip_grapple( CHAR_DATA * ch )
{
   if( IS_NPC( ch ) )
      return;

   if( IS_AFFECTED( ch, AFF_GRAPPLE ) )
   {
      affect_strip( ch, gsn_grapple );
      set_char_color( AT_WEAROFF, ch );
      send_to_char( skill_table[gsn_grapple]->msg_off, ch );
      send_to_char( "\r\n", ch );
      if( get_timer( ch, TIMER_PKILLED ) > 0 )
         remove_timer( ch, TIMER_ASUPRESSED );
   }

   if( who_fighting( ch ) )
   {
      if( who_fighting( ch->fighting->who ) == ch && IS_AFFECTED( who_fighting( ch ), AFF_GRAPPLE ) )
      {
         affect_strip( who_fighting( ch ), gsn_grapple );
         set_char_color( AT_WEAROFF, who_fighting( ch ) );
         send_to_char( skill_table[gsn_grapple]->msg_off, who_fighting( ch ) );
         send_to_char( "\r\n", who_fighting( ch ) );
         if( get_timer( who_fighting( ch ), TIMER_PKILLED ) > 0 )
            remove_timer( who_fighting( ch ), TIMER_ASUPRESSED );
      }
   }
}

/*
 * Get the current armor class for a vampire based on time of day
 */
short VAMP_AC( CHAR_DATA * ch )
{
   if( IS_VAMPIRE( ch ) && IS_OUTSIDE( ch ) )
   {
      switch ( time_info.sunlight )
      {
         case SUN_DARK:
            return -10;
         case SUN_RISE:
            return 5;
         case SUN_LIGHT:
            return 10;
         case SUN_SET:
            return 2;
         default:
            return 0;
      }
   }
   return 0;
}

/*
 * Control the fights going on.
 * Called periodically by update_handler.
 * Many hours spent fixing bugs in here by Thoric, as noted by residual
 * debugging checks.  If you never get any of these error messages again
 * in your logs... then you can comment out some of the checks without
 * worry.
 *
 * Note:  This function also handles some non-violence updates.
 */
void violence_update( void )
{
   CHAR_DATA *ch;
   CHAR_DATA *victim;
   CHAR_DATA *rch;
   TRV_WORLD *lcw;
   TRV_DATA *lcr;
   AFFECT_DATA *paf, *paf_next;
   TIMER *timer, *timer_next;
   ch_ret retcode;
   int attacktype, cnt;
   SKILLTYPE *skill;
   static int pulse = 0;

   pulse = ( pulse + 1 ) % 100;

   lcw = trworld_create( TR_CHAR_WORLD_BACK );
   for( ch = last_char; ch; ch = trvch_wnext( lcw ) )
   {
      set_cur_char( ch );

      /*
       * See if we got a pointer to someone who recently died...
       * if so, either the pointer is bad... or it's a player who
       * "died", and is back at the healer...
       * Since he/she's in the char_list, it's likely to be the later...
       * and should not already be in another fight already
       */
      if( char_died( ch ) )
         continue;
	 
	  /* Handle focus generation/decay based on combat status */
      if( !IS_NPC(ch) )
      {
          if( who_fighting(ch) )
          {
              /* In combat - use the proper generate_focus function */
              generate_focus(ch);
          }
          else
          {
              /* Not in combat - decay focus */
              decay_focus(ch);
          }
      }

      
       

      for( timer = ch->first_timer; timer; timer = timer_next )
      {
         timer_next = timer->next;

         if( ch->fighting && timer->type == TIMER_DO_FUN )
         {
            int tempsub;

            tempsub = ch->substate;
            ch->substate = SUB_TIMER_DO_ABORT;
            ( timer->do_fun ) ( ch, "" );
            if( char_died( ch ) )
               break;
            ch->substate = tempsub;
            extract_timer( ch, timer );
            continue;
         }

         if( --timer->count <= 0 )
         {
            if( timer->type == TIMER_ASUPRESSED )
            {
               if( timer->value == -1 )
               {
                  timer->count = 1000;
                  continue;
               }
            }

            if( timer->type == TIMER_NUISANCE )
               DISPOSE( ch->pcdata->nuisance );

            if( timer->type == TIMER_DO_FUN )
            {
               int tempsub;

               tempsub = ch->substate;
               ch->substate = timer->value;
               ( timer->do_fun ) ( ch, "" );
               if( char_died( ch ) )
                  break;
               ch->substate = tempsub;
               if( timer->count > 0 )
                  continue;
            }
            extract_timer( ch, timer );
         }
      }
      
	  
      if( char_died( ch ) )
         continue;

      /*
       * We need spells that have shorter durations than an hour.
       * So a melee round sounds good to me... -Thoric
       */
      for( paf = ch->first_affect; paf; paf = paf_next )
      {
         paf_next = paf->next;
         if( paf->duration > 0 )
            paf->duration--;
         else if( paf->duration < 0 )
            ;
         else
         {
            if( !paf_next || paf_next->type != paf->type || paf_next->duration > 0 )
            {
               skill = get_skilltype( paf->type );
               if( paf->type > 0 && skill && skill->msg_off )
               {
                  set_char_color( AT_WEAROFF, ch );
                  send_to_char( skill->msg_off, ch );
                  send_to_char( "\r\n", ch );
               }
            }
            if( paf->type == gsn_possess )
            {
               ch->desc->character = ch->desc->original;
               ch->desc->original = NULL;
               ch->desc->character->desc = ch->desc;
               ch->desc->character->switched = NULL;
               ch->desc = NULL;
            }
            affect_remove( ch, paf );
         }
      }

      if( char_died( ch ) )
         continue;

      /*
       * check for exits moving players around 
       */
      if( ( retcode = pullcheck( ch, pulse ) ) == rCHAR_DIED || char_died( ch ) )
         continue;

      /*
       * Let the battle begin! 
       */
      if( ( victim = who_fighting( ch ) ) == NULL || IS_AFFECTED( ch, AFF_PARALYSIS ) )
         continue;

      retcode = rNONE;

      if( xIS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
      {
         log_printf( "violence_update: %s fighting %s in a SAFE room.", ch->name, victim->name );
         stop_fighting( ch, TRUE );
      }
      else if( IS_AWAKE( ch ) && ch->in_room == victim->in_room )
         retcode = multi_hit( ch, victim, TYPE_UNDEFINED );
      else
         stop_fighting( ch, FALSE );

      if( char_died( ch ) )
         continue;

      if( retcode == rCHAR_DIED || ( victim = who_fighting( ch ) ) == NULL )
         continue;

      /*
       *  Mob triggers
       *  -- Added some victim death checks, because it IS possible.. -- Alty
       */
      rprog_rfight_trigger( ch );
      if( char_died( ch ) || char_died( victim ) )
         continue;
      mprog_hitprcnt_trigger( ch, victim );
      if( char_died( ch ) || char_died( victim ) )
         continue;
      mprog_fight_trigger( ch, victim );
      if( char_died( ch ) || char_died( victim ) )
         continue;

      /*
       * NPC special attack flags            -Thoric
       */
      if( IS_NPC( ch ) )
      {
         if( !xIS_EMPTY( ch->attacks ) )
         {
            attacktype = -1;
            if( 30 + ( ch->level / 4 ) >= number_percent(  ) )
            {
               cnt = 0;
               for( ;; )
               {
                  if( cnt++ > 10 )
                  {
                     attacktype = -1;
                     break;
                  }
                  attacktype = number_range( 7, MAX_ATTACK_TYPE - 1 );
                  if( xIS_SET( ch->attacks, attacktype ) )
                     break;
               }
               switch ( attacktype )
               {
                  case ATCK_BASH:
                     do_bash( ch, "" );
                     retcode = global_retcode;
                     break;
                  case ATCK_STUN:
                     do_stun( ch, "" );
                     retcode = global_retcode;
                     break;
                  case ATCK_GOUGE:
                     do_gouge( ch, "" );
                     retcode = global_retcode;
                     break;
                  case ATCK_FEED:
                     do_feed( ch, "" );
                     retcode = global_retcode;
                     break;
                  case ATCK_DRAIN:
                     retcode = spell_energy_drain( skill_lookup( "energy drain" ), ch->level, ch, victim );
                     break;
                  case ATCK_FIREBREATH:
                     retcode = spell_fire_breath( skill_lookup( "fire breath" ), ch->level, ch, victim );
                     break;
                  case ATCK_FROSTBREATH:
                     retcode = spell_frost_breath( skill_lookup( "frost breath" ), ch->level, ch, victim );
                     break;
                  case ATCK_ACIDBREATH:
                     retcode = spell_acid_breath( skill_lookup( "acid breath" ), ch->level, ch, victim );
                     break;
                  case ATCK_LIGHTNBREATH:
                     retcode = spell_lightning_breath( skill_lookup( "lightning breath" ), ch->level, ch, victim );
                     break;
                  case ATCK_GASBREATH:
                     retcode = spell_gas_breath( skill_lookup( "gas breath" ), ch->level, ch, victim );
                     break;
                  case ATCK_SPIRALBLAST:
                     retcode = spell_spiral_blast( skill_lookup( "spiral blast" ), ch->level, ch, victim );
                     break;
                  case ATCK_POISON:
                     retcode = spell_poison( gsn_poison, ch->level, ch, victim );
                     break;
                  case ATCK_NASTYPOISON:
                     /*
                      * retcode = spell_nasty_poison( skill_lookup( "nasty poison" ), ch->level, ch, victim );
                      */
                     break;
                  case ATCK_GAZE:
                     /*
                      * retcode = spell_gaze( skill_lookup( "gaze" ), ch->level, ch, victim );
                      */
                     break;
                  case ATCK_BLINDNESS:
                     retcode = spell_blindness( gsn_blindness, ch->level, ch, victim );
                     break;
                  case ATCK_CAUSESERIOUS:
                     retcode = spell_cause_serious( skill_lookup( "cause serious" ), ch->level, ch, victim );
                     break;
                  case ATCK_EARTHQUAKE:
                     retcode = spell_earthquake( skill_lookup( "earthquake" ), ch->level, ch, victim );
                     break;
                  case ATCK_CAUSECRITICAL:
                     retcode = spell_cause_critical( skill_lookup( "cause critical" ), ch->level, ch, victim );
                     break;
                  case ATCK_CURSE:
                     retcode = spell_curse( skill_lookup( "curse" ), ch->level, ch, victim );
                     break;
                  case ATCK_FLAMESTRIKE:
                     retcode = spell_flamestrike( skill_lookup( "flamestrike" ), ch->level, ch, victim );
                     break;
                  case ATCK_HARM:
                     retcode = spell_harm( skill_lookup( "harm" ), ch->level, ch, victim );
                     break;
                  case ATCK_FIREBALL:
                     retcode = spell_fireball( skill_lookup( "fireball" ), ch->level, ch, victim );
                     break;
                  case ATCK_COLORSPRAY:
                     retcode = spell_colour_spray( skill_lookup( "colour spray" ), ch->level, ch, victim );
                     break;
                  case ATCK_WEAKEN:
                     retcode = spell_weaken( skill_lookup( "weaken" ), ch->level, ch, victim );
                     break;
               }
               if( attacktype != -1 && ( retcode == rCHAR_DIED || char_died( ch ) ) )
                  continue;
            }
         }

         /*
          * NPC special defense flags          -Thoric
          */
         if( !xIS_EMPTY( ch->defenses ) )
         {
            /*
             * Fix for character not here bugs --Shaddai 
             */
            if( char_died( ch ) || char_died( victim ) )
               continue;

            attacktype = -1;
            if( 50 + ( ch->level / 4 ) > number_percent(  ) )
            {
               cnt = 0;
               for( ;; )
               {
                  if( cnt++ > 10 )
                  {
                     attacktype = -1;
                     break;
                  }
                  attacktype = number_range( 2, MAX_DEFENSE_TYPE - 1 );
                  if( xIS_SET( ch->defenses, attacktype ) )
                     break;
               }

               switch ( attacktype )
               {
                  case DFND_CURELIGHT:
                     /*
                      * A few quick checks in the cure ones so that a) less spam and
                      * b) we don't have mobs looking stupider than normal by healing
                      * themselves when they aren't even being hit (although that
                      * doesn't happen TOO often 
                      */
                     if( ch->hit < ch->max_hit )
                     {
                        act( AT_MAGIC, "$n mutters a few incantations...and looks a little better.", ch, NULL, NULL,
                             TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "cure light" ), ch->level, ch, ch );
                     }
                     break;

                  case DFND_CURESERIOUS:
                     if( ch->hit < ch->max_hit )
                     {
                        act( AT_MAGIC, "$n mutters a few incantations...and looks a bit better.", ch, NULL, NULL, TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "cure serious" ), ch->level, ch, ch );
                     }
                     break;

                  case DFND_CURECRITICAL:
                     if( ch->hit < ch->max_hit )
                     {
                        act( AT_MAGIC, "$n mutters a few incantations...and looks healthier.", ch, NULL, NULL, TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "cure critical" ), ch->level, ch, ch );
                     }
                     break;

                  case DFND_HEAL:
                     if( ch->hit < ch->max_hit )
                     {
                        act( AT_MAGIC, "$n mutters a few incantations...and looks much healthier.", ch, NULL, NULL,
                             TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "heal" ), ch->level, ch, ch );
                     }
                     break;

                  case DFND_DISPELMAGIC:
                     if( victim->first_affect )
                     {
                        act( AT_MAGIC, "$n utters an incantation...", ch, NULL, NULL, TO_ROOM );
                        retcode = spell_dispel_magic( skill_lookup( "dispel magic" ), ch->level, ch, victim );
                     }
                     break;

                  case DFND_DISPELEVIL:
                     act( AT_MAGIC, "$n utters an incantation...", ch, NULL, NULL, TO_ROOM );
                     retcode = spell_dispel_evil( skill_lookup( "dispel evil" ), ch->level, ch, victim );
                     break;

                  case DFND_TELEPORT:
                     retcode = spell_teleport( skill_lookup( "teleport" ), ch->level, ch, ch );
                     break;

                  case DFND_SHOCKSHIELD:
                     if( !IS_AFFECTED( ch, AFF_SHOCKSHIELD ) )
                     {
                        act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL, TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "shockshield" ), ch->level, ch, ch );
                     }
                     else
                        retcode = rNONE;
                     break;

                  case DFND_VENOMSHIELD:
                     if( !IS_AFFECTED( ch, AFF_VENOMSHIELD ) )
                     {
                        act( AT_MAGIC, "$n utters a few incantations ...", ch, NULL, NULL, TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "venomshield" ), ch->level, ch, ch );
                     }
                     else
                        retcode = rNONE;
                     break;

                  case DFND_ACIDMIST:
                     if( !IS_AFFECTED( ch, AFF_ACIDMIST ) )
                     {
                        act( AT_MAGIC, "$n utters a few incantations ...", ch, NULL, NULL, TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "acidmist" ), ch->level, ch, ch );
                     }
                     else
                        retcode = rNONE;
                     break;

                  case DFND_FIRESHIELD:
                     if( !IS_AFFECTED( ch, AFF_FIRESHIELD ) )
                     {
                        act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL, TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "fireshield" ), ch->level, ch, ch );
                     }
                     else
                        retcode = rNONE;
                     break;

                  case DFND_ICESHIELD:
                     if( !IS_AFFECTED( ch, AFF_ICESHIELD ) )
                     {
                        act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL, TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "iceshield" ), ch->level, ch, ch );
                     }
                     else
                        retcode = rNONE;
                     break;

                  case DFND_TRUESIGHT:
                     if( !IS_AFFECTED( ch, AFF_TRUESIGHT ) )
                        retcode = spell_smaug( skill_lookup( "true" ), ch->level, ch, ch );
                     else
                        retcode = rNONE;
                     break;

                  case DFND_SANCTUARY:
                     if( !IS_AFFECTED( ch, AFF_SANCTUARY ) )
                     {
                        act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL, TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "sanctuary" ), ch->level, ch, ch );
                     }
                     else
                        retcode = rNONE;
                     break;
               }
               if( attacktype != -1 && ( retcode == rCHAR_DIED || char_died( ch ) ) )
                  continue;
            }
         }
      }

      /*
       * Fun for the whole family!
       */
      lcr = trvch_create( ch, TR_CHAR_ROOM_FORW );
      for( rch = ch->in_room->first_person; rch; rch = trvch_next( lcr ) )
      {
         /*
          *   Group Fighting Styles Support:
          *   If ch is tanking
          *   If rch is using a more aggressive style than ch
          *   Then rch is the new tank   -h
          */
         if( ( !IS_NPC( ch ) && !IS_NPC( rch ) )
             && ( rch != ch )
             && ( rch->fighting )
             && ( who_fighting( rch->fighting->who ) == ch )
             && ( !xIS_SET( rch->fighting->who->act, ACT_AUTONOMOUS ) ) && ( rch->style < ch->style ) )
         {
             stop_fighting( rch->fighting->who, FALSE );
             set_fighting( rch->fighting->who, rch ); 
         }

         if( IS_AWAKE( rch ) && !rch->fighting )
         {
            /*
             * PC's auto-assist others in their group.
             */
            if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
            {
               if( ( ( !IS_NPC( rch ) && rch->desc )
                     || IS_AFFECTED( rch, AFF_CHARM ) ) && is_same_group( ch, rch ) && !is_safe( rch, victim, TRUE ) )
                  multi_hit( rch, victim, TYPE_UNDEFINED );
               continue;
            }

            /*
             * NPC's assist NPC's of same type or 12.5% chance regardless.
             */
            if( IS_NPC( rch ) && !IS_AFFECTED( rch, AFF_CHARM ) && !xIS_SET( rch->act, ACT_NOASSIST )
                && !xIS_SET( rch->act, ACT_PET ) )
            {
               if( char_died( ch ) )
                  break;
               if( rch->pIndexData == ch->pIndexData || number_bits( 3 ) == 0 )
               {
                  CHAR_DATA *vch;
                  CHAR_DATA *target;
                  int number;

                  target = NULL;
                  number = 0;
                  for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
                  {
                     if( can_see( rch, vch ) && is_same_group( vch, victim ) && number_range( 0, number ) == 0 )
                     {
                        if( vch->mount && vch->mount == rch )
                           target = NULL;
                        else
                        {
                           target = vch;
                           number++;
                        }
                     }
                  }

                  if( target )
                     multi_hit( rch, target, TYPE_UNDEFINED );
               }
            }
         }
      }
      trv_dispose( &lcr );
   }
   trworld_dispose( &lcw );
}

/* Group experience distribution (DBSC style)         -Dragus */ 
void group_gain( CHAR_DATA *ch, CHAR_DATA *victim )
{
   CHAR_DATA *gch;
   CHAR_DATA *lch;  
   int members = 0;
   int base_exp;
   
   if( !ch || !victim )
      return;
      
   /* Calculate base experience from victim */
   base_exp = get_exp_worth( victim );
   
   /* Find group leader */
   lch = (ch->leader != NULL) ? ch->leader : ch;
   
   /* Count group members in same room */
   for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
   {
      if( is_same_group( gch, lch ) )
         members++;
   }
   
   if( members == 0 )
   {
      bug( "Group_gain: members = 0." );
      members = 1;
   }
   
   /* Distribute experience to group members */
   for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
   {
      int group_exp;
      
      if( !is_same_group( gch, ch ) || IS_NPC( gch ) )
         continue;
         
      /* Calculate individual share */
      group_exp = base_exp / members;
      
      /* DBSC: Power level difference modifier */
      long long ch_pl = get_power_level( gch );
      long long victim_pl = get_power_level( victim );
      
      if( victim_pl > 0 )
      {
         if( ch_pl > victim_pl * 2 )
            group_exp /= 4;  /* Much stronger = less gain */
         else if( ch_pl > victim_pl )
            group_exp = group_exp * 3 / 4;  /* Stronger = reduced gain */
         else if( victim_pl > ch_pl * 2 )
            group_exp = group_exp * 3 / 2;  /* Much weaker = bonus */
      }
   }
}

/*
 * Do one group of attacks.
 */
ch_ret multi_hit( CHAR_DATA * ch, CHAR_DATA * victim, int dt )
{
   int schance;
   int dual_bonus;
   ch_ret retcode;
	
	/* Enhanced safety checks */
   if( !ch )
   {
      bug( "%s: null ch!", __func__ );
      return rERROR;
   }
   
   if( !victim )
   {
      bug( "%s: null victim!", __func__ );
      return rERROR;
   }
   
   if( ch->in_room != victim->in_room )
   {
      bug( "%s: ch (%s) and victim (%s) not in same room", __func__, 
           ch->name, victim->name );
      return rVICT_DIED;
   }
   
   if( victim->position == POS_DEAD )
   {
      return rVICT_DIED;
   }

   /*
    * add timer to pkillers 
    */
   if( !IS_NPC( ch ) && !IS_NPC( victim ) )
   {
      if( xIS_SET( ch->act, PLR_NICE ) )
         return rNONE;
      add_timer( ch, TIMER_RECENTFIGHT, 11, NULL, 0 );
      add_timer( victim, TIMER_RECENTFIGHT, 11, NULL, 0 );
   }

   if( is_attack_supressed( ch ) )
      return rNONE;

   if( IS_NPC( ch ) && xIS_SET( ch->act, ACT_NOATTACK ) )
      return rNONE;

   if( ( retcode = one_hit( ch, victim, dt ) ) != rNONE )
      return retcode;

   if( who_fighting( ch ) != victim || dt == gsn_backstab || dt == gsn_circle || dt == gsn_pounce )
      return rNONE;

   /*
    * Very high chance of hitting compared to chance of going berserk 
    * 40% or higher is always hit.. don't learn anything here though. 
    * -- Altrag 
    */
   schance = IS_NPC( ch ) ? 100 : ( learned_percent( ch, gsn_berserk ) * 5 / 2 );
   if( IS_AFFECTED( ch, AFF_BERSERK ) && number_percent(  ) < schance )
      if( ( retcode = one_hit( ch, victim, dt ) ) != rNONE || who_fighting( ch ) != victim )
         return retcode;

   if( get_eq_char( ch, WEAR_DUAL_WIELD ) )
   {
      dual_bonus = IS_NPC( ch ) ? ( ch->level / 10 ) : ( learned_percent( ch, gsn_dual_wield ) / 10 );
      schance = IS_NPC( ch ) ? ch->level : learned_percent( ch, gsn_dual_wield );
      if( number_percent(  ) < schance )
      {
         learn_from_success( ch, gsn_dual_wield );
         retcode = one_hit( ch, victim, dt );
         if( retcode != rNONE || who_fighting( ch ) != victim )
            return retcode;
      }
      else
         learn_from_failure( ch, gsn_dual_wield );
   }
   else
      dual_bonus = 0;

   if( ch->move < 10 )
      dual_bonus = -20;

   /*
    * NPC predetermined number of attacks         -Thoric
    */
   if( IS_NPC( ch ) && ch->numattacks > 0 )
   {
      /* numattacks now includes the initial attack, so we do (numattacks - 1) additional attacks */
      int additional_attacks = ch->numattacks - 1;
      int i;
      
      for( i = 0; i < additional_attacks; i++ )
      {
         retcode = one_hit( ch, victim, dt );
         if( retcode != rNONE || who_fighting( ch ) != victim )
            return retcode;
      }
      return retcode;
   }

   schance = IS_NPC( ch ) ? ch->level : ( int )( ( learned_percent( ch, gsn_second_attack ) + dual_bonus ) / 1.5 );
   if( number_percent(  ) < schance )
   {
      learn_from_success( ch, gsn_second_attack );
      retcode = one_hit( ch, victim, dt );
      if( retcode != rNONE || who_fighting( ch ) != victim )
         return retcode;
   }
   else
      learn_from_failure( ch, gsn_second_attack );

   schance = IS_NPC( ch ) ? ch->level : ( int )( ( learned_percent( ch, gsn_third_attack ) + ( dual_bonus * 1.5 ) ) / 2 );
   if( number_percent(  ) < schance )
   {
      learn_from_success( ch, gsn_third_attack );
      retcode = one_hit( ch, victim, dt );
      if( retcode != rNONE || who_fighting( ch ) != victim )
         return retcode;
   }
   else
      learn_from_failure( ch, gsn_third_attack );

   schance = IS_NPC( ch ) ? ch->level : ( int )( ( learned_percent( ch, gsn_fourth_attack ) + ( dual_bonus * 2 ) ) / 3 );
   if( number_percent(  ) < schance )
   {
      learn_from_success( ch, gsn_fourth_attack );
      retcode = one_hit( ch, victim, dt );
      if( retcode != rNONE || who_fighting( ch ) != victim )
         return retcode;
   }
   else
      learn_from_failure( ch, gsn_fourth_attack );

   schance = IS_NPC( ch ) ? ch->level : ( int )( ( learned_percent( ch, gsn_fifth_attack ) + ( dual_bonus * 3 ) ) / 4 );
   if( number_percent(  ) < schance )
   {
      learn_from_success( ch, gsn_fifth_attack );
      retcode = one_hit( ch, victim, dt );
      if( retcode != rNONE || who_fighting( ch ) != victim )
         return retcode;
   }
   else
      learn_from_failure( ch, gsn_fifth_attack );

   retcode = rNONE;

   schance = IS_NPC( ch ) ? ( int )( ch->level / 2 ) : 0;
   if( number_percent(  ) < schance )
      retcode = one_hit( ch, victim, dt );

   if( retcode == rNONE )
   {
      int move;

      if( !IS_AFFECTED( ch, AFF_FLYING ) && !IS_AFFECTED( ch, AFF_FLOATING ) )
         move = encumbrance( ch, movement_loss[UMIN( SECT_MAX - 1, ch->in_room->sector_type )] );
      else
         move = encumbrance( ch, 1 );
      if( ch->move )
         ch->move = UMAX( 0, ch->move - move );
   }
	tail_chain();
   return retcode;
}

/*
 * Weapon types, haus
 */
int weapon_prof_bonus_check( CHAR_DATA * ch, OBJ_DATA * wield, int *gsn_ptr )
{
   int bonus;

   bonus = 0;
   *gsn_ptr = -1;
   if( !IS_NPC( ch ) && ch->level > 5 && wield )
   {
      switch ( wield->value[3] )
      {
         default:
            *gsn_ptr = -1;
            break;
		 case DAM_BALLISTIC:  
            *gsn_ptr = gsn_firearms;
            break;	
         case DAM_HIT:
         case DAM_SUCTION:
		 case DAM_BITE:
         case DAM_BLAST:
            *gsn_ptr = gsn_unarmed;
            break;
         case DAM_SLASH:
         case DAM_SLICE:
            *gsn_ptr = gsn_long_blades;
            break;
         case DAM_PIERCE:
         case DAM_STAB:
            *gsn_ptr = gsn_short_blades;
            break;
         case DAM_WHIP:
            *gsn_ptr = gsn_flexible_arms;
            break;
         case DAM_CLAW:
            *gsn_ptr = gsn_talonous_arms;
            break;
         case DAM_POUND:
         case DAM_CRUSH:
            *gsn_ptr = gsn_bludgeons;
            break;
         case DAM_BOLT:
         case DAM_ARROW:
         case DAM_DART:
         case DAM_STONE:
         case DAM_PEA:
            *gsn_ptr = gsn_missile_weapons;
            break;

      }
      if( *gsn_ptr != -1 )
         bonus = ( int )( ( learned_percent( ch, *gsn_ptr ) - 50 ) / 10 );

      /*
       * Reduce weapon bonuses for misaligned clannies.
       * if ( IS_CLANNED(ch) )
       * {
       * bonus = bonus * 1000 / 
       * ( 1 + abs( ch->alignment - ch->pcdata->clan->alignment ) );
       * }
       */

      if( IS_DEVOTED( ch ) )
         bonus -= ch->pcdata->favor / -400;

   }
   return bonus;
}

/*
 * Calculate the tohit bonus on the object and return RIS values.
 * -- Altrag
 */
int obj_hitroll( OBJ_DATA * obj )
{
   int tohit = 0;
   AFFECT_DATA *paf;

   for( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
      if( paf->location == APPLY_HITROLL )
         tohit += paf->modifier;
   for( paf = obj->first_affect; paf; paf = paf->next )
      if( paf->location == APPLY_HITROLL )
         tohit += paf->modifier;
   return tohit;
}

/*
 * Offensive shield level modifier
 */
short off_shld_lvl( CHAR_DATA * ch, CHAR_DATA * victim )
{
   short lvl;

   if( !IS_NPC( ch ) )  /* players get much less effect */
   {
      lvl = UMAX( 1, ( ch->level - 10 ) / 2 );
      if( number_percent(  ) + ( victim->level - lvl ) < 40 )
      {
         if( CAN_PKILL( ch ) && CAN_PKILL( victim ) )
            return ch->level;
         else
            return lvl;
      }
      else
         return 0;
   }
   else
   {
      lvl = ch->level / 2;
      if( number_percent(  ) + ( victim->level - lvl ) < 70 )
         return lvl;
      else
         return 0;
   }
}

	/* Calculate combat bonus from power level difference */
int get_pl_combat_bonus( CHAR_DATA *ch, CHAR_DATA *victim )
{
   long long ch_pl, victim_pl;
   double ratio;
   int bonus = 0;
   
   if( !ch || !victim )
      return 0;
      
   ch_pl = get_power_level( ch );
   victim_pl = get_power_level( victim );
   
   if( victim_pl <= 0 )
      return 0;
      
   ratio = (double)ch_pl / (double)victim_pl;
   
   /* DBSC-style combat modifiers */
   if( ratio > 3.0 )
      bonus = 100;  /* 3x stronger = +100% damage */
   else if( ratio > 2.0 )
      bonus = 50;   /* 2x stronger = +50% damage */
   else if( ratio > 1.5 )
      bonus = 25;   /* 1.5x stronger = +25% damage */
   else if( ratio < 0.33 )
      bonus = -75;  /* 3x weaker = -75% damage */
   else if( ratio < 0.5 )
      bonus = -50;  /* 2x weaker = -50% damage */
   else if( ratio < 0.67 )
      bonus = -25;  /* 1.5x weaker = -25% damage */
   
   return bonus;
}

/*
 * Hit one guy once.
 */
ch_ret one_hit( CHAR_DATA * ch, CHAR_DATA * victim, int dt )
{
   OBJ_DATA *wield;
   int plusris, dam, attacktype, cnt, prof_bonus, prof_gsn = -1;
   ch_ret retcode = rNONE;

   /*
    * Can't beat a dead char!
    * Guard against weird room-leavings.
    */
   if( victim->position == POS_DEAD || ch->in_room != victim->in_room )
      return rVICT_DIED;
   if( !ch->in_room )
   {
      bug( "%s: ch (%s) has no in_room!", __func__, ch->name );
      return rERROR;
   }
   
   if( !victim->in_room )
   {
      bug( "%s: victim (%s) has no in_room!", __func__, victim->name );
      return rERROR;
   }

   used_weapon = NULL;
   /*
    * Figure out the weapon doing the damage         -Thoric
    * Dual wield support -- switch weapons each attack
    */
   if( ( wield = get_eq_char( ch, WEAR_DUAL_WIELD ) ) != NULL )
   {
      bool dual_flip = FALSE;
      
      /* Use per-character dual_flip for players, random for NPCs */
      if( !IS_NPC( ch ) && ch->pcdata )
      {
         dual_flip = ch->pcdata->dual_flip;
         ch->pcdata->dual_flip = !ch->pcdata->dual_flip; /* Flip for next attack */
      }
      else
      {
         dual_flip = (number_bits(1) == 0); /* Random for NPCs */
      }
      
      if( dual_flip == FALSE )
      {
         wield = get_eq_char( ch, WEAR_WIELD );
      }
      /* else use the dual_wield weapon already retrieved */
   }
   else
      wield = get_eq_char( ch, WEAR_WIELD );

   used_weapon = wield;

   if( wield )
   {
      /* Check if we can use cached proficiency */
      if( !IS_NPC(ch) && ch->pcdata && ch->pcdata->last_weapon == wield )
      {
         prof_bonus = ch->pcdata->cached_prof_bonus;
         prof_gsn = ch->pcdata->cached_prof_gsn;
      }
      else
      {
         prof_bonus = weapon_prof_bonus_check( ch, wield, &prof_gsn );
         /* Cache the result for players */
         if( !IS_NPC(ch) && ch->pcdata )
         {
            ch->pcdata->last_weapon = wield;
            ch->pcdata->cached_prof_bonus = prof_bonus;
            ch->pcdata->cached_prof_gsn = prof_gsn;
         }
      }
   }
   else
      prof_bonus = 0;

   if( ch->fighting  /* make sure fight is already started */
       && dt == TYPE_UNDEFINED && IS_NPC( ch ) && !xIS_EMPTY( ch->attacks ) )
   {
      cnt = 0;
      for( ;; )
      {
         attacktype = number_range( 0, 6 );
         if( xIS_SET( ch->attacks, attacktype ) )
            break;
         if( cnt++ > 16 )
         {
            attacktype = -1;
            break;
         }
      }
      
      /* Bounds checking for attack type */
      if( attacktype > 6 || attacktype < -1 )
      {
         bug( "%s: invalid attacktype %d for mob %s", __func__, attacktype, ch->name );
         attacktype = -1;
      }
      
      if( attacktype == ATCK_BACKSTAB )
         attacktype = -1;
      if( wield && number_percent(  ) > 25 )
         attacktype = -1;
      if( !wield && number_percent(  ) > 50 )
         attacktype = -1;

      switch ( attacktype )
      {
         default:
            if( attacktype > 0 )
            {
               bug( "%s: unhandled attacktype %d for mob %s", __func__, attacktype, ch->name );
            }
            break;
         case ATCK_CLAWS:
            do_claw( ch, "" );
            retcode = global_retcode;
            break;
         case ATCK_TAIL:
            do_tail( ch, "" );
            retcode = global_retcode;
            break;
         case ATCK_STING:
            do_sting( ch, "" );
            retcode = global_retcode;
            break;
         case ATCK_PUNCH:
            do_punch( ch, "" );
            retcode = global_retcode;
            break;
         case ATCK_KICK:
            do_kick( ch, "" );
            retcode = global_retcode;
            break;
         case ATCK_TRIP:
            attacktype = 0;
            break;
      }
      if( attacktype >= 0 )
         return retcode;
   }

   if( dt == TYPE_UNDEFINED )
   {
      dt = TYPE_HIT;
      if( wield && wield->item_type == ITEM_WEAPON )
         dt += wield->value[3];
   }

   int atk_type = ATK_MELEE;
   bool using_ranged_weapon = FALSE;
   bool using_ki_ability = FALSE;

   if( using_ranged_weapon )
      atk_type = ATK_RANGED;
   else if( using_ki_ability )
      atk_type = ATK_KI;
   else
      atk_type = ATK_MELEE;

   int sn_att = attack_sn_for( ch, atk_type );
   int ar = attack_roll( ch, sn_att, atk_type );
   ar += prof_bonus;
   int dr = defense_roll( victim, atk_type );
   int MoS = ar - dr;

   hit_band_t band = classify_band( MoS );
   avoid_t avo = avoid_reason_from_stance( victim, atk_type );

   if( band == HIT_AVOIDED )
   {
      if( prof_gsn != -1 )
         learn_from_failure( ch, prof_gsn );
      enhanced_dam_message_ex( ch, victim, 0, dt, wield, 0, band, avo );
      tail_chain(  );
      return rNONE;
   }

   int base_dam;

   if( !wield )
      base_dam = number_range( ch->barenumdie, ch->baresizedie * ch->barenumdie ) + ch->damplus;
   else
      base_dam = number_range( wield->value[1], wield->value[2] );

   base_dam += GET_DAMROLL( ch );

   if( prof_bonus )
      base_dam += prof_bonus / 4;

   /*
    * Calculate Damage Modifiers from Victim's Fighting Style
    */
   if( victim->position == POS_BERSERK )
      base_dam = ( int )( 1.2 * base_dam );
   else if( victim->position == POS_AGGRESSIVE )
      base_dam = ( int )( 1.1 * base_dam );
   else if( victim->position == POS_DEFENSIVE )
      base_dam = ( int )( .85 * base_dam );
   else if( victim->position == POS_EVASIVE )
      base_dam = ( int )( .8 * base_dam );

   /*
    * Calculate Damage Modifiers from Attacker's Fighting Style
    */
   if( ch->position == POS_BERSERK )
      base_dam = ( int )( 1.2 * base_dam );
   else if( ch->position == POS_AGGRESSIVE )
      base_dam = ( int )( 1.1 * base_dam );
   else if( ch->position == POS_DEFENSIVE )
      base_dam = ( int )( .85 * base_dam );
   else if( ch->position == POS_EVASIVE )
      base_dam = ( int )( .8 * base_dam );

   if( !IS_NPC( ch ) && ch->pcdata->skills[gsn_enhanced_damage].value_tenths > 0 )
   {
      base_dam += ( int )( base_dam * learned_percent( ch, gsn_enhanced_damage ) / 120 );
      learn_from_success( ch, gsn_enhanced_damage );
   }

   if( !IS_AWAKE( victim ) )
      base_dam *= 2;

   if( dt == gsn_backstab )
      base_dam *= ( 2 + URANGE( 2, ch->level - ( victim->level / 4 ), 30 ) / 8 );

   if( dt == gsn_pounce )
      base_dam *= ( 2 + URANGE( 2, ch->level - ( victim->level / 4 ), 30 ) / 11 );

   if( dt == gsn_circle )
      base_dam *= ( 2 + URANGE( 2, ch->level - ( victim->level / 4 ), 30 ) / 16 );

   if( base_dam <= 0 )
      base_dam = 1;

   double mult = ( band == HIT_GLANCING ? 0.5 : band == HIT_SOLID ? 1.0 : band == HIT_CLEAN ? 1.25 : 0.0 );
   int mit = compute_final_mitigation_percent( victim, MoS, band, atk_type );
   dam = ( int )( base_dam * mult );
   dam = dam - ( dam * mit ) / 100;

   if( dam < 0 )
      dam = 0;

   plusris = 0;

   if( wield )
   {
      if( IS_OBJ_STAT( wield, ITEM_MAGIC ) )
         dam = ris_damage( victim, dam, RIS_MAGIC );
      else
         dam = ris_damage( victim, dam, RIS_NONMAGIC );

      /*
       * Handle PLUS1 - PLUS6 ris bits vs. weapon hitroll   -Thoric
       */
      plusris = obj_hitroll( wield );
   }
   else
      dam = ris_damage( victim, dam, RIS_NONMAGIC );

   /*
    * check for RIS_PLUSx                -Thoric 
    */
   if( dam )
   {
      int x, res, imm, sus, mod;

      if( plusris )
         plusris = RIS_PLUS1 << UMIN( plusris, 7 );

      /*
       * initialize values to handle a zero plusris 
       */
      imm = res = -1;
      sus = 1;

      /*
       * find high ris 
       */
      for( x = RIS_PLUS1; x <= RIS_PLUS6; x <<= 1 )
      {
         if( IS_SET( victim->immune, x ) )
            imm = x;
         if( IS_SET( victim->resistant, x ) )
            res = x;
         if( IS_SET( victim->susceptible, x ) )
            sus = x;
      }
      mod = 10;
      if( imm >= plusris )
         mod -= 10;
      if( res >= plusris )
         mod -= 2;
      if( sus <= plusris )
         mod += 2;

      /*
       * check if immune 
       */
      if( mod <= 0 )
         dam = -1;
      if( mod != 10 )
         dam = ( dam * mod ) / 10;
   }

   if( prof_gsn != -1 )
   {
      if( dam > 0 )
         learn_from_success( ch, prof_gsn );
      else
         learn_from_failure( ch, prof_gsn );
   }

   /*
    * immune to damage 
    */
   if( dam == -1 )
   {
      if( dt >= 0 && dt < num_skills )
      {
         bool found = FALSE;
         SKILLTYPE *skill = skill_table[dt];

         if( skill->imm_char && skill->imm_char[0] != '\0' )
         {
            act( AT_HIT, skill->imm_char, ch, NULL, victim, TO_CHAR );
            found = TRUE;
         }
         if( skill->imm_vict && skill->imm_vict[0] != '\0' )
         {
            act( AT_HITME, skill->imm_vict, ch, NULL, victim, TO_VICT );
            found = TRUE;
         }
         if( skill->imm_room && skill->imm_room[0] != '\0' )
         {
            act( AT_ACTION, skill->imm_room, ch, NULL, victim, TO_NOTVICT );
            found = TRUE;
         }
         if( found )
         {
            tail_chain(  );
            return rNONE;
         }
      }
      dam = 0;
   }

   /*
    * weapon spells  -Thoric 
    */
   if( wield && !IS_SET( victim->immune, RIS_MAGIC ) && !xIS_SET( victim->in_room->room_flags, ROOM_NO_MAGIC ) )
   {
      AFFECT_DATA *aff;

      for( aff = wield->pIndexData->first_affect; aff; aff = aff->next )
         if( aff->location == APPLY_WEAPONSPELL && IS_VALID_SN( aff->modifier ) && skill_table[aff->modifier]->spell_fun )
            retcode = ( *skill_table[aff->modifier]->spell_fun ) ( aff->modifier, ( wield->level + 3 ) / 3, ch, victim );

      if( retcode == rSPELL_FAILED )
         retcode = rNONE;  // Luc, 6/11/2007

      if( retcode != rNONE || char_died( ch ) || char_died( victim ) )
         return retcode;

      for( aff = wield->first_affect; aff; aff = aff->next )
         if( aff->location == APPLY_WEAPONSPELL && IS_VALID_SN( aff->modifier ) && skill_table[aff->modifier]->spell_fun )
            retcode = ( *skill_table[aff->modifier]->spell_fun ) ( aff->modifier, ( wield->level + 3 ) / 3, ch, victim );

      if( retcode == rSPELL_FAILED )
         retcode = rNONE;  // Luc, 6/11/2007

      if( retcode != rNONE || char_died( ch ) || char_died( victim ) )
         return retcode;
   }

   /*
    * magic shields that retaliate          -Thoric
    * FIXED: Check return codes properly to avoid duplicate damage calls
    */
   if( IS_AFFECTED( victim, AFF_FIRESHIELD ) && !IS_AFFECTED( ch, AFF_FIRESHIELD ) )
   {
      retcode = spell_smaug( skill_lookup( "flare" ), off_shld_lvl( victim, ch ), victim, ch );
      if( retcode != rNONE || char_died( ch ) || char_died( victim ) )
         return retcode;
   }

   if( IS_AFFECTED( victim, AFF_ICESHIELD ) && !IS_AFFECTED( ch, AFF_ICESHIELD ) )
   {
      retcode = spell_smaug( skill_lookup( "iceshard" ), off_shld_lvl( victim, ch ), victim, ch );
      if( retcode != rNONE || char_died( ch ) || char_died( victim ) )
         return retcode;
   }

   if( IS_AFFECTED( victim, AFF_SHOCKSHIELD ) && !IS_AFFECTED( ch, AFF_SHOCKSHIELD ) )
   {
      retcode = spell_smaug( skill_lookup( "torrent" ), off_shld_lvl( victim, ch ), victim, ch );
      if( retcode != rNONE || char_died( ch ) || char_died( victim ) )
         return retcode;
   }

   if( IS_AFFECTED( victim, AFF_ACIDMIST ) && !IS_AFFECTED( ch, AFF_ACIDMIST ) )
   {
      retcode = spell_smaug( skill_lookup( "acidshot" ), off_shld_lvl( victim, ch ), victim, ch );
      if( retcode != rNONE || char_died( ch ) || char_died( victim ) )
         return retcode;
   }

   if( IS_AFFECTED( victim, AFF_VENOMSHIELD ) && !IS_AFFECTED( ch, AFF_VENOMSHIELD ) )
   {
      retcode = spell_smaug( skill_lookup( "venomshot" ), off_shld_lvl( victim, ch ), victim, ch );
      if( retcode != rNONE || char_died( ch ) || char_died( victim ) )
         return retcode;
   }

   long long pl_gain = 0;
   enhanced_dam_message_ex( ch, victim, dam, ( unsigned int ) dt, wield, pl_gain, band, avo );

   if( dam > 0 )
      retcode = damage_ex( ch, victim, dam, dt, true );
   else
      retcode = rNONE;

   if( retcode != rNONE )
      return retcode;

   tail_chain(  );
   return retcode;
}

/*
 * Hit one guy with a projectile.
 * Handles use of missile weapons (wield = missile weapon)
 * or thrown items/weapons
 */
ch_ret projectile_hit( CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * wield, OBJ_DATA * projectile, short dist )
{
   int victim_ac;
   int thac0;
   int thac0_00;
   int thac0_32;
   int plusris;
   int dam;
   int diceroll;
   int prof_bonus;
   int prof_gsn = -1;
   int proj_bonus;
   int dt;
   ch_ret retcode;

   if( !projectile )
      return rNONE;

   if( projectile->item_type == ITEM_PROJECTILE || projectile->item_type == ITEM_WEAPON )
   {
      dt = TYPE_HIT + projectile->value[3];
      proj_bonus = number_range( projectile->value[1], projectile->value[2] );
   }
   else
   {
      dt = TYPE_UNDEFINED;
      proj_bonus = number_range( 1, URANGE( 2, get_obj_weight( projectile ), 100 ) );
   }

   /*
    * Can't beat a dead char!
    * Guard against weird room-leavings.
    */
   if( victim->position == POS_DEAD || char_died( victim ) )
   {
      extract_obj( projectile );
      return rVICT_DIED;
   }

   if( wield )
      prof_bonus = weapon_prof_bonus_check( ch, wield, &prof_gsn );
   else
      prof_bonus = 0;

   if( dt == TYPE_UNDEFINED )
   {
      dt = TYPE_HIT;
      if( wield && wield->item_type == ITEM_MISSILE_WEAPON )
         dt += wield->value[3];
   }

   /*
    * Calculate to-hit-armor-class-0 versus armor.
    */
   if( IS_NPC( ch ) )
   {
      thac0_00 = ch->mobthac0;
      thac0_32 = 0;
   }
   else
   {
      thac0_00 = class_table[ch->Class]->thac0_00;
      thac0_32 = class_table[ch->Class]->thac0_32;
   }
   thac0 = interpolate( ch->level, thac0_00, thac0_32 ) - GET_HITROLL( ch ) + ( dist * 2 );
   victim_ac = UMAX( -19, ( int )( GET_AC( victim ) / 10 ) );

   /*
    * if you can't see what's coming... 
    */
   if( !can_see_obj( victim, projectile ) )
      victim_ac += 1;
   if( !can_see( ch, victim ) )
      victim_ac -= 4;

   /*
    * Weapon proficiency bonus 
    */
   victim_ac += prof_bonus;

   /*
    * The moment of excitement!
    */
   while( ( diceroll = number_bits( 5 ) ) >= 20 )
      ;

   if( diceroll == 0 || ( diceroll != 19 && diceroll < thac0 - victim_ac ) )
   {
      /*
       * Miss. 
       */
      if( prof_gsn != -1 )
         learn_from_failure( ch, prof_gsn );

      /*
       * Do something with the projectile 
       */
      if( number_percent(  ) < 50 )
         extract_obj( projectile );
      else
      {
         if( projectile->in_obj )
            obj_from_obj( projectile );
         if( projectile->carried_by )
            obj_from_char( projectile );
         obj_to_room( projectile, victim->in_room );
      }
      damage( ch, victim, 0, dt );
      tail_chain(  );
      return rNONE;
   }

   /*
    * Hit.
    * Calc damage.
    */

   if( !wield )   /* dice formula fixed by Thoric */
      dam = proj_bonus;
   else
      dam = number_range( wield->value[1], wield->value[2] ) + ( proj_bonus / 10 );

   /*
    * Bonuses.
    */
   dam += GET_DAMROLL( ch );

   if( prof_bonus )
      dam += prof_bonus / 4;

   /*
    * Calculate Damage Modifiers from Victim's Fighting Style
    */
   if( victim->position == POS_BERSERK )
      dam = ( int )( 1.2 * dam );
   else if( victim->position == POS_AGGRESSIVE )
      dam = ( int )( 1.1 * dam );
   else if( victim->position == POS_DEFENSIVE )
      dam = ( int )( .85 * dam );
   else if( victim->position == POS_EVASIVE )
      dam = ( int )( .8 * dam );

   if( !IS_NPC( ch ) && ch->pcdata->skills[gsn_enhanced_damage].value_tenths > 0 )
   {
      dam += ( int )( dam * learned_percent( ch, gsn_enhanced_damage ) / 120 );
      learn_from_success( ch, gsn_enhanced_damage );
   }

   if( !IS_AWAKE( victim ) )
      dam *= 2;

   if( dam <= 0 )
      dam = 1;

   plusris = 0;

   if( IS_OBJ_STAT( projectile, ITEM_MAGIC ) )
      dam = ris_damage( victim, dam, RIS_MAGIC );
   else
      dam = ris_damage( victim, dam, RIS_NONMAGIC );

   /*
    * Handle PLUS1 - PLUS6 ris bits vs. weapon hitroll  -Thoric
    */
   if( wield )
      plusris = obj_hitroll( wield );

   /*
    * check for RIS_PLUSx                -Thoric 
    */
   if( dam )
   {
      int x, res, imm, sus, mod;

      if( plusris )
         plusris = RIS_PLUS1 << UMIN( plusris, 7 );

      /*
       * initialize values to handle a zero plusris 
       */
      imm = res = -1;
      sus = 1;

      /*
       * find high ris 
       */
      for( x = RIS_PLUS1; x <= RIS_PLUS6; x <<= 1 )
      {
         if( IS_SET( victim->immune, x ) )
            imm = x;
         if( IS_SET( victim->resistant, x ) )
            res = x;
         if( IS_SET( victim->susceptible, x ) )
            sus = x;
      }
      mod = 10;
      if( imm >= plusris )
         mod -= 10;
      if( res >= plusris )
         mod -= 2;
      if( sus <= plusris )
         mod += 2;

      /*
       * check if immune 
       */
      if( mod <= 0 )
         dam = -1;
      if( mod != 10 )
         dam = ( dam * mod ) / 10;
   }

   if( prof_gsn != -1 )
   {
      if( dam > 0 )
         learn_from_success( ch, prof_gsn );
      else
         learn_from_failure( ch, prof_gsn );
   }

   /*
    * immune to damage 
    */
   if( dam == -1 )
   {
      if( dt >= 0 && dt < num_skills )
      {
         SKILLTYPE *skill = skill_table[dt];
         bool found = FALSE;

         if( skill->imm_char && skill->imm_char[0] != '\0' )
         {
            act( AT_HIT, skill->imm_char, ch, NULL, victim, TO_CHAR );
            found = TRUE;
         }
         if( skill->imm_vict && skill->imm_vict[0] != '\0' )
         {
            act( AT_HITME, skill->imm_vict, ch, NULL, victim, TO_VICT );
            found = TRUE;
         }
         if( skill->imm_room && skill->imm_room[0] != '\0' )
         {
            act( AT_ACTION, skill->imm_room, ch, NULL, victim, TO_NOTVICT );
            found = TRUE;
         }
         if( found )
         {
            if( number_percent(  ) < 50 )
               extract_obj( projectile );
            else
            {
               if( projectile->in_obj )
                  obj_from_obj( projectile );
               if( projectile->carried_by )
                  obj_from_char( projectile );
               obj_to_room( projectile, victim->in_room );
            }
            return rNONE;
         }
      }
      dam = 0;
   }

   /*
    * weapon spells  -Thoric 
    */
   if( wield && !IS_SET( victim->immune, RIS_MAGIC ) && !xIS_SET( victim->in_room->room_flags, ROOM_NO_MAGIC ) )
   {
      AFFECT_DATA *aff;

      for( aff = wield->pIndexData->first_affect; aff; aff = aff->next )
         if( aff->location == APPLY_WEAPONSPELL && IS_VALID_SN( aff->modifier ) && skill_table[aff->modifier]->spell_fun )
         {
            retcode = ( *skill_table[aff->modifier]->spell_fun ) ( aff->modifier, ( wield->level + 3 ) / 3, ch, victim );
            if( retcode != rNONE || char_died( ch ) || char_died( victim ) )
            {
               extract_obj( projectile );
               return retcode;
            }
         }

      for( aff = wield->first_affect; aff; aff = aff->next )
         if( aff->location == APPLY_WEAPONSPELL && IS_VALID_SN( aff->modifier ) && skill_table[aff->modifier]->spell_fun )
         {
            retcode = ( *skill_table[aff->modifier]->spell_fun ) ( aff->modifier, ( wield->level + 3 ) / 3, ch, victim );
            if( retcode != rNONE || char_died( ch ) || char_died( victim ) )
            {
               extract_obj( projectile );
               return retcode;
            }
         }
   }

   /* 
    * FIXED: Actually do the projectile damage! This was missing in the original
    */
   retcode = damage( ch, victim, dam, dt );
   if( retcode != rNONE )
   {
      extract_obj( projectile );
      return retcode;
   }

   if( char_died( ch ) )
   {
      extract_obj( projectile );
      return rCHAR_DIED;
   }

   if( char_died( victim ) )
   {
      extract_obj( projectile );
      return rVICT_DIED;
   }

   retcode = rNONE;
   if( dam == 0 )
   {
      if( number_percent(  ) < 50 )
         extract_obj( projectile );
      else
      {
         if( projectile->in_obj )
            obj_from_obj( projectile );
         if( projectile->carried_by )
            obj_from_char( projectile );
         obj_to_room( projectile, victim->in_room );
      }
   }
   else
   {
      /* Always extract projectile on successful hit */
      extract_obj( projectile );
   }

   tail_chain(  );
   return retcode;
}

/*
 * Calculate damage based on resistances, immunities and suceptibilities
 *					-Thoric
 */
short ris_damage( CHAR_DATA * ch, short dam, int ris )
{
   short modifier;

   modifier = 10;
   if( IS_SET( ch->immune, ris ) && !IS_SET( ch->no_immune, ris ) )
      modifier -= 10;
   if( IS_SET( ch->resistant, ris ) && !IS_SET( ch->no_resistant, ris ) )
      modifier -= 2;
   if( IS_SET( ch->susceptible, ris ) && !IS_SET( ch->no_susceptible, ris ) )
   {
      if( IS_NPC( ch ) && IS_SET( ch->immune, ris ) )
         modifier += 0;
      else
         modifier += 2;
   }
   if( modifier <= 0 )
      return -1;
   if( modifier == 10 )
      return dam;
   return ( dam * modifier ) / 10;
}

/*
 * Inflict damage from a hit. This is one damn big function.
 */
ch_ret damage_ex( CHAR_DATA * ch, CHAR_DATA * victim, int dam, int dt, bool suppress_message )
{
   char log_buf[MAX_STRING_LENGTH];
   short maxdam;
   bool npcvict;
   bool loot;
   int xp_gain;
   // ch_ret retcode; potential delete
   // CHAR_DATA *gch; potential delete
   int original_dam = dam;  /* Store original damage for RIS comparison */
   long long pl_gained = 0;

   //retcode = rNONE; potential delete

   if( !ch )
   {
      bug( "%s: null ch!", __func__ );
      return rERROR;
   }

   if( !victim )
   {
      bug( "%s: null victim!", __func__ );
      return rVICT_DIED;
   }

   if( victim->position == POS_DEAD )
      return rVICT_DIED;

   npcvict = IS_NPC( victim );

   /* Calculate PL gain BEFORE RIS processing */
   if( original_dam > 0 && ch != victim && !IS_NPC( ch ) && IS_NPC( victim ) )
   {
      long long ch_pl = get_power_level( ch );
      long long victim_pl = get_power_level( victim );
      
      /* Only gain PL if the opponent isn't too weak */
      if( victim_pl >= (ch_pl / 10) )  /* Victim must be at least 1/10th your PL */
      {
         double ratio = (double)victim_pl / (double)ch_pl;
         
         /* Base PL gain calculation */
         if( ratio >= 1.0 )        /* Equal or stronger opponent */
            pl_gained = original_dam / 8;
         else if( ratio >= 0.5 )   /* Half as strong */
            pl_gained = original_dam / 15;
         else if( ratio >= 0.2 )   /* 1/5 as strong */
            pl_gained = original_dam / 40;
         else if( ratio >= 0.1 )   /* 1/10 as strong */
            pl_gained = original_dam / 80;
         
         /* Apply attacker's PL scaling to gain (higher PL = smaller gains) */
         if( ch_pl > 1000000000LL )      /* 1 billion+ PL */
            pl_gained = pl_gained / 3;
         else if( ch_pl > 100000000LL )  /* 100 million+ PL */
            pl_gained = pl_gained * 2 / 3;
         else if( ch_pl > 10000000LL )   /* 10 million+ PL */
            pl_gained = pl_gained * 85 / 100;
      }
   }

   /*
    * Check for areas where violence is not permitted
    */
   if( IS_PACIFIST( ch ) )
   {
      char buf[MAX_STRING_LENGTH];
      snprintf( buf, MAX_STRING_LENGTH, "%s turned violent in a PACIFIST area (%d)", ch->name, ch->in_room->vnum );
      log_string( buf );
      return rNONE;
   }

   /*
    * Some quick checking to make sure that people can't abuse
    * this function to bypass checks such as player killer, etc...
    * As a side note, dam_message( ) should be the one calling
    * this function, so the dtypes SHOULD be fine.  -Orion
    */
   if( dam )
   {
      if( !IS_NPC( ch ) && !IS_NPC( victim ) )
      {
         if( get_timer( victim, TIMER_PKILLED ) > 0 )
         {
            send_to_char( "That character has died within the last 5 minutes.\r\n", ch );
            return rNONE;
         }

         if( get_timer( ch, TIMER_PKILLED ) > 0 )
         {
            send_to_char( "You have been killed within the last 5 minutes.\r\n", ch );
            return rNONE;
         }

         if( xIS_SET( victim->act, PLR_KILLER ) || xIS_SET( victim->act, PLR_THIEF ) )
            npcvict = TRUE;
         else if( !IS_PKILL( ch ) || !IS_PKILL( victim ) )
         {
            if( IS_PKILL( ch ) && !IS_PKILL( victim ) )
               send_to_char( "You can only attack other deadly characters.\r\n", ch );
            else
               send_to_char( "That character is not a deadly character.\r\n", ch );
            return rNONE;
         }
      }

      /*
       * Damage modifiers.
       */

      if( IS_AFFECTED( victim, AFF_SANCTUARY ) )
         dam /= 2;

      if( IS_AFFECTED( victim, AFF_PROTECT ) && IS_EVIL( ch ) )
         dam -= ( int )( dam / 4 );

      if( dam < 0 )
         dam = 0;

      /*
       * Check for parry, and dodge.
       */
      if( dt >= TYPE_HIT && ch != victim )
      {
         if( check_parry( ch, victim ) )
            return rNONE;
         if( check_dodge( ch, victim ) )
            return rNONE;
      }

      /*
       * Dam reduction from spells/items/weapons
       */
      if( !IS_FIRE( dt ) && !IS_COLD( dt ) && !IS_ACID( dt ) && !IS_ELECTRICITY( dt ) && !IS_ENERGY( dt ) )
      {
         if( dt == ( TYPE_HIT + DAM_POUND ) || dt == ( TYPE_HIT + DAM_CRUSH )
             || dt == ( TYPE_HIT + DAM_STONE ) || dt == ( TYPE_HIT + DAM_PEA ) )
            dam = ris_damage( victim, dam, RIS_BLUNT );
         else if( dt == ( TYPE_HIT + DAM_STAB ) || dt == ( TYPE_HIT + DAM_PIERCE )
                  || dt == ( TYPE_HIT + DAM_BITE ) || dt == ( TYPE_HIT + DAM_BOLT )
                  || dt == ( TYPE_HIT + DAM_DART ) || dt == ( TYPE_HIT + DAM_ARROW ) )
            dam = ris_damage( victim, dam, RIS_PIERCE );
         else if( dt == ( TYPE_HIT + DAM_SLICE ) || dt == ( TYPE_HIT + DAM_SLASH )
                  || dt == ( TYPE_HIT + DAM_WHIP ) || dt == ( TYPE_HIT + DAM_CLAW ) )
            dam = ris_damage( victim, dam, RIS_SLASH );

         if( dam == -1 )
         {
            if( dt >= 0 && dt < num_skills )
            {
               bool found = FALSE;
               SKILLTYPE *skill = skill_table[dt];

               if( skill->imm_char && skill->imm_char[0] != '\0' )
               {
                  act( AT_HIT, skill->imm_char, ch, NULL, victim, TO_CHAR );
                  found = TRUE;
               }
               if( skill->imm_vict && skill->imm_vict[0] != '\0' )
               {
                  act( AT_HITME, skill->imm_vict, ch, NULL, victim, TO_VICT );
                  found = TRUE;
               }
               if( skill->imm_room && skill->imm_room[0] != '\0' )
               {
                  act( AT_ACTION, skill->imm_room, ch, NULL, victim, TO_NOTVICT );
                  found = TRUE;
               }
               if( found )
                  return rNONE;
            }
            dam = 0;
         }
      }

      /*
       * Precautionary step mainly to prevent people in Hell from finding
       * a way out. --Shaddai
       */
      if( xIS_SET( victim->in_room->room_flags, ROOM_SAFE ) )
         dam = 0;

      if( dam && npcvict && ch != victim )
      {
         if( !xIS_SET( victim->act, ACT_SENTINEL ) )
         {
            if( victim->hunting )
            {
               if( victim->hunting->who != ch )
               {
                  STRFREE( victim->hunting->name );
                  victim->hunting->name = QUICKLINK( ch->name );
                  victim->hunting->who = ch;
               }
            }
            else if( !xIS_SET( victim->act, ACT_PACIFIST ) )
               start_hunting( victim, ch );
         }

         if( victim->hating )
         {
            if( victim->hating->who != ch )
            {
               STRFREE( victim->hating->name );
               victim->hating->name = QUICKLINK( ch->name );
               victim->hating->who = ch;
            }
         }
         else if( !xIS_SET( victim->act, ACT_PACIFIST ) )
            start_hating( victim, ch );
      }

      /*
       * Stop up any residual loopholes.
       */
      if( dt == gsn_backstab )
         maxdam = ch->level * 80;
      else
         maxdam = ch->level * 40;

      if( dam > maxdam )
      {
         bug( "%s: %d more than %d points!", __func__, dam, maxdam );
         dam = maxdam;
      }
   }

   /* Handle the damage message display */
   if( ch != victim && !suppress_message )
      new_dam_message( ch, victim, dam, dt, NULL, pl_gained );

   /*
    * Hurt the victim.
    * Inform the victim of his new state.
    */
   victim->hit -= dam;

   /*
    * Get experience based on % of damage done       -Thoric
    */
   if( dam && ch != victim && !IS_NPC( ch ) && ch->fighting && ch->fighting->xp )
   {
      if( ch->fighting->who == victim )
         xp_gain = ( int )( ch->fighting->xp * dam ) / victim->max_hit;
      else
         xp_gain = ( int )( xp_compute( ch, victim ) * 0.85 * dam ) / victim->max_hit;
      if( dt == gsn_backstab || dt == gsn_circle )
         xp_gain = ( int )( xp_gain * 0.05 );
   }

   if( !IS_NPC( victim ) && victim->level >= LEVEL_IMMORTAL && victim->hit < 1 )
      victim->hit = 1;

   /*
    * Make sure newbies dont die 
    */
   if( !IS_NPC( victim ) && NOT_AUTHED( victim ) && victim->hit < 1 )
      victim->hit = 1;

   if( dam > 0 && dt > TYPE_HIT
       && !IS_AFFECTED( victim, AFF_POISON )
       && is_wielding_poisoned( ch ) && !IS_SET( victim->immune, RIS_POISON ) && !saves_poison_death( ch->level, victim ) )
   {
      AFFECT_DATA af;

      af.type = gsn_poison;
      af.duration = 20;
      af.location = APPLY_STR;
      af.modifier = -2;
      af.bitvector = meb( AFF_POISON );
      affect_join( victim, &af );
   }

   /*
    * Vampire self preservation          -Thoric
    */
   if( IS_VAMPIRE( victim ) )
   {
      if( dam >= ( victim->max_hit / 10 ) )  /* get hit hard, lose blood */
         gain_condition( victim, COND_BLOODTHIRST, -1 - ( victim->level / 20 ) );
      if( victim->hit <= ( victim->max_hit / 8 ) && victim->pcdata->condition[COND_BLOODTHIRST] > 5 )
      {
         gain_condition( victim, COND_BLOODTHIRST, -URANGE( 3, victim->level / 10, 8 ) );
         victim->hit += URANGE( 4, ( victim->max_hit / 30 ), 15 );
         set_char_color( AT_BLOOD, victim );
         send_to_char( "You howl with rage as the beast within stirs!\r\n", victim );
      }
   }

   if( !npcvict && get_trust( victim ) >= LEVEL_IMMORTAL && get_trust( ch ) >= LEVEL_IMMORTAL && victim->hit < 1 )
      victim->hit = 1;
   update_pos( victim );

  gain_pl( ch, pl_gained, false );

   switch ( victim->position )
   {
      case POS_MORTAL:
         act( AT_DYING, "$n is mortally wounded, and will die soon, if not aided.", victim, NULL, NULL, TO_ROOM );
         act( AT_DANGER, "You are mortally wounded, and will die soon, if not aided.", victim, NULL, NULL, TO_CHAR );
         break;

      case POS_INCAP:
         act( AT_DYING, "$n is incapacitated and will slowly die, if not aided.", victim, NULL, NULL, TO_ROOM );
         act( AT_DANGER, "You are incapacitated and will slowly die, if not aided.", victim, NULL, NULL, TO_CHAR );
         break;

      case POS_STUNNED:
         if( !IS_AFFECTED( victim, AFF_PARALYSIS ) )
         {
            act( AT_ACTION, "$n is stunned, but will probably recover.", victim, NULL, NULL, TO_ROOM );
            act( AT_HURT, "You are stunned, but will probably recover.", victim, NULL, NULL, TO_CHAR );
         }
         break;

      case POS_DEAD:
         act( AT_DEAD, "$n is DEAD!!", victim, 0, 0, TO_ROOM );
         act( AT_DEAD, "You have been KILLED!!\r\n", victim, 0, 0, TO_CHAR );
         break;

      default:
         if( dam > ( victim->max_hit / 4 ) )
         {
            act( AT_HURT, "That really did HURT!", victim, 0, 0, TO_CHAR );
         }
         if( victim->hit < ( victim->max_hit / 4 ) )
         {
            act( AT_DANGER, "You wish that your wounds would stop BLEEDING so much!", victim, 0, 0, TO_CHAR );
         }
         break;
   }
	
	/* Check for racial transformation unlock */
   if (victim->position > POS_DEAD && dam > 0) 
   {
      try_unlock_racial_transformation(victim, ch);
   }

   /*
    * Sleep spells and extremely wounded folks.
    */
   if( !IS_AWAKE( victim ) /* lets make NPC's not slaughter PC's */
       && !IS_AFFECTED( victim, AFF_PARALYSIS ) )
   {
      if( victim->fighting && victim->fighting->who->hunting && victim->fighting->who->hunting->who == victim )
         stop_hunting( victim->fighting->who );

      if( victim->fighting && victim->fighting->who->hating && victim->fighting->who->hating->who == victim )
         stop_hating( victim->fighting->who );

      if( !npcvict && IS_NPC( ch ) )
         stop_fighting( victim, FALSE );
      else
         stop_fighting( victim, TRUE );
   }

   /*
    * Payoff for killing things.
    */
   if( victim->position == POS_DEAD )
   {
      group_gain( ch, victim );

      if( !npcvict )
      {
         snprintf( log_buf, MAX_STRING_LENGTH, "%s (%d) killed by %s at %d",
                   victim->name, victim->level, ( IS_NPC( ch ) ? ch->short_descr : ch->name ), victim->in_room->vnum );
         log_string( log_buf );
         to_channel( log_buf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );

         /*
          * Dying penalty:
          * 1/2 your current exp  
          */
         if( victim->exp > 0 )
         {
            victim->exp = ( victim->exp / 2 );
         }

         /*
          * New penalty... go back to character creation if this is a new character!
          */
         if( NOT_AUTHED( victim ) )
         {
            if( victim->level == 1 && victim->exp == 0 )
            {
               victim->hit = victim->max_hit;
               victim->mana = victim->max_mana;
               victim->move = victim->max_move;
               victim->position = POS_RESTING;
               char_to_room( victim, get_room_index( ROOM_VNUM_SCHOOL ) );
            }
         }
      }
      else if( !IS_NPC( ch ) )  /* keep track of mob vnum killed */
         add_kill( ch, victim );

      check_killer( ch, victim );

      if( ch->in_room == victim->in_room )
         loot = legal_loot( ch, victim );
      else
         loot = FALSE;

      set_cur_char( victim );
      raw_kill( ch, victim );
      victim = NULL;

      if( !IS_NPC( ch ) && loot )
      {
         /* Autogold by Scryn 8/12 */
         if( xIS_SET( ch->act, PLR_AUTOGOLD ) )
         {
            interpret( ch, "get coins corpse" );
         }
         /* Autoloot by Scryn 8/12 */
         if( xIS_SET( ch->act, PLR_AUTOLOOT ) && victim != ch )  /* prevent autoloot the own corpse */
         {
            interpret( ch, "get all corpse" );
         }
         /* Autosac by Scryn 8/12 */
         if( xIS_SET( ch->act, PLR_AUTOSAC ) )
         {
            interpret( ch, "sacrifice corpse" );
         }
      }

      /* Killing someone clears ATTACKER flag */
      if( !IS_NPC( ch ) )
         xREMOVE_BIT( ch->act, PLR_ATTACKER );

      return rVICT_DIED;
   }

   if( victim == ch )
      return rNONE;

   /*
    * Take care of link dead people.
    */
   if( !npcvict && !victim->desc && !IS_SET( victim->pcdata->flags, PCFLAG_NORECALL ) )
   {
      if( number_range( 0, victim->wait ) == 0 )
      {
         act( AT_MAGIC, "$n disappears in a swirling mist.", victim, NULL, NULL, TO_ROOM );
         char_from_room( victim );
         char_to_room( victim, get_room_index( ROOM_VNUM_TEMPLE ) );
         act( AT_MAGIC, "$n appears in a swirling mist.", victim, NULL, NULL, TO_ROOM );
         victim->position = POS_RESTING;
         return rNONE;
      }
   }

   if( npcvict && dam > 0 )
   {
      if( ( xIS_SET( victim->act, ACT_WIMPY ) && number_bits( 1 ) == 0
            && victim->hit < victim->max_hit / 2 )
          || ( IS_AFFECTED( victim, AFF_CHARM ) && victim->master && victim->master->in_room != victim->in_room ) )
      {
         start_fearing( victim, ch );
         stop_hunting( victim );
         do_flee( victim, "" );
      }
   }

   if( !npcvict && victim->hit > 0 && victim->hit <= victim->wimpy && victim->wait == 0 )
      do_flee( victim, "" );
   else if( !npcvict && xIS_SET( victim->act, PLR_FLEE ) )
      do_flee( victim, "" );

   tail_chain(  );
   return rNONE;
}

/*
 * Changed is_safe to have the show_messg boolian.  This is so if you don't
 * want to show why you can't kill someone you can't turn it off.  This is
 * useful for things like area attacks.  --Shaddai
 */
bool is_safe( CHAR_DATA * ch, CHAR_DATA * victim, bool show_messg )
{
   if( char_died( victim ) || char_died( ch ) )
      return TRUE;

   /*
    * Thx Josh! 
    */
   if( who_fighting( ch ) == ch )
      return FALSE;

   if( !victim )  /*Gonna find this is_safe crash bug -Blod */
   {
      bug( "%s: %s opponent does not exist!", __func__, ch->name );
      return TRUE;
   }

   if( !victim->in_room )
   {
      bug( "%s: %s has no physical location!", __func__, victim->name );
      return TRUE;
   }

   if( xIS_SET( victim->in_room->room_flags, ROOM_SAFE ) )
   {
      if( show_messg )
      {
         set_char_color( AT_MAGIC, ch );
         send_to_char( "A magical force prevents you from attacking.\r\n", ch );
      }
      return TRUE;
   }

   if( IS_PACIFIST( ch ) ) /* Fireblade */
   {
      if( show_messg )
      {
         set_char_color( AT_MAGIC, ch );
         ch_printf( ch, "You are a pacifist and will not fight.\r\n" );
      }
      return TRUE;
   }

   if( IS_PACIFIST( victim ) )   /* Gorog */
   {
      char buf[MAX_STRING_LENGTH];
      if( show_messg )
      {
         snprintf( buf, MAX_STRING_LENGTH, "%s is a pacifist and will not fight.\r\n", capitalize( victim->short_descr ) );
         set_char_color( AT_MAGIC, ch );
         send_to_char( buf, ch );
      }
      return TRUE;
   }

   if( !IS_NPC( ch ) && ch->level >= LEVEL_IMMORTAL )
      return FALSE;

   if( !IS_NPC( ch ) && !IS_NPC( victim ) && ch != victim && IS_SET( victim->in_room->area->flags, AFLAG_NOPKILL ) )
   {
      if( show_messg )
      {
         set_char_color( AT_IMMORT, ch );
         send_to_char( "The gods have forbidden player killing in this area.\r\n", ch );
      }
      return TRUE;
   }

   if( IS_NPC( ch ) || IS_NPC( victim ) )
      return FALSE;

   if( calculate_age( ch ) < 18 || ch->level < 5 )
   {
      if( show_messg )
      {
         set_char_color( AT_WHITE, ch );
         send_to_char( "You are not yet ready, needing age or experience, if not both. \r\n", ch );
      }
      return TRUE;
   }

   if( calculate_age( victim ) < 18 || victim->level < 5 )
   {
      if( show_messg )
      {
         set_char_color( AT_WHITE, ch );
         send_to_char( "They are yet too young to die.\r\n", ch );
      }
      return TRUE;
   }

   if( ch->level - victim->level > 5 || victim->level - ch->level > 5 )
   {
      if( show_messg )
      {
         set_char_color( AT_IMMORT, ch );
         send_to_char( "The gods do not allow murder when there is such a difference in level.\r\n", ch );
      }
      return TRUE;
   }

   if( get_timer( victim, TIMER_PKILLED ) > 0 )
   {
      if( show_messg )
      {
         set_char_color( AT_GREEN, ch );
         send_to_char( "That character has died within the last 5 minutes.\r\n", ch );
      }
      return TRUE;
   }

   if( get_timer( ch, TIMER_PKILLED ) > 0 )
   {
      if( show_messg )
      {
         set_char_color( AT_GREEN, ch );
         send_to_char( "You have been killed within the last 5 minutes.\r\n", ch );
      }
      return TRUE;
   }

   return FALSE;
}

/*
 * just verify that a corpse looting is legal
 */
bool legal_loot( CHAR_DATA * ch, CHAR_DATA * victim )
{
   /*
    * anyone can loot mobs 
    */
   if( IS_NPC( victim ) )
      return TRUE;
   /*
    * non-charmed mobs can loot anything 
    */
   if( IS_NPC( ch ) && !ch->master )
      return TRUE;
   /*
    * members of different clans can loot too! -Thoric 
    */
   if( !IS_NPC( ch ) && !IS_NPC( victim )
       && IS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) && IS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) )
      return TRUE;
   return FALSE;
}

/*
 * See if an attack justifies a KILLER flag.
 */
void check_killer( CHAR_DATA * ch, CHAR_DATA * victim )
{
   /*
    * NPC's are fair game.
    */
   if( IS_NPC( victim ) )
   {
      if( !IS_NPC( ch ) )
      {
         int level_ratio;
         /*
          * Fix for crashes when killing mobs of level 0
          * * by Joe Fabiano -rinthos@yahoo.com
          * * on 03-16-03.
          */
         if( victim->level < 1 )
            level_ratio = URANGE( 1, ch->level, MAX_LEVEL );
         else
            level_ratio = URANGE( 1, ch->level / victim->level, MAX_LEVEL );
         if( ch->pcdata->clan )
            ch->pcdata->clan->mkills++;
         ch->pcdata->mkills++;
         ch->in_room->area->mkills++;
         if( ch->pcdata->deity )
         {
            if( victim->race == ch->pcdata->deity->npcrace )
               adjust_favor( ch, 3, level_ratio );
            else if( victim->race == ch->pcdata->deity->npcfoe )
               adjust_favor( ch, 17, level_ratio );
            else
               adjust_favor( ch, 2, level_ratio );
         }
      }
      return;
   }

   /*
    * If you kill yourself nothing happens.
    */
   if( ch == victim || ch->level >= LEVEL_IMMORTAL )
      return;

   /*
    * Any character in the arena is ok to kill.
    * Added pdeath and pkills here
    */
   if( in_arena( ch ) )
   {
      if( !IS_NPC( ch ) && !IS_NPC( victim ) )
      {
         ch->pcdata->pkills++;
         victim->pcdata->pdeaths++;
      }
      return;
   }

   /*
    * So are killers and thieves.
    */
   if( xIS_SET( victim->act, PLR_KILLER ) || xIS_SET( victim->act, PLR_THIEF ) )
   {
      if( !IS_NPC( ch ) )
      {
         if( ch->pcdata->clan )
         {
            if( victim->level < 10 )
               ch->pcdata->clan->pkills[0]++;
            else if( victim->level < 15 )
               ch->pcdata->clan->pkills[1]++;
            else if( victim->level < 20 )
               ch->pcdata->clan->pkills[2]++;
            else if( victim->level < 30 )
               ch->pcdata->clan->pkills[3]++;
            else if( victim->level < 40 )
               ch->pcdata->clan->pkills[4]++;
            else if( victim->level < 50 )
               ch->pcdata->clan->pkills[5]++;
            else
               ch->pcdata->clan->pkills[6]++;
         }
         ch->pcdata->pkills++;
         ch->in_room->area->pkills++;
      }
      return;
   }

   /*
    * clan checks               -Thoric 
    */
   if( !IS_NPC( ch ) && !IS_NPC( victim )
       && IS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) && IS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) )
   {
      /*
       * not of same clan? Go ahead and kill!!! 
       */
      if( !ch->pcdata->clan
          || !victim->pcdata->clan
          || ( ch->pcdata->clan->clan_type != CLAN_NOKILL
               && victim->pcdata->clan->clan_type != CLAN_NOKILL && ch->pcdata->clan != victim->pcdata->clan ) )
      {
         if( ch->pcdata->clan )
         {
            if( victim->level < 10 )
               ch->pcdata->clan->pkills[0]++;
            else if( victim->level < 15 )
               ch->pcdata->clan->pkills[1]++;
            else if( victim->level < 20 )
               ch->pcdata->clan->pkills[2]++;
            else if( victim->level < 30 )
               ch->pcdata->clan->pkills[3]++;
            else if( victim->level < 40 )
               ch->pcdata->clan->pkills[4]++;
            else if( victim->level < 50 )
               ch->pcdata->clan->pkills[5]++;
            else
               ch->pcdata->clan->pkills[6]++;
         }
         ch->pcdata->pkills++;
         ch->hit = ch->max_hit;
         ch->mana = ch->max_mana;
         ch->move = ch->max_move;
         if( ch->pcdata )
            ch->pcdata->condition[COND_BLOODTHIRST] = ( 10 + ch->level );
         update_pos( victim );
         if( victim != ch )
         {
            act( AT_MAGIC, "Bolts of blue energy rise from the corpse, seeping into $n.", ch, victim->name, NULL, TO_ROOM );
            act( AT_MAGIC, "Bolts of blue energy rise from the corpse, seeping into you.", ch, victim->name, NULL, TO_CHAR );
         }
         if( victim->pcdata->clan )
         {
            if( ch->level < 10 )
               victim->pcdata->clan->pdeaths[0]++;
            else if( ch->level < 15 )
               victim->pcdata->clan->pdeaths[1]++;
            else if( ch->level < 20 )
               victim->pcdata->clan->pdeaths[2]++;
            else if( ch->level < 30 )
               victim->pcdata->clan->pdeaths[3]++;
            else if( ch->level < 40 )
               victim->pcdata->clan->pdeaths[4]++;
            else if( ch->level < 50 )
               victim->pcdata->clan->pdeaths[5]++;
            else
               victim->pcdata->clan->pdeaths[6]++;
         }
         victim->pcdata->pdeaths++;
         adjust_favor( victim, 11, 1 );
         adjust_favor( ch, 2, 1 );
         add_timer( victim, TIMER_PKILLED, 115, NULL, 0 );
         WAIT_STATE( victim, 3 * PULSE_VIOLENCE );
         /*
          * xSET_BIT(victim->act, PLR_PK); 
          */
         return;
      }
   }

   /*
    * Charm-o-rama.
    */
   if( IS_AFFECTED( ch, AFF_CHARM ) )
   {
      if( !ch->master )
      {
         bug( "%s: %s bad AFF_CHARM", __func__, IS_NPC( ch ) ? ch->short_descr : ch->name );
         affect_strip( ch, gsn_charm_person );
         xREMOVE_BIT( ch->affected_by, AFF_CHARM );
         return;
      }

      /*
       * stop_follower( ch ); 
       */
      if( ch->master )
         check_killer( ch->master, victim );
      return;
   }

   /*
    * NPC's are cool of course (as long as not charmed).
    * Hitting yourself is cool too (bleeding).
    * So is being immortal (Alander's idea).
    * And current killers stay as they are.
    */
   if( IS_NPC( ch ) )
   {
      if( !IS_NPC( victim ) )
      {
         int level_ratio;
         if( victim->pcdata->clan )
            victim->pcdata->clan->mdeaths++;
         victim->pcdata->mdeaths++;
         victim->in_room->area->mdeaths++;
         level_ratio = URANGE( 1, ch->level / victim->level, LEVEL_AVATAR );
         if( victim->pcdata->deity )
         {
            if( ch->race == victim->pcdata->deity->npcrace )
               adjust_favor( victim, 12, level_ratio );
            else if( ch->race == victim->pcdata->deity->npcfoe )
               adjust_favor( victim, 15, level_ratio );
            else
               adjust_favor( victim, 11, level_ratio );
         }
      }
      return;
   }

   if( !IS_NPC( ch ) )
   {
      if( ch->pcdata->clan )
         ch->pcdata->clan->illegal_pk++;
      ch->pcdata->illegal_pk++;
      ch->in_room->area->illegal_pk++;
   }

   if( !IS_NPC( victim ) )
   {
      if( victim->pcdata->clan )
      {
         if( ch->level < 10 )
            victim->pcdata->clan->pdeaths[0]++;
         else if( ch->level < 15 )
            victim->pcdata->clan->pdeaths[1]++;
         else if( ch->level < 20 )
            victim->pcdata->clan->pdeaths[2]++;
         else if( ch->level < 30 )
            victim->pcdata->clan->pdeaths[3]++;
         else if( ch->level < 40 )
            victim->pcdata->clan->pdeaths[4]++;
         else if( ch->level < 50 )
            victim->pcdata->clan->pdeaths[5]++;
         else
            victim->pcdata->clan->pdeaths[6]++;
      }
      victim->pcdata->pdeaths++;
      victim->in_room->area->pdeaths++;
   }

   if( xIS_SET( ch->act, PLR_KILLER ) )
      return;

   set_char_color( AT_WHITE, ch );
   send_to_char( "A strange feeling grows deep inside you, and a tingle goes up your spine...\r\n", ch );
   set_char_color( AT_IMMORT, ch );
   send_to_char( "A deep voice booms inside your head, 'Thou shall now be known as a deadly murderer!!!'\r\n", ch );
   set_char_color( AT_WHITE, ch );
   send_to_char( "You feel as if your soul has been revealed for all to see.\r\n", ch );
   xSET_BIT( ch->act, PLR_KILLER );
   if( xIS_SET( ch->act, PLR_ATTACKER ) )
      xREMOVE_BIT( ch->act, PLR_ATTACKER );
   save_char_obj( ch );
}

/*
 * See if an attack justifies a ATTACKER flag.
 */
void check_attacker( CHAR_DATA * ch, CHAR_DATA * victim )
{
/* 
 * Made some changes to this function Apr 6/96 to reduce the prolifiration
 * of attacker flags in the realms. -Narn
 */
   /*
    * NPC's are fair game.
    * So are killers and thieves.
    */
   if( IS_NPC( victim ) || xIS_SET( victim->act, PLR_KILLER ) || xIS_SET( victim->act, PLR_THIEF ) )
      return;

   /*
    * deadly char check 
    */
   if( !IS_NPC( ch ) && !IS_NPC( victim ) && CAN_PKILL( ch ) && CAN_PKILL( victim ) )
      return;

/* Pkiller versus pkiller will no longer ever make an attacker flag
    { if ( !(ch->pcdata->clan && victim->pcdata->clan
      && ch->pcdata->clan == victim->pcdata->clan ) )  return; }
*/

   /*
    * Charm-o-rama.
    */
   if( IS_AFFECTED( ch, AFF_CHARM ) )
   {
      if( !ch->master )
      {
         bug( "%s: %s bad AFF_CHARM", __func__, IS_NPC( ch ) ? ch->short_descr : ch->name );
         affect_strip( ch, gsn_charm_person );
         xREMOVE_BIT( ch->affected_by, AFF_CHARM );
         return;
      }

      /*
       * Won't have charmed mobs fighting give the master an attacker 
       * flag.  The killer flag stays in, and I'll put something in 
       * do_murder. -Narn 
       */
      /*
       * xSET_BIT(ch->master->act, PLR_ATTACKER);
       */
      /*
       * stop_follower( ch ); 
       */
      return;
   }

   /*
    * NPC's are cool of course (as long as not charmed).
    * Hitting yourself is cool too (bleeding).
    * So is being immortal (Alander's idea).
    * And current killers stay as they are.
    */
   if( IS_NPC( ch )
       || ch == victim || ch->level >= LEVEL_IMMORTAL || xIS_SET( ch->act, PLR_ATTACKER ) || xIS_SET( ch->act, PLR_KILLER ) )
      return;

   xSET_BIT( ch->act, PLR_ATTACKER );
   save_char_obj( ch );
}

/*
 * Set position of a victim.
 */
void update_pos( CHAR_DATA * victim )
{
   if( !victim )
   {
      bug( "%s: null victim", __func__ );
      return;
   }

   if( victim->hit > 0 )
   {
      if( victim->position <= POS_STUNNED )
         victim->position = POS_STANDING;
      if( IS_AFFECTED( victim, AFF_PARALYSIS ) )
         victim->position = POS_STUNNED;
      return;
   }

   if( IS_NPC( victim ) || victim->hit <= -11 )
   {
      if( victim->mount )
      {
         act( AT_ACTION, "$n falls from $N.", victim, NULL, victim->mount, TO_ROOM );
         xREMOVE_BIT( victim->mount->act, ACT_MOUNTED );
         victim->mount = NULL;
      }
      victim->position = POS_DEAD;
      return;
   }

   if( victim->hit <= -6 )
      victim->position = POS_MORTAL;
   else if( victim->hit <= -3 )
      victim->position = POS_INCAP;
   else
      victim->position = POS_STUNNED;

   if( victim->position > POS_STUNNED && IS_AFFECTED( victim, AFF_PARALYSIS ) )
      victim->position = POS_STUNNED;

   if( victim->mount )
   {
      act( AT_ACTION, "$n falls unconscious from $N.", victim, NULL, victim->mount, TO_ROOM );
      xREMOVE_BIT( victim->mount->act, ACT_MOUNTED );
      victim->mount = NULL;
   }
}

/*
 * Start fights.
 */
void set_fighting( CHAR_DATA * ch, CHAR_DATA * victim )
{
   FIGHT_DATA *fight;

   if( ch->fighting )
   {
      bug( "%s: %s -> %s (already fighting %s)", __func__, ch->name, victim->name, ch->fighting->who->name );
      return;
   }

   if( IS_AFFECTED( ch, AFF_SLEEP ) )
      affect_strip( ch, gsn_sleep );

   /*
    * Limit attackers -Thoric 
    */
   if( victim->num_fighting > MAX_FIGHT )
   {
      send_to_char( "There are too many people fighting for you to join in.\r\n", ch );
      return;
   }
   
   /*
    * Reset focus when combat begins
    */
   reset_focus( ch );
   reset_focus( victim );

   CREATE( fight, FIGHT_DATA, 1 );
   fight->who = victim;
   fight->xp = ( int )( xp_compute( ch, victim ) * 0.85 );
   fight->align = align_compute( ch, victim );
   if( !IS_NPC( ch ) && IS_NPC( victim ) )
      fight->timeskilled = times_killed( ch, victim );
   ch->num_fighting = 1;
   ch->fighting = fight;
   /*
    * ch->position = POS_FIGHTING; 
    */
   if( IS_NPC( ch ) )
      ch->position = POS_FIGHTING;
   else
      switch ( ch->style )
      {
         case ( STYLE_EVASIVE ):
            ch->position = POS_EVASIVE;
            break;
         case ( STYLE_DEFENSIVE ):
            ch->position = POS_DEFENSIVE;
            break;
         case ( STYLE_AGGRESSIVE ):
            ch->position = POS_AGGRESSIVE;
            break;
         case ( STYLE_BERSERK ):
            ch->position = POS_BERSERK;
            break;
         default:
            ch->position = POS_FIGHTING;
      }
   victim->num_fighting++;
   if( victim->switched && IS_AFFECTED( victim->switched, AFF_POSSESS ) )
   {
      send_to_char( "You are disturbed!\r\n", victim->switched );
      do_return( victim->switched, "" );
   }
}

CHAR_DATA *who_fighting( CHAR_DATA * ch )
{
   if( !ch )
   {
      bug( "%sg: null ch", __func__ );
      return NULL;
   }
   if( !ch->fighting )
      return NULL;
   return ch->fighting->who;
}

void free_fight( CHAR_DATA * ch )
{
   if( !ch )
   {
      bug( "%s: null ch!", __func__ );
      return;
   }

   if( ch->fighting )
   {
      if( !char_died( ch->fighting->who ) )
         --ch->fighting->who->num_fighting;
      DISPOSE( ch->fighting );
   }

   ch->fighting = NULL;
   if( ch->mount )
      ch->position = POS_MOUNTED;
   else
      ch->position = POS_STANDING;

   /*
    * Berserk wears off after combat. -- Altrag 
    */
   if( IS_AFFECTED( ch, AFF_BERSERK ) )
   {
      affect_strip( ch, gsn_berserk );
      set_char_color( AT_WEAROFF, ch );
      send_to_char( skill_table[gsn_berserk]->msg_off, ch );
      send_to_char( "\r\n", ch );
   }
}

/*
 * Stop fights.
 */
void stop_fighting( CHAR_DATA * ch, bool fBoth )
{
   CHAR_DATA *fch;

   strip_grapple( ch );
   free_fight( ch );
   update_pos( ch );

   if( !fBoth )   /* major short cut here by Thoric */
      return;

   for( fch = first_char; fch; fch = fch->next )
   {
      if( who_fighting( fch ) == ch )
      {
         free_fight( fch );
         update_pos( fch );
      }
   }
}

/* Vnums for the various bodyparts */
int part_vnums[] = { 12,   /* Head */
   14,   /* arms */
   15,   /* legs */
   13,   /* heart */
   44,   /* brains */
   16,   /* guts */
   45,   /* hands */
   46,   /* feet */
   47,   /* fingers */
   48,   /* ear */
   49,   /* eye */
   50,   /* long_tongue */
   51,   /* eyestalks */
   52,   /* tentacles */
   53,   /* fins */
   54,   /* wings */
   55,   /* tail */
   56,   /* scales */
   59,   /* claws */
   87,   /* fangs */
   58,   /* horns */
   57,   /* tusks */
   55,   /* tailattack */
   85,   /* sharpscales */
   84,   /* beak */
   86,   /* haunches */
   83,   /* hooves */
   82,   /* paws */
   81,   /* forelegs */
   80,   /* feathers */
	79,	/* husk_shell */
   0  /* r2 */
};

/* Messages for flinging off the various bodyparts */
const char *part_messages[] = {
   "$n's severed head plops from its neck.",
   "$n's arm is sliced from $s dead body.",
   "$n's leg is sliced from $s dead body.",
   "$n's heart is torn from $s chest.",
   "$n's brains spill grotesquely from $s head.",
   "$n's guts spill grotesquely from $s torso.",
   "$n's hand is sliced from $s dead body.",
   "$n's foot is sliced from $s dead body.",
   "A finger is sliced from $n's dead body.",
   "$n's ear is sliced from $s dead body.",
   "$n's eye is gouged from its socket.",
   "$n's tongue is torn from $s mouth.",
   "An eyestalk is sliced from $n's dead body.",
   "A tentacle is severed from $n's dead body.",
   "A fin is sliced from $n's dead body.",
   "A wing is severed from $n's dead body.",
   "$n's tail is sliced from $s dead body.",
   "A scale falls from the body of $n.",
   "A claw is torn from $n's dead body.",
   "$n's fangs are torn from $s mouth.",
   "A horn is wrenched from the body of $n.",
   "$n's tusk is torn from $s dead body.",
   "$n's tail is sliced from $s dead body.",
   "A ridged scale falls from the body of $n.",
   "$n's beak is sliced from $s dead body.",
   "$n's haunches are sliced from $s dead body.",
   "A hoof is sliced from $n's dead body.",
   "A paw is sliced from $n's dead body.",
   "$n's foreleg is sliced from $s dead body.",
   "Some feathers fall from $n's dead body.",
	"$n's shell remains.",
   "r2 message."
};

/*
 * Improved Death_cry contributed by Diavolo.
 * Additional improvement by Thoric (and removal of turds... sheesh!)  
 * Support for additional bodyparts by Fireblade
 */
void death_cry( CHAR_DATA * ch )
{
   ROOM_INDEX_DATA *was_in_room;
   const char *msg;
   EXIT_DATA *pexit;
   int vnum, shift, cindex, i;

   if( !ch )
   {
      bug( "%s: null ch!", __func__ );
      return;
   }

   vnum = 0;
   msg = NULL;

   switch ( number_range( 0, 5 ) )
   {
      default:
         msg = "You hear $n's death cry.";
         break;
      case 0:
         msg = "$n screams furiously as $e falls to the ground in a heap!";
         break;
      case 1:
         msg = "$n hits the ground ... DEAD.";
         break;
      case 2:
         msg = "$n catches $s guts in $s hands as they pour through $s fatal wound!";
         break;
      case 3:
         msg = "$n splatters blood on your armor.";
         break;
      case 4:
         msg = "$n gasps $s last breath and blood spurts out of $s mouth and ears.";
         break;
      case 5:
         shift = number_range( 0, 31 );
         cindex = 1 << shift;

         for( i = 0; i < 32 && ch->xflags; i++ )
         {
            if( HAS_BODYPART( ch, cindex ) )
            {
               msg = part_messages[shift];
               vnum = part_vnums[shift];
               break;
            }
            else
            {
               shift = number_range( 0, 31 );
               cindex = 1 << shift;
            }
         }

         if( !msg )
            msg = "You hear $n's death cry.";
         break;
   }

   act( AT_CARNAGE, msg, ch, NULL, NULL, TO_ROOM );

   if( vnum )
   {
      char buf[MAX_STRING_LENGTH];
      OBJ_DATA *obj;
      const char *name;

      if( !get_obj_index( vnum ) )
      {
         bug( "%s: invalid vnum", __func__ );
         return;
      }

      name = IS_NPC( ch ) ? ch->short_descr : ch->name;
      obj = create_object( get_obj_index( vnum ), 0 );
      obj->timer = number_range( 4, 7 );
      if( IS_AFFECTED( ch, AFF_POISON ) )
         obj->value[3] = 10;

      snprintf( buf, MAX_STRING_LENGTH, obj->short_descr, name );
      STRFREE( obj->short_descr );
      obj->short_descr = STRALLOC( buf );

      snprintf( buf, MAX_STRING_LENGTH, obj->description, name );
      STRFREE( obj->description );
      obj->description = STRALLOC( buf );

      obj = obj_to_room( obj, ch->in_room );
   }

   if( IS_NPC( ch ) )
      msg = "You hear something's death cry.";
   else
      msg = "You hear someone's death cry.";

   was_in_room = ch->in_room;
   for( pexit = was_in_room->first_exit; pexit; pexit = pexit->next )
   {
      if( pexit->to_room && pexit->to_room != was_in_room )
      {
         ch->in_room = pexit->to_room;
         act( AT_CARNAGE, msg, ch, NULL, NULL, TO_ROOM );
      }
   }
   ch->in_room = was_in_room;
}

OBJ_DATA *raw_kill( CHAR_DATA * ch, CHAR_DATA * victim )
{
   OBJ_DATA *corpse_to_return = NULL;

   if( !victim )
   {
      bug( "%s: null victim!", __func__ );
      return NULL;
   }

   /*
    * backup in case hp goes below 1 
    */
   if( NOT_AUTHED( victim ) )
   {
      bug( "%s: killing unauthed", __func__ );
      return NULL;
   }

   stop_fighting( victim, TRUE );

   /*
    * Take care of morphed characters 
    */
   if( victim->morph )
   {
      do_unmorph_char( victim );
      return raw_kill( ch, victim );
   }

   mprog_death_trigger( ch, victim );
   if( char_died( victim ) )
      return NULL;

   /*
    * death_cry( victim ); 
    */

   rprog_death_trigger( victim );
   if( char_died( victim ) )
      return NULL;
  
   if( !IS_NPC( ch ) && IS_NPC( victim ) )
   {
	   group_gain( ch, victim );
		if( ch && !IS_NPC(ch) && IS_BIO_ANDROID(ch) && ch != victim )
	{
		/* Calculate absorption power gain */
		int power_gain = 0;
		int absorption_debt = 0;
		
		/* Base absorption power */
		if( IS_NPC(victim) )
			power_gain = get_power_level(victim) * 0.05; /* 5% from NPCs */
		else
			power_gain = get_power_level(victim) * 0.10; /* 10% from players */
		
		/* Add back the PL they missed during combat */
		if( ch->pcdata && ch->pcdata->absorption_debt > 0 )
		{
			absorption_debt = ch->pcdata->absorption_debt;
			ch->pcdata->absorption_debt = 0;  /* Reset debt */
		}
		
		if( power_gain > 0 || absorption_debt > 0 )
		{
			int total_gain = power_gain + absorption_debt;
			
			/* Absorption messages */
			ch_printf( ch, "&RYou absorb %d power from %s's essence!", total_gain, victim->name );
			if( absorption_debt > 0 )
					ch_printf( ch, " (&Y%d&R from complete absorption bonus)", absorption_debt );
			ch_printf( ch, "&x\r\n" );
			
			act( AT_BLOOD, "$n's body glows as $e absorbs energy from $N's defeated form!", ch, NULL, victim, TO_ROOM );
			
			/* Apply total power gain */
			gain_pl( ch, total_gain, true );
			
			/* Track absorption count */
			if( ch->pcdata )
					ch->pcdata->absorbed_count++;
			
			/* 25% chance to steal a skill from players */
			if( !IS_NPC(victim) && number_percent() <= 25 )
			{
					/* Try to steal a random skill */
					int skill_num = number_range(0, MAX_SKILL-1);
					if( victim->pcdata->skills[skill_num].value_tenths > 0 && ch->pcdata->skills[skill_num].value_tenths == 0 )
{
						/* Bio-android learns the skill */
						ch->pcdata->skills[skill_num].value_tenths = victim->pcdata->skills[skill_num].value_tenths;
						ch_printf( ch, "&GYou absorb knowledge of %s from %s!&x\r\n", 
									skill_table[skill_num]->name, victim->name );
						act( AT_MAGIC, "$n's eyes flash as $e absorbs $N's knowledge!", ch, NULL, victim, TO_ROOM );
					}
			}
		}
	}
   }
	
	check_android_components( ch, victim );
	corpse_to_return = make_corpse( victim, ch );
   if( victim->in_room->sector_type == SECT_OCEANFLOOR
       || victim->in_room->sector_type == SECT_UNDERWATER
       || victim->in_room->sector_type == SECT_WATER_SWIM || victim->in_room->sector_type == SECT_WATER_NOSWIM )
      act( AT_BLOOD, "$n's blood slowly clouds the surrounding water.", victim, NULL, NULL, TO_ROOM );
   else if( victim->in_room->sector_type == SECT_AIR )
      act( AT_BLOOD, "$n's blood sprays wildly through the air.", victim, NULL, NULL, TO_ROOM );
   else
      make_blood( victim );

   if( IS_NPC( victim ) )
   {
      victim->pIndexData->killed++;
      extract_char( victim, TRUE );
      victim = NULL;
      return corpse_to_return;
   }

   set_char_color( AT_DIEMSG, victim );
   if( victim->pcdata->mdeaths + victim->pcdata->pdeaths < 3 )
      do_help( victim, "new_death" );
   else
      do_help( victim, "_DIEMSG_" );

   extract_char( victim, FALSE );
   if( !victim )
   {
      bug( "%s: oops! extract_char destroyed pc char", __func__ );
      return NULL;
   }
   while( victim->first_affect )
      affect_remove( victim, victim->first_affect );
   victim->affected_by = race_table[victim->race]->affected;
   victim->resistant = 0;
   victim->susceptible = 0;
   victim->immune = 0;
   victim->carry_weight = 0;
   victim->armor = 100;
   victim->armor += race_table[victim->race]->ac_plus;
   victim->attacks = race_table[victim->race]->attacks;
   victim->defenses = race_table[victim->race]->defenses;
   victim->mod_str = 0;
   victim->mod_dex = 0;
   victim->mod_wis = 0;
   victim->mod_int = 0;
   victim->mod_con = 0;
   victim->mod_spr = 0;
   victim->mod_lck = 0;
   victim->damroll = 0;
   victim->hitroll = 0;
   victim->mental_state = -10;
   victim->alignment = URANGE( -1000, victim->alignment, 1000 );
/*  victim->alignment		= race_table[victim->race]->alignment;
-- switched lines just for now to prevent mortals from building up
days of bellyaching about their angelic or satanic humans becoming
neutral when they die given the difficulting of changing align */

   victim->saving_poison_death = race_table[victim->race]->saving_poison_death;
   victim->saving_wand = race_table[victim->race]->saving_wand;
   victim->saving_para_petri = race_table[victim->race]->saving_para_petri;
   victim->saving_breath = race_table[victim->race]->saving_breath;
   victim->saving_spell_staff = race_table[victim->race]->saving_spell_staff;
   victim->position = POS_RESTING;
   victim->hit = UMAX( 1, victim->hit );
   /*
    * Shut down some of those naked spammer killers - Blodkai 
    */
   if( victim->level < LEVEL_AVATAR )
      victim->mana = UMAX( 1, victim->mana );
   else
      victim->mana = 1;
   victim->move = UMAX( 1, victim->move );

   /*
    * Pardon crimes...                -Thoric
    */
   if( xIS_SET( victim->act, PLR_KILLER ) )
   {
      xREMOVE_BIT( victim->act, PLR_KILLER );
      send_to_char( "The gods have pardoned you for your murderous acts.\r\n", victim );
   }
   if( xIS_SET( victim->act, PLR_THIEF ) )
   {
      xREMOVE_BIT( victim->act, PLR_THIEF );
      send_to_char( "The gods have pardoned you for your thievery.\r\n", victim );
   }
   if( IS_VAMPIRE( victim ) )
      victim->pcdata->condition[COND_BLOODTHIRST] = ( victim->level / 2 );

   if( IS_SET( sysdata.save_flags, SV_DEATH ) )
      save_char_obj( victim );
   return corpse_to_return;
}
/* SET FOR DELETION
void group_gain( CHAR_DATA * ch, CHAR_DATA * victim )
{
   CHAR_DATA *gch, *gch_next;
   CHAR_DATA *lch;
   int xp;
   int members;

   
    * Monsters don't get kill xp's or alignment changes.
    * Dying of mortal wounds or poison doesn't give xp to anyone!
    
   if( IS_NPC( ch ) || victim == ch )
      return;

   members = 0;
   for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
   {
      if( is_same_group( gch, ch ) )
         members++;
   }

   if( members == 0 )
   {
      bug( "%s: members %d", __func__, members );
      members = 1;
   }

   lch = ch->leader ? ch->leader : ch;

   for( gch = ch->in_room->first_person; gch; gch = gch_next )
   {
      OBJ_DATA *obj;
      OBJ_DATA *obj_next;

      gch_next = gch->next_in_room;

      if( !is_same_group( gch, ch ) )
         continue;

      if( gch->level - lch->level > 8 )
      {
         send_to_char( "You are too high for this group.\r\n", gch );
         continue;
      }

      if( gch->level - lch->level < -8 )
      {
         send_to_char( "You are too low for this group.\r\n", gch );
         continue;
      }

      xp = ( int )( xp_compute( gch, victim ) * 0.1765 ) / members;
      if( !gch->fighting )
         xp /= 2;
      gch->alignment = align_compute( gch, victim );
      ch_printf( gch, "You receive %d experience points.\r\n", xp );
      gain_exp( gch, xp );

      for( obj = gch->first_carrying; obj; obj = obj_next )
      {
         obj_next = obj->next_content;
         if( obj->wear_loc == WEAR_NONE )
            continue;

         if( ( IS_OBJ_STAT( obj, ITEM_ANTI_EVIL ) && IS_EVIL( gch ) ) ||
             ( IS_OBJ_STAT( obj, ITEM_ANTI_GOOD ) && IS_GOOD( gch ) ) ||
             ( IS_OBJ_STAT( obj, ITEM_ANTI_NEUTRAL ) && IS_NEUTRAL( gch ) ) )
         {
            act( AT_MAGIC, "You are zapped by $p.", gch, obj, NULL, TO_CHAR );
            act( AT_MAGIC, "$n is zapped by $p.", gch, obj, NULL, TO_ROOM );

            obj_from_char( obj );
            if( in_arena( ch ) )
               obj = obj_to_char( obj, ch );
            else
               obj = obj_to_room( obj, ch->in_room );
            oprog_zap_trigger( gch, obj );   * mudprogs 
            if( char_died( gch ) )
               break;
         }
      }
   }
}
 */
int align_compute( CHAR_DATA * gch, CHAR_DATA * victim )
{
   int align, newalign, divalign;

   align = gch->alignment - victim->alignment;

   /*
    * slowed movement in good & evil ranges by a factor of 5, h 
    * Added divalign to keep neutral chars shifting faster -- Blodkai 
    * This is obviously gonna take a lot more thought 
    */
   if( gch->alignment > -350 && gch->alignment < 350 )
      divalign = 4;
   else
      divalign = 20;

   if( align > 500 )
      newalign = UMIN( gch->alignment + ( align - 500 ) / divalign, 1000 );
   else if( align < -500 )
      newalign = UMAX( gch->alignment + ( align + 500 ) / divalign, -1000 );
   else
      newalign = gch->alignment - ( int )( gch->alignment / divalign );

   return newalign;
}

/*
 * Calculate how much XP gch should gain for killing victim
 * Lots of redesigning for new exp system by Thoric
 */
int xp_compute( CHAR_DATA * gch, CHAR_DATA * victim )
{
   int align;
   int xp;
   int xp_ratio;
   int gchlev = gch->level;

   xp = ( get_exp_worth( victim ) * URANGE( 0, ( victim->level - gchlev ) + 10, 13 ) ) / 10;
   align = gch->alignment - victim->alignment;

   /*
    * bonus for attacking opposite alignment 
    */
   if( align > 990 || align < -990 )
      xp = ( xp * 5 ) >> 2;
   else
      /*
       * penalty for good attacking same alignment 
       */
   if( gch->alignment > 300 && align < 250 )
      xp = ( xp * 3 ) >> 2;

   xp = number_range( ( xp * 3 ) >> 2, ( xp * 5 ) >> 2 );

   /*
    * get 1/4 exp for players               -Thoric 
    */
   if( !IS_NPC( victim ) )
      xp /= 4;
   else
      /*
       * reduce exp for killing the same mob repeatedly    -Thoric 
       */
   if( !IS_NPC( gch ) )
   {
      int times = times_killed( gch, victim );

      if( times >= 20 )
         xp = 0;
      else if( times )
      {
         xp = ( xp * ( 20 - times ) ) / 20;
         if( times > 15 )
            xp /= 3;
         else if( times > 10 )
            xp >>= 1;
      }
   }

   /*
    * semi-intelligent experienced player vs. novice player xp gain
    * "bell curve"ish xp mod by Thoric
    * based on time played vs. level
    */
   if( !IS_NPC( gch ) && gchlev > 5 )
   {
      xp_ratio = ( int )gch->played / gchlev;
      if( xp_ratio > 20000 )  /* 5/4 */
         xp = ( xp * 5 ) >> 2;
      else if( xp_ratio > 16000 )   /* 3/4 */
         xp = ( xp * 3 ) >> 2;
      else if( xp_ratio > 10000 )   /* 1/2 */
         xp >>= 1;
      else if( xp_ratio > 5000 ) /* 1/4 */
         xp >>= 2;
      else if( xp_ratio > 3500 ) /* 1/8 */
         xp >>= 3;
      else if( xp_ratio > 2000 ) /* 1/16 */
         xp >>= 4;
      else
         xp >>= 5; // 1/32 - Bugfix for Issue #15 @ GitHub
   }

   /*
    * Level based experience gain cap.  Cannot get more experience for
    * a kill than the amount for your current experience level   -Thoric
    */
   return URANGE( 0, xp, exp_level( gch, gchlev + 1 ) - exp_level( gch, gchlev ) );
}

/*
 * Revamped by Thoric to be more realistic
 * Added code to produce different messages based on weapon type - FB
 * Added better bug message so you can track down the bad dt's -Shaddai
 */
void new_dam_message( CHAR_DATA * ch, CHAR_DATA * victim, int dam, unsigned int dt, OBJ_DATA * obj, long long pl_gained )
{
   struct skill_type *skill = NULL;
   ROOM_INDEX_DATA *was_in_room;
   int original_dam = dam;

   if( ch->in_room != victim->in_room )
   {
      was_in_room = ch->in_room;
      char_from_room( ch );
      char_to_room( ch, victim->in_room );
   }
   else
      was_in_room = NULL;

   /* Check for skill messages FIRST and handle them exclusively */
   if( dt >= 0 && dt < ( unsigned int )num_skills )
   {
      skill = skill_table[dt];
      if( skill )
      {
         if( dam == 0 )
         {
            bool found = FALSE;

            if( skill->miss_char && skill->miss_char[0] != '\0' )
            {
               act( AT_HIT, skill->miss_char, ch, NULL, victim, TO_CHAR );
               found = TRUE;
            }
            if( skill->miss_vict && skill->miss_vict[0] != '\0' )
            {
               /* Add damage tag to skill miss messages */
               char modified_miss_msg[MAX_STRING_LENGTH];
               snprintf( modified_miss_msg, MAX_STRING_LENGTH, "%s &w[&r0 dmg&w]&x", skill->miss_vict );
               act( AT_HITME, modified_miss_msg, ch, NULL, victim, TO_VICT );
               found = TRUE;
            }
            if( skill->miss_room && skill->miss_room[0] != '\0' )
            {
               if( str_cmp( skill->miss_room, "supress" ) )
                  act( AT_ACTION, skill->miss_room, ch, NULL, victim, TO_NOTVICT );
               found = TRUE;
            }
            if( found ) /* miss message already sent */
            {
               if( was_in_room )
               {
                  char_from_room( ch );
                  char_to_room( ch, was_in_room );
               }
               return;
            }
         }
         else
         {
            bool sent_skill_message = FALSE;
            char damage_str[32];
            char damage_color[8];
            
            /* Format damage with commas for skill messages */
            if( dam >= 1000000 )
               snprintf( damage_str, 32, "%d,%03d,%03d", 
                         dam / 1000000, (dam / 1000) % 1000, dam % 1000 );
            else if( dam >= 1000 )
               snprintf( damage_str, 32, "%d,%03d", dam / 1000, dam % 1000 );
            else
               snprintf( damage_str, 32, "%d", dam );
            
            /* Determine RIS color */
            if( original_dam > 0 && dam == -1 )
               strlcpy( damage_color, "&B", 8 );  /* Blue - immune */
            else if( original_dam > 0 && dam < original_dam )
               strlcpy( damage_color, "&C", 8 );  /* Yellow - resistant */
            else if( original_dam > 0 && dam > original_dam )
               strlcpy( damage_color, "&P", 8 );  /* Magenta - susceptible */
            else
               strlcpy( damage_color, "&R", 8 );  /* Red - normal */
            
            if( skill->hit_char && skill->hit_char[0] != '\0' )
            {
               char modified_char_msg[MAX_STRING_LENGTH];
               
               if( dam == -1 )
               {
                  snprintf( modified_char_msg, MAX_STRING_LENGTH, "%s &w[&BIMMUNE&w]&x", skill->hit_char );
               }
               else
               {
                  snprintf( modified_char_msg, MAX_STRING_LENGTH, "%s &w[%s%s dmg&w]&x",
                           skill->hit_char, damage_color, damage_str );
               }
               
               act( AT_HIT, modified_char_msg, ch, NULL, victim, TO_CHAR );
               sent_skill_message = TRUE;
            }
            if( skill->hit_vict && skill->hit_vict[0] != '\0' )
            {
               char modified_vict_msg[MAX_STRING_LENGTH];
               
               if( dam == -1 )
               {
                  snprintf( modified_vict_msg, MAX_STRING_LENGTH, "%s &w[&BIMMUNE&w]&x", skill->hit_vict );
               }
               else
               {
                  snprintf( modified_vict_msg, MAX_STRING_LENGTH, "%s &w[%s%s dmg&w]&x",
                           skill->hit_vict, damage_color, damage_str );
               }
               
               act( AT_HITME, modified_vict_msg, ch, NULL, victim, TO_VICT );
               sent_skill_message = TRUE;
            }
            if( skill->hit_room && skill->hit_room[0] != '\0' )
            {
               if( str_cmp( skill->hit_room, "supress" ) )
                  act( AT_ACTION, skill->hit_room, ch, NULL, victim, TO_NOTVICT );
               sent_skill_message = TRUE;
            }
            
            if( sent_skill_message )
            {
               if( was_in_room )
               {
                  char_from_room( ch );
                  char_to_room( ch, was_in_room );
               }
               return;
            }
         }
      }
   }

   /* Enhanced combat messages are now the default for everyone */
   enhanced_dam_message( ch, victim, dam, dt, obj, pl_gained );

   if( was_in_room )
   {
      char_from_room( ch );
      char_to_room( ch, was_in_room );
   }
}

#ifndef dam_message
void dam_message( CHAR_DATA * ch, CHAR_DATA * victim, int dam, int dt )
{
   new_dam_message( ch, victim, dam, dt , NULL, 0 );
}
#endif



void do_kill( CHAR_DATA* ch, const char* argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      send_to_char( "Kill whom?\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( IS_NPC( victim ) && victim->morph )
   {
      send_to_char( "This creature appears strange to you.  Look upon it more closely before attempting to kill it.", ch );
      return;
   }

   if( !IS_NPC( victim ) )
   {
      if( !xIS_SET( victim->act, PLR_KILLER ) && !xIS_SET( victim->act, PLR_THIEF ) )
      {
         send_to_char( "You must MURDER a player.\r\n", ch );
         return;
      }
   }

   if( victim == ch )
   {
      send_to_char( "You hit yourself.  Ouch!\r\n", ch );
      multi_hit( ch, ch, TYPE_UNDEFINED );
      return;
   }

   if( is_safe( ch, victim, TRUE ) )
      return;

   if( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
   {
      act( AT_PLAIN, "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
      return;
   }

   /* Handle if they are already fighting */
   if( ch->fighting )
   {
      /* If already fighting the same target, continue attacking */
      if( ch->fighting->who == victim )
      {
         send_to_char( "You do the best you can!\r\n", ch );
         return;
      }
      /* If fighting a different target, inform but don't break current combat */
      send_to_char( "You are already fighting someone else!\r\n", ch );
      return;
   }

   WAIT_STATE( ch, 1 * PULSE_VIOLENCE );
   check_attacker( ch, victim );
   
   
   set_fighting( ch, victim );
   
   multi_hit( ch, victim, TYPE_UNDEFINED );
}


void do_murder( CHAR_DATA* ch, const char* argument )
{
   char buf[MAX_STRING_LENGTH];
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      send_to_char( "Murder whom?\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "Suicide is a mortal sin.\r\n", ch );
      return;
   }

   if( is_safe( ch, victim, TRUE ) )
      return;

   if( IS_AFFECTED( ch, AFF_CHARM ) )
   {
      if( ch->master == victim )
      {
         act( AT_PLAIN, "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
         return;
      }
      else
      {
         if( ch->master )
            xSET_BIT( ch->master->act, PLR_ATTACKER );
      }
   }

   /* Handle if they are already fighting */
   if( ch->fighting )
   {
      /* If already fighting the same target, continue attacking */
      if( ch->fighting->who == victim )
      {
         send_to_char( "You do the best you can!\r\n", ch );
         return;
      }
      /* If fighting a different target, inform but don't break current combat */
      send_to_char( "You are already fighting someone else!\r\n", ch );
      return;
   }

   if( !IS_NPC( victim ) && xIS_SET( ch->act, PLR_NICE ) )
   {
      send_to_char( "You feel too nice to do that!\r\n", ch );
      return;
   }

   if( !IS_NPC( victim ) )
   {
      log_printf_plus( LOG_NORMAL, ch->level, "%s: murder %s.", ch->name, victim->name );
   }

   WAIT_STATE( ch, 1 * PULSE_VIOLENCE );
   snprintf( buf, MAX_STRING_LENGTH, "Help!  I am being attacked by %s!", IS_NPC( ch ) ? ch->short_descr : ch->name );
   if( IS_PKILL( victim ) )
      do_wartalk( victim, buf );
   else
      do_yell( victim, buf );
   check_illegal_pk( ch, victim );
   check_attacker( ch, victim );
   
   set_fighting( ch, victim );
   
   multi_hit( ch, victim, TYPE_UNDEFINED );
}

/*
 * Check to see if the player is in an "Arena".
 */
bool in_arena( CHAR_DATA * ch )
{
   if( xIS_SET( ch->in_room->room_flags, ROOM_ARENA ) )
      return TRUE;
   if( IS_SET( ch->in_room->area->flags, AFLAG_FREEKILL ) )
      return TRUE;
   if( ch->in_room->vnum >= 29 && ch->in_room->vnum <= 43 )
      return TRUE;
   if( !str_cmp( ch->in_room->area->filename, "arena.are" ) )
      return TRUE;

   return FALSE;
}

bool check_illegal_pk( CHAR_DATA * ch, CHAR_DATA * victim )
{
   char buf[MAX_INPUT_LENGTH];
   char buf2[MAX_INPUT_LENGTH];
   char log_buf[MAX_STRING_LENGTH];

   if( !IS_NPC( victim ) && !IS_NPC( ch ) )
   {
      if( ( !IS_SET( victim->pcdata->flags, PCFLAG_DEADLY )
            || ch->level - victim->level > 10
            || !IS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) )
          && !in_arena( ch ) && ch != victim && !( IS_IMMORTAL( ch ) && IS_IMMORTAL( victim ) ) )
      {
         if( IS_NPC( ch ) )
            snprintf( buf, MAX_INPUT_LENGTH, " (%s)", ch->name );
         if( IS_NPC( victim ) )
            snprintf( buf2, MAX_INPUT_LENGTH, " (%s)", victim->name );

         snprintf( log_buf, MAX_STRING_LENGTH, "&p%s on %s%s in &W***&rILLEGAL PKILL&W*** &pattempt at %d",
                   ( lastplayercmd ),
                   ( IS_NPC( victim ) ? victim->short_descr : victim->name ),
                   ( IS_NPC( victim ) ? buf2 : "" ), victim->in_room->vnum );
         last_pkroom = victim->in_room->vnum;
         log_string( log_buf );
         to_channel( log_buf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );
         return TRUE;
      }
   }
   return FALSE;
}

void do_flee( CHAR_DATA* ch, const char* argument )
{
   ROOM_INDEX_DATA *was_in;
   ROOM_INDEX_DATA *now_in;
   char buf[MAX_STRING_LENGTH];
   int attempt, los;
   short door;
   EXIT_DATA *pexit;

   if( !who_fighting( ch ) )
   {
      if( ch->position == POS_FIGHTING
          || ch->position == POS_EVASIVE
          || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE || ch->position == POS_BERSERK )
      {
         if( ch->mount )
            ch->position = POS_MOUNTED;
         else
            ch->position = POS_STANDING;
      }
      send_to_char( "You aren't fighting anyone.\r\n", ch );
      return;
   }

   if( IS_AFFECTED( ch, AFF_BERSERK ) )
   {
      send_to_char( "Flee while berserking?  You aren't thinking very clearly...\r\n", ch );
      return;
   }

   if( IS_AFFECTED( ch, AFF_GRAPPLE ) )
   {
      send_to_char( "You're too wrapped up to flee!\r\n", ch );
      return;
   }

   if( ch->move <= 0 )
   {
      send_to_char( "You're too exhausted to flee from combat!\r\n", ch );
      return;
   }

   /*
    * No fleeing while more aggressive than standard or hurt. - Haus 
    */
   if( !IS_NPC( ch ) && ch->position < POS_FIGHTING )
   {
      send_to_char( "You can't flee in an aggressive stance...\r\n", ch );
      return;
   }

   if( IS_NPC( ch ) && ch->position <= POS_SLEEPING )
      return;

   was_in = ch->in_room;
   for( attempt = 0; attempt < 8; attempt++ )
   {
      door = number_door(  );
      if( ( pexit = get_exit( was_in, door ) ) == NULL
          || !pexit->to_room
          || IS_SET( pexit->exit_info, EX_NOFLEE )
          || ( IS_PKILL( ch )
               && xIS_SET( pexit->to_room->room_flags, ROOM_DEATH ) )
          || ( IS_SET( pexit->exit_info, EX_CLOSED )
               && !IS_AFFECTED( ch, AFF_PASS_DOOR ) )
          || ( IS_NPC( ch ) && xIS_SET( pexit->to_room->room_flags, ROOM_NO_MOB ) ) )
         continue;
      affect_strip( ch, gsn_sneak );
      xREMOVE_BIT( ch->affected_by, AFF_SNEAK );
      if( ch->mount && ch->mount->fighting )
         stop_fighting( ch->mount, TRUE );
      move_char( ch, pexit, 0 );
      if( ( now_in = ch->in_room ) == was_in )
         continue;
      ch->in_room = was_in;
      act( AT_FLEE, "$n flees head over heels!", ch, NULL, NULL, TO_ROOM );
      ch->in_room = now_in;
      act( AT_FLEE, "$n glances around for signs of pursuit.", ch, NULL, NULL, TO_ROOM );
      if( !IS_NPC( ch ) )
      {
         CHAR_DATA *wf = who_fighting( ch );

         act( AT_FLEE, "You flee head over heels from combat!", ch, NULL, NULL, TO_CHAR );
         los = ( int )( ( exp_level( ch, ch->level + 1 ) - exp_level( ch, ch->level ) ) * 0.2 );
         if( ch->level < LEVEL_AVATAR )
         {
            if( !IS_PKILL( ch ) )
            {
               if( ch->level > 1 )
               {
                  snprintf( buf, MAX_STRING_LENGTH, "Curse the gods, you've lost %d power level!", los );
                  act( AT_FLEE, buf, ch, NULL, NULL, TO_CHAR );
                  gain_pl( ch, 0 - los, true );
               }
            }
         }

         if( wf && ch->pcdata->deity )
         {
            int level_ratio = URANGE( 1, wf->level / ch->level, MAX_LEVEL );

            if( wf && wf->race == ch->pcdata->deity->npcrace )
               adjust_favor( ch, 1, level_ratio );
            else if( wf && wf->race == ch->pcdata->deity->npcfoe )
               adjust_favor( ch, 16, level_ratio );
            else
               adjust_favor( ch, 0, level_ratio );
         }
      }
      stop_fighting( ch, TRUE );
      return;
   }

   los = ( int )( ( exp_level( ch, ch->level + 1 ) - exp_level( ch, ch->level ) ) * 0.1 );
   act( AT_FLEE, "You attempt to flee from combat but can't escape!", ch, NULL, NULL, TO_CHAR );
   if( ch->level < LEVEL_AVATAR && number_bits( 3 ) == 1 )
   {
      if( !IS_PKILL( ch ) )
      {
         if( ch->level > 1 )
         {
            snprintf( buf, MAX_STRING_LENGTH, "Curse the gods, you've lost %d experience!\n\r", los );
            act( AT_FLEE, buf, ch, NULL, NULL, TO_CHAR );
            gain_pl( ch, 0 - los, true );
         }
      }
   }
}

void do_sla( CHAR_DATA* ch, const char* argument )
{
   send_to_char( "If you want to SLAY, spell it out.\r\n", ch );
}

void do_slay( CHAR_DATA *ch, const char *argument )
{
   CHAR_DATA *victim;
   char arg[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];

   argument = one_argument( argument, arg );
   one_argument( argument, arg2 );

   /* 'slay list' shows builtin + custom types */
   if( !str_cmp( arg, "list" ) || !str_cmp( arg2, "list" ) )
   {
      send_to_char( "Built-in slay types:\r\n", ch );
      send_to_char( "  immolate shatter demon pounce slit dog\r\n", ch );
      custom_slay_show_list( ch );
      send_to_char( "Usage: slay <victim> [type]\r\n", ch );
      return;
   }

   if( arg[0] == '\0' )
   {
      send_to_char( "Slay whom?\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( ch == victim )
   {
      send_to_char( "Suicide is a mortal sin.\r\n", ch );
      return;
   }

   if( !IS_NPC( victim ) && get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\r\n", ch );
      return;
   }

   /* built-ins */
   if( !str_cmp( arg2, "immolate" ) )
   {
      act( AT_FIRE, "Your fireball turns $N into a blazing inferno.", ch, NULL, victim, TO_CHAR );
      act( AT_FIRE, "$n releases a searing fireball in your direction.", ch, NULL, victim, TO_VICT );
      act( AT_FIRE, "$n points at $N, who bursts into a flaming inferno.", ch, NULL, victim, TO_NOTVICT );
   }
   else if( !str_cmp( arg2, "shatter" ) )
   {
      act( AT_LBLUE, "You freeze $N with a glance and shatter the frozen corpse into tiny shards.", ch, NULL, victim, TO_CHAR );
      act( AT_LBLUE, "$n freezes you with a glance and shatters your frozen body into tiny shards.", ch, NULL, victim, TO_VICT );
      act( AT_LBLUE, "$n freezes $N with a glance and shatters the frozen body into tiny shards.", ch, NULL, victim, TO_NOTVICT );
   }
   else if( !str_cmp( arg2, "demon" ) )
   {
      act( AT_IMMORT, "You gesture, and a slavering demon appears.  With a horrible grin, the", ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "foul creature turns on $N, who screams in panic before being eaten alive.", ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "$n gestures, and a slavering demon appears.  The foul creature turns on", ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "you with a horrible grin.   You scream in panic before being eaten alive.", ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "$n gestures, and a slavering demon appears.  With a horrible grin, the", ch, NULL, victim, TO_NOTVICT );
      act( AT_IMMORT, "foul creature turns on $N, who screams in panic before being eaten alive.", ch, NULL, victim, TO_NOTVICT );
   }
   else if( !str_cmp( arg2, "pounce" ) )
   {
      act( AT_BLOOD, "Leaping upon $N with bared fangs, you tear open $S throat and toss the corpse to the ground...", ch, NULL, victim, TO_CHAR );
      act( AT_BLOOD, "In a heartbeat, $n rips $s fangs through your throat!  Your blood sprays and pours to the ground as your life ends...", ch, NULL, victim, TO_VICT );
      act( AT_BLOOD, "Leaping suddenly, $n sinks $s fangs into $N's throat.  As blood sprays and gushes to the ground, $n tosses $N's dying body away.", ch, NULL, victim, TO_NOTVICT );
   }
   else if( !str_cmp( arg2, "slit" ) )
   {
      act( AT_BLOOD, "You calmly slit $N's throat.", ch, NULL, victim, TO_CHAR );
      act( AT_BLOOD, "$n reaches out with a clawed finger and calmly slits your throat.", ch, NULL, victim, TO_VICT );
      act( AT_BLOOD, "$n calmly slits $N's throat.", ch, NULL, victim, TO_NOTVICT );
   }
   else if( !str_cmp( arg2, "dog" ) )
   {
      act( AT_BLOOD, "You order your dogs to rip $N to shreds.", ch, NULL, victim, TO_CHAR );
      act( AT_BLOOD, "$n orders $s dogs to rip you apart.", ch, NULL, victim, TO_VICT );
      act( AT_BLOOD, "$n orders $s dogs to rip $N to shreds.", ch, NULL, victim, TO_NOTVICT );
   }
   /* custom slay fallback – keep this inside the same if/else chain */
   else if( arg2[0] != '\0' && apply_custom_slay( ch, victim, arg2 ) )
   {
      return; /* custom handled and killed the victim */
   }
   else
   {
      act( AT_IMMORT, "You slay $N in cold blood!", ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "$n slays you in cold blood!", ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "$n slays $N in cold blood!", ch, NULL, victim, TO_NOTVICT );
   }

   set_cur_char( victim );
   raw_kill( ch, victim );
}
