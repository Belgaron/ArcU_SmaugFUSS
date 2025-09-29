/* ===============================================================
 * racial_transformations.c - Complete System with Spell Functions
 * All transformation logic and spell functions in one place
 * =============================================================== */

#include "mud.h"

/* Configuration structure for racial transformations */
typedef struct racial_trans_def {
   int race;         /* RACE_* constant */
   int morph_vnum;   /* vnum from your morph file */
   const char *skill_name;  /* skill name to look up */
   int skill_sn;     /* skill number (set at runtime) */
   int hp_pct;       /* hp% threshold to start attempting (e.g., 30) */
   float min_ratio;  /* opponent_pl / your_pl must be >= this (e.g., 0.60) */
   int base_chance;  /* base % chance per damage instance */
   int desperation_bonus; /* additional % chance per HP% below threshold */
   const char *hint_msg;     /* Race-specific hint message */
   const char *unlock_msg;   /* Race-specific unlock message */
} RACIAL_TRANS_DEF;

/* Race transformation definitions */
static RACIAL_TRANS_DEF racial_trans[] = {
   {
      RACE_HUMAN,
      25001,  /* morph vnum for human_ascended */
      "ascended form",  /* skill name */
      -1,     /* skill_sn (set at runtime) */
      30,     /* 30% HP threshold */
      0.60f,  /* Enemy must be 60% of player's PL */
      2,      /* 2% base chance per damage */
      1,      /* +1% per HP% below threshold */
      "&CYour human spirit blazes briefly, yearning to transcend its limits.&D",
      "&WYour mortality shatters as divine potential awakens within!"
   },
   {
      RACE_SPIRITBORN,
      25002,  /* morph vnum for spiritborn_shikai */
      "shikai",  /* skill name */
      -1,     /* skill_sn (set at runtime) */
      25,     /* 25% HP threshold - more sensitive */
      0.65f,  /* Enemy must be 65% of player's PL */
      1,      /* 1% base chance per damage */
      2,      /* +2% per HP% below threshold */
      "&MThe spirits whisper urgently, their voices growing stronger.&D",
      "&MThe spirit realm tears open as ancestral power floods your being!"
   },
   {
      RACE_HOLLOWBORN,
      25003,  /* morph vnum for hollowborn_rage */
      "hollow rage",  /* skill name */
      -1,     /* skill_sn (set at runtime) */
      35,     /* 35% HP threshold */
      0.55f,  /* Enemy must be 55% of player's PL */
      3,      /* 3% base chance per damage */
      1,      /* +1% per HP% below threshold */
      "&rThe emptiness within you hungers, clawing toward the surface.&D",
      "&rYour hollow core explodes outward, consuming light and hope!"
   },
   {
      RACE_LYCAN,
      25004,  /* morph vnum for lycan_primal */
      "primal form",  /* skill name */
      -1,     /* skill_sn (set at runtime) */
      40,     /* 40% HP threshold */
      0.70f,  /* Enemy must be 70% of player's PL */
      2,      /* 2% base chance per damage */
      2,      /* +2% per HP% below threshold */
      "&YYour beast stirs restlessly, testing the chains of your humanity.&D",
      "&YThe beast breaks free as primal instincts consume your consciousness!"
   },
   {
      RACE_VAMPIRE,
      25005,  /* morph vnum for vampire_awakening */
      "blood awakening",  /* skill name */
      -1,     /* skill_sn (set at runtime) */
      20,     /* 20% HP threshold - vampires awaken easily when threatened */
      0.75f,  /* Enemy must be 75% of player's PL */
      1,      /* 1% base chance per damage */
      3,      /* +3% per HP% below threshold */
      "&RYour ancient blood burns with forgotten power.&D",
      "&RCenturies of dormant power erupt as your true vampiric nature awakens!"
   },
   {
      RACE_ARCOSIAN,
      25006,  /* morph vnum for arcosian_true */
      "true form",  /* skill name */
      -1,     /* skill_sn (set at runtime) */
      30,     /* 30% HP threshold */
      0.80f,  /* Enemy must be 80% of player's PL - strongest requirement */
      1,      /* 1% base chance per damage - lowest base chance */
      1,      /* +1% per HP% below threshold - requires real desperation */
      "&WYour natural form feels... constraining.&D",
      "&WYour body reshapes as you shed the limitations of your lesser form!"
   }
   /* NOTE: Androids, Bio-Androids, and Eldarians are intentionally omitted */
};

static const int racial_trans_count = sizeof(racial_trans)/sizeof(racial_trans[0]);

/* ===============================================================
 * SPELL FUNCTIONS - All transformation spells in one place
 * =============================================================== */

/* Human Ascended Form */
ch_ret spell_ascended_form( int sn, int level, CHAR_DATA *ch, void *vo )
{
    MORPH_DATA *morph;
    SKILLTYPE *skill = get_skilltype( sn );

    if( IS_NPC(ch) || !IS_HUMAN(ch) )
    {
        send_to_char( "Only humans can access this transformation.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    if( ch->morph )
    {
        send_to_char( "You are already transformed.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    morph = get_morph_vnum( 25001 );
    if( !morph )
    {
        send_to_char( "Ascended form transformation not available.\r\n", ch );
        return rSPELL_FAILED;
    }

    if( !send_skill_transform_messages( ch, skill ) )
    {
        act( AT_MAGIC, "You feel divine potential stirring within your mortal frame...", ch, NULL, NULL, TO_CHAR );
        act( AT_MAGIC, "$n's body begins to glow with inner light...", ch, NULL, NULL, TO_ROOM );
    }

    if( !do_morph_char( ch, morph ) )
        return rSPELL_FAILED;

    apply_transform_skill_effects( ch, sn, skill, level );
    return rNONE;
}

/* Spiritborn Shikai */
ch_ret spell_shikai( int sn, int level, CHAR_DATA *ch, void *vo )
{
    MORPH_DATA *morph;
    SKILLTYPE *skill = get_skilltype( sn );

    if( IS_NPC(ch) || !IS_SPIRITBORN(ch) )
    {
        send_to_char( "Only spiritborn can access shikai.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    if( ch->morph )
    {
        send_to_char( "You are already transformed.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    if( ch->mana < 50 )
    {
        send_to_char( "You lack the spiritual pressure to release your shikai.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    morph = get_morph_vnum( 25002 );
    if( !morph )
    {
        send_to_char( "Shikai transformation not available.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    act( AT_MAGIC, "You close your eyes and focus your spiritual pressure...", ch, NULL, NULL, TO_CHAR );
    act( AT_MAGIC, "$n closes $s eyes, spiritual energy gathering around $m...", ch, NULL, NULL, TO_ROOM );

    WAIT_STATE( ch, PULSE_VIOLENCE );

    if( !send_skill_transform_messages( ch, skill ) )
    {
        act( AT_WHITE, "&WYou whisper your zanpakuto's true name as power floods your being!&D", ch, NULL, NULL, TO_CHAR );
        act( AT_WHITE, "&W$n's spiritual pressure explodes outward in a brilliant display!&D", ch, NULL, NULL, TO_ROOM );
    }

    if( !do_morph_char( ch, morph ) )
        return rSPELL_FAILED;

    apply_transform_skill_effects( ch, sn, skill, level );
    return rNONE;
}

/* Hollowborn Rage */
ch_ret spell_hollow_rage( int sn, int level, CHAR_DATA *ch, void *vo )
{
    MORPH_DATA *morph;
    SKILLTYPE *skill = get_skilltype( sn );

    if( IS_NPC(ch) || !IS_HOLLOWBORN(ch) )
    {
        send_to_char( "Only hollowborn can access this rage.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    if( ch->morph )
    {
        send_to_char( "You are already transformed.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    morph = get_morph_vnum( 25003 );
    if( !morph )
    {
        send_to_char( "Hollow rage transformation not available.\r\n", ch );
        return rSPELL_FAILED;
    }

    if( !send_skill_transform_messages( ch, skill ) )
    {
        act( AT_RED, "The emptiness within you stirs hungrily...", ch, NULL, NULL, TO_CHAR );
        act( AT_RED, "Darkness seems to gather around $n...", ch, NULL, NULL, TO_ROOM );
    }

    if( !do_morph_char( ch, morph ) )
        return rSPELL_FAILED;

    apply_transform_skill_effects( ch, sn, skill, level );
    return rNONE;
}

/* Lycan Primal Form */
ch_ret spell_primal_form( int sn, int level, CHAR_DATA *ch, void *vo )
{
    MORPH_DATA *morph;
    SKILLTYPE *skill = get_skilltype( sn );

    if( IS_NPC(ch) || !IS_LYCAN(ch) )
    {
        send_to_char( "Only lycans can access their primal form.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    if( ch->morph )
    {
        send_to_char( "You are already transformed.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    morph = get_morph_vnum( 25004 );
    if( !morph )
    {
        send_to_char( "Primal form transformation not available.\r\n", ch );
        return rSPELL_FAILED;
    }

    if( !send_skill_transform_messages( ch, skill ) )
    {
        act( AT_YELLOW, "Your beast nature claws its way to the surface...", ch, NULL, NULL, TO_CHAR );
        act( AT_YELLOW, "$n's eyes take on a feral gleam...", ch, NULL, NULL, TO_ROOM );
    }

    if( !do_morph_char( ch, morph ) )
        return rSPELL_FAILED;

    apply_transform_skill_effects( ch, sn, skill, level );
    return rNONE;
}

/* Vampire Blood Awakening */
ch_ret spell_blood_awakening( int sn, int level, CHAR_DATA *ch, void *vo )
{
    MORPH_DATA *morph;
    SKILLTYPE *skill = get_skilltype( sn );

    if( IS_NPC(ch) || !IS_VAMPIRE(ch) )
    {
        send_to_char( "Only vampires can awaken their ancient blood.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    if( ch->morph )
    {
        send_to_char( "You are already transformed.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    morph = get_morph_vnum( 25005 );
    if( !morph )
    {
        send_to_char( "Blood awakening transformation not available.\r\n", ch );
        return rSPELL_FAILED;
    }

    if( !send_skill_transform_messages( ch, skill ) )
    {
        act( AT_BLOOD, "Ancient power stirs in your undead veins...", ch, NULL, NULL, TO_CHAR );
        act( AT_BLOOD, "$n's eyes blaze with crimson fire...", ch, NULL, NULL, TO_ROOM );
    }

    if( !do_morph_char( ch, morph ) )
        return rSPELL_FAILED;

    apply_transform_skill_effects( ch, sn, skill, level );
    return rNONE;
}

/* Arcosian True Form */
ch_ret spell_true_form( int sn, int level, CHAR_DATA *ch, void *vo )
{
    MORPH_DATA *morph;
    SKILLTYPE *skill = get_skilltype( sn );

    if( IS_NPC(ch) || !IS_ARCOSIAN(ch) )
    {
        send_to_char( "Only arcosians can access their true form.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    if( ch->morph )
    {
        send_to_char( "You are already transformed.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    morph = get_morph_vnum( 25006 );
    if( !morph )
    {
        send_to_char( "True form transformation not available.\r\n", ch );
        return rSPELL_FAILED;
    }

    if( !send_skill_transform_messages( ch, skill ) )
    {
        act( AT_WHITE, "Your natural form feels increasingly restrictive...", ch, NULL, NULL, TO_CHAR );
        act( AT_WHITE, "$n's body begins to reshape and grow...", ch, NULL, NULL, TO_ROOM );
    }

    if( !do_morph_char( ch, morph ) )
        return rSPELL_FAILED;

    apply_transform_skill_effects( ch, sn, skill, level );
    return rNONE;
}

/* ===============================================================
 * HELPER FUNCTIONS
 * =============================================================== */

/* Helper to find a character's racial transformation definition */
static const RACIAL_TRANS_DEF *get_racial_trans_def(const CHAR_DATA *ch)
{
   if (!ch || IS_NPC(ch))
      return NULL;
      
   for (int i = 0; i < racial_trans_count; ++i)
   {
      if (ch->race == racial_trans[i].race)
         return &racial_trans[i];
   }
   return NULL;
}

/* Initialize skill numbers at startup */
void init_racial_transformations(void)
{
   for (int i = 0; i < racial_trans_count; i++)
   {
      racial_trans[i].skill_sn = skill_lookup(racial_trans[i].skill_name);
      
      if (racial_trans[i].skill_sn < 0)
      {
         log_printf("WARNING: Racial transformation skill '%s' not found", racial_trans[i].skill_name);
      }
      else
      {
         log_printf("Initialized racial transformation: %s", racial_trans[i].skill_name);
      }
   }
   
   log_string("Racial transformation system initialized");
}

/* Main function called from damage() */
void try_unlock_racial_transformation(CHAR_DATA *victim, CHAR_DATA *aggressor)
{
   if (!victim || IS_NPC(victim) || !victim->fighting)
      return;

   const RACIAL_TRANS_DEF *rt = get_racial_trans_def(victim);
   if (!rt) 
      return;  /* Not a race that uses this system */

   if (victim->morph) 
      return;  /* Already morphed */
      
   /* Check if already learned */
   if (rt->skill_sn >= 0 && victim->pcdata->skills[rt->skill_sn].value_tenths > 0)
      return;  /* Already learned */

   /* HP threshold check */
   int hp_pct = (victim->max_hit > 0) ? (victim->hit * 100 / victim->max_hit) : 100;
   if (hp_pct > rt->hp_pct) 
      return;

   /* Worthy foe check (prevents farming weaklings) */
   if (!aggressor) 
      return;
      
   long long my_pl = get_power_level(victim);
   long long foe_pl = get_power_level(aggressor);
   
   if (my_pl <= 0 || foe_pl <= 0) 
      return;
      
   if ((double)foe_pl / (double)my_pl < rt->min_ratio) 
      return;

   /* Calculate transformation chance */
   int base_chance = rt->base_chance;
   int desperation_points = rt->hp_pct - hp_pct;  /* How many % below threshold */
   int desperation_bonus = desperation_points * rt->desperation_bonus;
   int total_chance = base_chance + desperation_bonus;
   
   /* Cap the maximum chance at something reasonable */
   total_chance = UMIN(total_chance, 25);  /* Max 25% chance per damage */
   
   /* Roll for transformation */
   if (number_percent() > total_chance)
   {
      /* Show occasional hints when they fail the roll */
      int now = current_time;
      if (victim->pcdata->trans_hint_cooldown < now && number_percent() <= 30)
      {
         victim->pcdata->trans_hint_cooldown = now + number_range(8, 15);
         
         /* Show race-specific hint message */
         act(AT_YELLOW, rt->hint_msg, victim, NULL, NULL, TO_CHAR);
         act(AT_YELLOW, "&Y$n's aura flickers strangely for a moment.&D", victim, NULL, NULL, TO_ROOM);
      }
      return;
   }

   /* SUCCESS! They made the roll */
   if (rt->skill_sn >= 0)
   {
      /* We have the skill - do full transformation */
      MORPH_DATA *md = get_morph_vnum(rt->morph_vnum);
      if (!md) 
      {
         bug("try_unlock_racial_transformation: Could not find morph vnum %d for race %d", 
             rt->morph_vnum, rt->race);
         return;
      }

      /* Dramatic unlock sequence */
      send_to_char("&R*** &YTRANSFORMATION UNLOCKED! &R***&D\r\n", victim);
      act(AT_YELLOW, rt->unlock_msg, victim, NULL, NULL, TO_CHAR);
      act(AT_ORANGE, "&O$n roars as $s aura detonates into a new form!&D", victim, NULL, NULL, TO_ROOM);

      /* Use the normal morph path */
      do_morph_char(victim, md);

      /* Permanently learn the transformation skill at 25% */
      int current_value = victim->pcdata->skills[rt->skill_sn].value_tenths;
      if( current_value < 250 )
         trainer_raise_skill_to( victim, rt->skill_sn, 250 );
      
      ch_printf(victim, "&CYou have learned: &W%s &Cat %.1f%% proficiency!&D\r\n",
                skill_table[rt->skill_sn]->name, victim->pcdata->skills[rt->skill_sn].value_tenths / 10.0);
      ch_printf(victim, "&GUse '&W%s&G' to activate this transformation in the future.&D\r\n", 
                rt->skill_name);

      /* Give a power level bonus for unlocking */
      long long bonus = total_chance * 10000;
      if (bonus > 0)
      {
         gain_pl(victim, bonus, true);
         ch_printf(victim, "&YYour awakening grants you %s power level!&D\r\n", 
                   num_punct_ll(bonus));
      }
   }
   else
   {
      /* Skill not found - show testing message */
      send_to_char("&R*** &YTRANSFORMATION UNLOCKED! &R***&D\r\n", victim);
      act(AT_YELLOW, rt->unlock_msg, victim, NULL, NULL, TO_CHAR);
      act(AT_ORANGE, "&O$n roars as $s aura detonates... but nothing happens yet!&D", victim, NULL, NULL, TO_ROOM);
      ch_printf(victim, "&Y[Skill '%s' not found - add it in-game first!]&D\r\n", rt->skill_name);
   }

   /* Reset hint cooldown */
   victim->pcdata->trans_hint_cooldown = current_time + 10;

   /* Save immediately to preserve this milestone */
   save_char_obj(victim);
}

/* Debugging command for immortals */
void do_testtransform(CHAR_DATA *ch, const char *argument)
{
   CHAR_DATA *victim;
   char arg[MAX_INPUT_LENGTH];
   
   if (get_trust(ch) < LEVEL_GREATER)
   {
      send_to_char("You don't have permission to use this command.\r\n", ch);
      return;
   }
   
   one_argument(argument, arg);
   
   if (arg[0] == '\0')
   {
      victim = ch;
   }
   else
   {
      victim = get_char_world(ch, arg);
      if (!victim)
      {
         send_to_char("They aren't here.\r\n", ch);
         return;
      }
   }
   
   if (IS_NPC(victim))
   {
      send_to_char("NPCs don't have racial transformations.\r\n", ch);
      return;
   }
   
   const RACIAL_TRANS_DEF *rt = get_racial_trans_def(victim);
   if (!rt)
   {
      ch_printf(ch, "%s's race is not eligible for racial transformations.\r\n", victim->name);
      return;
   }
   
   ch_printf(ch, "Testing transformation for %s (Race: %s):\r\n", victim->name, race_table[victim->race]->race_name);
   ch_printf(ch, "  Skill: %s (%s)\r\n", rt->skill_name, 
             rt->skill_sn >= 0 ? "FOUND" : "NOT FOUND");
   ch_printf(ch, "  Base Chance: %d%%\r\n", rt->base_chance);
   ch_printf(ch, "  Desperation Bonus: %d%% per HP%% below %d%%\r\n", rt->desperation_bonus, rt->hp_pct);
   
   if (rt->skill_sn >= 0)
   {
      ch_printf(ch, "  Current Skill Level: %.1f%%\r\n", victim->pcdata->skills[rt->skill_sn].value_tenths / 10.0);
   }
   
   /* Calculate current chance */
   int hp_pct = (victim->max_hit > 0) ? (victim->hit * 100 / victim->max_hit) : 100;
   int desperation_points = UMAX(0, rt->hp_pct - hp_pct);
   int total_chance = rt->base_chance + (desperation_points * rt->desperation_bonus);
   total_chance = UMIN(total_chance, 25);
   
   ch_printf(ch, "  Current HP: %d%% (%s threshold)\r\n", hp_pct, 
             hp_pct <= rt->hp_pct ? "BELOW" : "ABOVE");
   ch_printf(ch, "  Current Chance: %d%% per damage\r\n", total_chance);
   
   send_to_char("System ready for testing!\r\n", ch);
}