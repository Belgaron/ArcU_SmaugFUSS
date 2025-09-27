/*
 * Enhanced Random Combat Message System for SmaugFUSS
 * Provides atmospheric, weapon-specific, and skill-specific combat messages
 */

#include "mud.h"

/* Function declarations for grammar variation system */
void varied_hit_msg(CHAR_DATA *ch, CHAR_DATA *victim, 
                   const char *attack_verb, const char *atmosphere,
                   char *buf1, char *buf2, char *buf3, char punct);

void varied_miss_msg(CHAR_DATA *ch, CHAR_DATA *victim,
                    const char *miss_verb, const char *miss_atmosphere,
                    char *buf1, char *buf2, char *buf3, char punct);

/* Combat atmosphere descriptor arrays */
static const char *combat_atmospheres[] = {
    // Adverbs
	 "cleanly",
	 "brutally",
	 "decisively",
	 "precisely",
	 "swiftly",
	 "coldly",
	 "efficiently",
	 "quietly",
	 "relentlessly",
	 "steadily",
	 "methodically",
	 "unflinchingly",
	 "implacably",
	 "patiently",
	 "mercilessly",
	 "firmly",
	 
	 // With …
	 "with surgical precision",
	 "with measured force",
	 "with controlled fury",
	 "with no wasted motion",
	 "with steady hands",
	 "with quiet certainty",
	 "with iron patience",
	 "with decisive weight",
	 "with practiced rhythm",
	 "with a tight guard",
	 "with sure footing",
	 "with a tightened grip",
	 "with absolute control",
	 "with single-beat commitment",
	 "with forward pressure",
	 "with compact footwork",
	 "with an anchored stance",
	 "with clean execution",
	 "with breath held",
	 "with eyes on target",
	 "with the edge aligned",
	 "with hips driving",
	 "with relaxed shoulders",
	 "with elbows tucked",
	 "with centered intent",
	 "with fixed intent",
	 "with your weight behind it",
	 
	 // In …
	 "in one motion",
	 "in a single beat",
	 "in cold silence",
	 "in perfect balance",
	 "in close quarters",
	 "in your stride",
	 "in a low stance",
	 "in a tight line",
	 "in grim focus",
	 "in full control",
	 
	 // Without …
	 "without hesitation",
	 "without mercy",
	 "without warning",
	 "without breaking stride",
	 "without looking away",
	 "without a wasted breath",
	 
	 // Direction / angle / range
	 "from the blind side",
	 "from an inside angle",
	 "from high to low",
	 "from low to high",
	 "on the inside line",
	 "on the half-step",
	 "on the back foot",
	 "at arm’s length",
	 "at point-blank range",
	 "from an off angle",
	 
	 // Path of the attack
	 "through the guard",
	 "through the opening",
	 "through the stance",
	 "under the guard",
	 "under the ribs",
	 "over the shield",
	 "between the plates",
	 
	 // Mindset / tone
	 "calm and committed",
	 "cold and certain",
	 "focused and unblinking",
	 "quiet and final",
	 "grim and deliberate",
	 
	 // Situational movement / context
	 "as you advance",
	 "as you slip inside",
	 "as you circle left",
	 "as you give ground",
	 "as they overextend",
	 "as their guard breaks",
	NULL
};

/* Unarmed combat attacks */
static const char *unarmed_attacks[] = {
	 	 
	 /* Hands */
    "punch",
	 "jab",
	 "cross",
	 "hook",
	 "uppercut",
	 "backfist",
	 "palm strike",
	 "hammerfist",
	 "chop",
	 "backhand",
	 "slap",
	 "ridgehand",
	 
	 
	 /* Elbows & forearms */
	 "elbow",
	 "short elbow",
	 "spinning elbow",
	 "downward elbow",
	 "forearm smash",
	 
	 /* Knees */
	 "knee",
	 "rising knee",
	 "driving knee",
	 "flying knee",
	 
	 /* Kicks & stomps */
	 "kick",
	 "front kick",
	 "push kick",
	 "round kick",
	 "side kick",
	 "hook kick",
	 "axe kick",
	 "heel kick",
	 "low kick",
	 "shin kick",
	 
	 /* Clinch / close range */
	 "clinch and hit",
	 "shoulder check",
	 "body check",
	 "headbutt",
	NULL
};

static const char *unarmed_atmospheres[] = {
    /* speed / motion */
	 "in a blur",
	 "in an instant",
	 "in midair",
	 "in a single heartbeat",
	 "before the echo fades",
	 "as time seems to stall",
	 "before they can react",
	 
	 /* precision / intent */
	 "with lethal intent",
	 "with fatal focus",
	 "with unshakable will",
	 "with perfect timing",
	 "with zero hesitation",
	 
	 /* power / impact (refined) */
	 "with ground splitting force",
	 "with bone cracking force",
	 "with air splitting force",
	 "with spine rattling impact",
	 "with stone shuddering power",
	 "with a shockwave behind it",
	 "with titan weight behind it",
	 
	 /* range / path */
	 "at a breaths distance",
	 "over their guard",
	 "through their stance",
	 "past their reach",
	 "from the blind side",
	 "from an inside angle",
	 "from an outside angle",
	 "from a shallow angle",
	 "from a sharp angle",
	 "from the rear angle",
	 "through their guard",
	 "through an opening",
	 "under their guard",
	 "on the step in",
	 "on the step out",
	 "on the half step",
	 "on the angle change",
	 "as you slip inside",
	 "as you pass the guard",

	 
	 /* cinematic effects */
	 "as the air ripples",
	 "as dust erupts from the force",
	 "as the ground cracks",
	 "as shadows recoil",
	 "leaving a ringing hush",
	 "as the air ripples",
	 "as debris skitters",
	 "as debris dances",
	 "as loose threads snap",
	 "as cloth snaps tight",
	 "as sleeves flare",
	 "as rings of dust spread",
	 "as the ground thuds",
	 "as hairline cracks creep",
	 "as shadows jump",
	 "as shadows recoil",
	 "as echoes chase the blow",
	 "as the air holds its breath",
	 "as silence breaks",
	 "as light stutters",
	 "as lights flicker",
	 "as the floor shivers",
	 "as the air booms once",
	 "as a dull boom rolls",
	 "leaving blurred edges",
	 "leaving a ringing hush",
	 "leaving a quiet wake",
	 "leaving drifting motes",
	 "leaving the area off balance",
	 
	 /* sound cues (new) */
	 "with a loud thud",
	 "with a sharp crack",
	 "with a sonic snap",
	 "with a dull boom",
	 "with a sharp whistle",
	 "with a forceful shout",
	NULL
};

/* Weapon-specific message arrays */
static const char *blade_attacks[] = {
    "carves", "slices", "cuts", "slashes", "cleaves", "rends", "severs",
    "bisects", "dissects", "flays", "shreds", "lacerates", "gashes",
    "incises", "scores", "marks", "etches", "traces", "draws across", NULL
};

static const char *blade_atmospheres[] = {
    "with razor-sharp precision", "in a silver arc of death", "like a whisper of steel",
    "with surgical accuracy", "in a gleaming flash", "through yielding flesh",
    "with predatory grace", "like liquid mercury", "in a dance of steel",
    "with serpentine fluidity", "like a striking viper", "through the air", NULL
};

static const char *blunt_attacks[] = {
    "pounds", "crushes", "smashes", "batters", "hammers", "pulverizes",
    "demolishes", "shatters", "breaks", "fractures", "splinters", "cracks",
    "caves in", "flattens", "compresses", "impacts", "collides with", NULL
};

static const char *blunt_atmospheres[] = {
    "with thunderous force", "like a falling mountain", "with earth-shaking power",
    "with bone-crushing weight", "like a battering ram", "with seismic impact",
    "with avalanche-like force", "like a meteor strike", "with titanic strength",
    "with demolition force", "like a wrecking ball", "with pulverizing might", NULL
};

static const char *magical_atmospheres[] = {
    "with arcane energy", "in waves of power", "with mystical force",
    "through dimensional barriers", "with otherworldly might", "in spirals of energy",
    "with cosmic force", "through reality itself", "with elemental fury",
    "in cascades of power", "with divine wrath", "through ethereal channels", NULL
};

/* Critical hit special messages */
static const char *critical_prefixes[] = {
    "In a devastating display,", "With overwhelming power,", "In a moment of pure destruction,",
    "With terrifying efficiency,", "In an explosion of violence,", "With unstoppable force,",
    "In a blur of motion,", "With deadly artistry,", "In a crescendo of destruction,",
    "With apocalyptic fury,", "In a symphony of pain,", "With merciless precision,", NULL
};

static const char *critical_suffixes[] = {
    "leaving devastation in its wake!", "shaking the very foundations of reality!",
    "causing the earth to tremble!", "with such force that the air itself screams!",
    "leaving nothing but destruction!", "with power that defies comprehension!",
    "causing reality to bend and warp!", "with the fury of a thousand storms!",
    "leaving a crater of annihilation!", "with force that splits the heavens!", NULL
};

/* Miss message variations */
static const char *miss_variations[] = {
    "narrowly misses", "barely whiffs past", "sails harmlessly by", "passes through empty air",
    "finds only emptiness", "strikes nothing but shadow", "meets only wind",
    "carves through vacant space", "finds its target has vanished", "misses by a hair's breadth",
    "glances off harmlessly", "fails to connect", "passes wide of its mark", NULL
};

static const char *miss_atmospheres[] = {
    "as the target dances away", "in a display of futility", "with frustrating imprecision",
    "as fate intervenes", "through sheer misfortune", "as the attack goes wide",
    "in a moment of clumsiness", "as the target proves elusive", "through poor timing",
    "as the opportunity slips away", "in a stroke of bad luck", "as skill fails", NULL
};

/*
 * Get a random element from a string array
 */
const char *get_random_string(const char **array)
{
    int count = 0;
    int choice;
    
    if (!array || !array[0])
        return "";
        
    /* Count elements */
    while (array[count])
        count++;
        
    choice = number_range(0, count - 1);
    return array[choice];
}

/*
 * Enhanced damage message function - replaces the appropriate section in new_dam_message
 * Add this code to replace the basic message generation in fight.c
 */
void enhanced_dam_message(CHAR_DATA *ch, CHAR_DATA *victim, int dam, unsigned int dt, 
                         OBJ_DATA *obj, long long pl_gained)
{
    char buf1[512], buf2[768], buf3[512]; /* Room, Char, Vict messages */
    char attack_verb[64];
    char atmosphere[128];
    char attacker_name[64];
    char victim_name[64];
    char damage_str[32];
    char pl_display[32];
    char damage_color[8];
    int original_dam = dam;
    bool is_critical = FALSE;
    bool is_skill = FALSE;
    struct skill_type *skill = NULL;
    int w_index = 0;
    char punct;
    
    /* Initialize */
    attack_verb[0] = '\0';
    atmosphere[0] = '\0';
    
    /* Calculate damage percentage for critical/punctuation */
    int dampc = 0;
    if (dam > 0 && victim->max_hit > 0)
        dampc = ((dam * 1000) / victim->max_hit) + (50 - ((victim->hit * 50) / victim->max_hit));
    
    is_critical = (dampc > 150 || dam > victim->max_hit / 3);
    punct = (dampc <= 30) ? '.' : '!';
    
    /* Get names for messages */
    if (IS_NPC(ch))
        strlcpy(attacker_name, ch->short_descr, 64);
    else
        strlcpy(attacker_name, ch->name, 64);
        
    if (IS_NPC(victim))
        strlcpy(victim_name, victim->short_descr, 64);
    else
        strlcpy(victim_name, victim->name, 64);
    
    /* Determine attack type */
    if (dt >= 0 && dt < (unsigned int)num_skills)
    {
        is_skill = TRUE;
        skill = skill_table[dt];
        w_index = 0;
    }
    else if (dt >= TYPE_HIT && dt < TYPE_HIT + sizeof(attack_table) / sizeof(attack_table[0]))
    {
        w_index = dt - TYPE_HIT;
    }
    else
    {
        w_index = 0; /* default to generic hit */
    }
    
    /* Enhanced combat is now the default for everyone */
    bool use_enhanced = TRUE;
    bool brief_mode = FALSE;  /* Always use full atmospheric descriptions */
    
    if (use_enhanced)
      {
        /* Generate enhanced attack verb and atmosphere */
        if (is_skill && skill && skill->name)
         {
            strlcpy(attack_verb, skill->name, 64);
            if (!brief_mode && skill->type == SKILL_SPELL)
                strlcpy(atmosphere, get_random_string(magical_atmospheres), 128);
            else if (!brief_mode)
                strlcpy(atmosphere, get_random_string(combat_atmospheres), 128);
         }
        else
         {
            /* Check if this is unarmed combat (no weapon equipped) */
            OBJ_DATA *weapon = get_eq_char(ch, WEAR_WIELD);
            if (!weapon && w_index == 0) /* No weapon and generic hit = unarmed */
            {
                strlcpy(attack_verb, get_random_string(unarmed_attacks), 64);
                if (!brief_mode)
                    strlcpy(atmosphere, get_random_string(unarmed_atmospheres), 128);
            }
            /* Weapon-based attacks using attack_table indices */
            else if (w_index == 1 || w_index == 2 || w_index == 3 || w_index == 11) /* slice, stab, slash, pierce - blade weapons */
            {
                strlcpy(attack_verb, get_random_string(blade_attacks), 64);
                if (!brief_mode)
                    strlcpy(atmosphere, get_random_string(blade_atmospheres), 128);
            }
            else if (w_index == 6 || w_index == 7 || w_index == 8) /* blast, pound, crush - blunt weapons */
            {
                strlcpy(attack_verb, get_random_string(blunt_attacks), 64);
                if (!brief_mode)
                    strlcpy(atmosphere, get_random_string(blunt_atmospheres), 128);
            }
            else if (w_index == 4) /* whip */
            {
                strlcpy(attack_verb, "lash", 64);
                if (!brief_mode)
                    strlcpy(atmosphere, "with whip-crack precision", 128);
            }
            else if (w_index == 5 || w_index == 10) /* claw, bite */
            {
                strlcpy(attack_verb, get_random_string(blade_attacks), 64);
                if (!brief_mode)
                    strlcpy(atmosphere, "with primal ferocity", 128);
            }
            else /* generic or ranged */
            {
                strlcpy(attack_verb, "strike", 64);
                if (!brief_mode)
                    strlcpy(atmosphere, get_random_string(combat_atmospheres), 128);
            }
         }
        
        /* Handle miss messages */
        if (dam == 0) 
		   {
				/* Miss messages with varied grammar */
				const char *miss_verb = get_random_string(miss_variations);
				const char *miss_atm = brief_mode ? "" : get_random_string(miss_atmospheres);
				
				if (brief_mode) 
				{
						/* Keep brief mode simple */
						snprintf(buf1, 512, "$n %s $N%c", miss_verb, punct);
						snprintf(buf2, 768, "You %s %s%c", miss_verb, victim_name, punct);
						snprintf(buf3, 512, "$n %s you%c", miss_verb, punct);
				} 
				else 
				{
						/* Use varied grammar for full messages */
						varied_miss_msg(ch, victim, miss_verb, miss_atm, buf1, buf2, buf3, punct);
				}
		   } 
		  else 
		   {
			  /* Hit messages with varied grammar */
			  if (is_critical && !brief_mode) 
			   {
					/* CRITICAL HIT - Use your dramatic arrays! */
					const char *crit_prefix = get_random_string(critical_prefixes);
					const char *crit_suffix = get_random_string(critical_suffixes);
					
					snprintf(buf1, 512, "%s $n %s $N %s, %s%c", 
								crit_prefix, attack_verb, atmosphere, crit_suffix, punct);
					snprintf(buf2, 768, "%s you %s $N %s, %s%c", 
								crit_prefix, attack_verb, atmosphere, crit_suffix, punct);
					snprintf(buf3, 512, "%s $n %s you %s, %s%c", 
								crit_prefix, attack_verb, atmosphere, crit_suffix, punct);
				} 
				else if (brief_mode) 
				{
					/* Keep brief mode simple */
					snprintf(buf1, 512, "$n %s $N%c", attack_verb, punct);
					snprintf(buf2, 768, "You %s $N%c", attack_verb, punct);
					snprintf(buf3, 512, "$n %s you%c", attack_verb, punct);
				} 
				else 
				{
					/* Normal hits - use simple variation */
					varied_hit_msg(ch, victim, attack_verb, atmosphere, buf1, buf2, buf3, punct);
				}

		   }
      }
    else
      {
        /* Use original system's vs/vp messages */
        const char *vs, *vp;
        const char *attack;
        int d_index;
        
        /* Get damage index like original system */
        if (dam == 0)
            d_index = 0;
        else if (dampc < 0)
            d_index = 1;
        else if (dampc <= 100)
            d_index = 1 + dampc / 10;
        else if (dampc <= 200)
            d_index = 11 + (dampc - 100) / 20;
        else if (dampc <= 900)
            d_index = 16 + (dampc - 200) / 100;
        else
            d_index = 23;
            
        vs = s_message_table[w_index][d_index];
        vp = p_message_table[w_index][d_index];
        
        if (obj)
            attack = obj->short_descr;
        else
            attack = attack_table[w_index];
            
        if (dam == 0)
         {
            snprintf(buf1, 512, "$n misses $N%c", punct);
            snprintf(buf2, 768, "You miss %s%c", victim_name, punct);
            snprintf(buf3, 512, "$n misses you%c", punct);
         }
        else
         {
            snprintf(buf1, 512, "$n's %s %s $N%c", attack, vs, punct);
            snprintf(buf2, 768, "Your %s %s %s%c", attack, vs, victim_name, punct);
            snprintf(buf3, 512, "$n's %s %s you%c", attack, vp, punct);
         }
      }
    
    /* Add damage display for players */
    if (!IS_NPC(ch))
      {
        /* Format damage with commas */
        if (dam >= 1000000)
            snprintf(damage_str, 32, "%d,%03d,%03d", 
                    dam / 1000000, (dam / 1000) % 1000, dam % 1000);
        else if (dam >= 1000)
            snprintf(damage_str, 32, "%d,%03d", dam / 1000, dam % 1000);
        else
            snprintf(damage_str, 32, "%d", dam);
            
        /* Format PL display */
        if (pl_gained >= 1000000000LL)
            snprintf(pl_display, 32, "+%.2fB", (double)pl_gained / 1000000000.0);
        else if (pl_gained >= 1000000LL)
            snprintf(pl_display, 32, "+%.2fM", (double)pl_gained / 1000000.0);
        else if (pl_gained >= 1000LL)
            snprintf(pl_display, 32, "+%.2fK", (double)pl_gained / 1000.0);
        else
            snprintf(pl_display, 32, "+%lld", pl_gained);
            
        /* Determine RIS color for player attacks */
        if (original_dam > 0 && dam == -1)
            strlcpy(damage_color, "&p", 8);  /* blue - immune */
        else if (original_dam > 0 && dam < original_dam)
            strlcpy(damage_color, "&r", 8);  /* yellow - resistant */
        else if (original_dam > 0 && dam > original_dam)
            strlcpy(damage_color, "&c", 8);  /* magenta - susceptible */
        else
            strlcpy(damage_color, "&z", 8);  /* yellow - normal */
            
        /* Add damage and PL to attacker message */
        if (dam == -1)
            strlcat(buf2, " &w[&pIMMUNE&w]&x", 768);
        else
         {
            char temp[128];
            snprintf(temp, 128, " &w[%s%s dmg&w]&x", damage_color, damage_str);
            strlcat(buf2, temp, 768);
         }
        
        char temp[64];
        snprintf(temp, 64, " &w[&g%s pl&w]&x", pl_display);
        strlcat(buf2, temp, 768);
      }
    
    /* Add damage display for victim */
    if (dam > 0 && !IS_NPC(victim))
    {
        char damage_str_vict[32];
        char damage_color_vict[8];
        
        /* Format damage for victim */
        if (dam >= 1000000)
            snprintf(damage_str_vict, 32, "%d,%03d,%03d", 
                    dam / 1000000, (dam / 1000) % 1000, dam % 1000);
        else if (dam >= 1000)
            snprintf(damage_str_vict, 32, "%d,%03d", dam / 1000, dam % 1000);
        else
            snprintf(damage_str_vict, 32, "%d", dam);
            
        /* Determine RIS color for victim */
        if (original_dam > 0 && dam == -1)
            strlcpy(damage_color_vict, "&B", 8);
        else if (original_dam > 0 && dam < original_dam)
            strlcpy(damage_color_vict, "&C", 8);
        else if (original_dam > 0 && dam > original_dam)
            strlcpy(damage_color_vict, "&P", 8);
        else
            strlcpy(damage_color_vict, "&R", 8);
            
        char temp[64];
        snprintf(temp, 64, " &w[%s%s dmg&w]&x", damage_color_vict, damage_str_vict);
        strlcat(buf3, temp, 512);
    }
    else if (dam == 0 && !IS_NPC(victim))
    {
        strlcat(buf3, " &w[&Y0 dmg&w]&x", 512);
    }
    
    /* Send the messages */
    act(AT_HIT, buf2, ch, NULL, victim, TO_CHAR);
    act(AT_HITME, buf3, ch, NULL, victim, TO_VICT);
    act(AT_ACTION, buf1, ch, NULL, victim, TO_NOTVICT);
}

void varied_hit_msg(CHAR_DATA *ch, CHAR_DATA *victim, 
                   const char *attack_verb, const char *atmosphere,
                   char *buf1, char *buf2, char *buf3, char punct)
{
    int pattern = number_range(1, 5);  /* 5 clean patterns */
    
    switch (pattern) {
        case 1: /* "connects with" - no articles needed */
            if (strlen(atmosphere) > 0) {
                snprintf(buf1, 512, "$n's %s connects with $N %s%c", attack_verb, atmosphere, punct);
                snprintf(buf2, 768, "Your %s connects with $N %s%c", attack_verb, atmosphere, punct);
                snprintf(buf3, 512, "$n's %s connects with you %s%c", attack_verb, atmosphere, punct);
            } else {
                snprintf(buf1, 512, "$n's %s connects with $N%c", attack_verb, punct);
                snprintf(buf2, 768, "Your %s connects with $N%c", attack_verb, punct);
                snprintf(buf3, 512, "$n's %s connects with you%c", attack_verb, punct);
            }
            break;
            
        case 2: /* "takes" - no articles needed */
            if (strlen(atmosphere) > 0) {
                snprintf(buf1, 512, "$N takes $n's %s %s%c", attack_verb, atmosphere, punct);
                snprintf(buf2, 768, "$N takes your %s %s%c", attack_verb, atmosphere, punct);
                snprintf(buf3, 512, "You take $n's %s %s%c", attack_verb, atmosphere, punct);
            } else {
                snprintf(buf1, 512, "$N takes $n's %s%c", attack_verb, punct);
                snprintf(buf2, 768, "$N takes your %s%c", attack_verb, punct);
                snprintf(buf3, 512, "You take $n's %s%c", attack_verb, punct);
            }
            break;
            
        case 3: /* Simple past tense - works perfectly */
            if (strlen(atmosphere) > 0) {
                snprintf(buf1, 512, "$n %sed $N %s%c", attack_verb, atmosphere, punct);
                snprintf(buf2, 768, "You %s $N %s%c", attack_verb, atmosphere, punct);
                snprintf(buf3, 512, "$n %sed you %s%c", attack_verb, atmosphere, punct);
            } else {
                snprintf(buf1, 512, "$n %sed $N%c", attack_verb, punct);
                snprintf(buf2, 768, "You %s $N%c", attack_verb, punct);
                snprintf(buf3, 512, "$n %sed you%c", attack_verb, punct);
            }
            break;
            
        case 4: /* "finds its mark" - elegant, no articles */
            if (strlen(atmosphere) > 0) {
                snprintf(buf1, 512, "$n's %s finds its mark on $N %s%c", attack_verb, atmosphere, punct);
                snprintf(buf2, 768, "Your %s finds its mark on $N %s%c", attack_verb, atmosphere, punct);
                snprintf(buf3, 512, "$n's %s finds its mark on you %s%c", attack_verb, atmosphere, punct);
            } else {
                snprintf(buf1, 512, "$n's %s finds its mark on $N%c", attack_verb, punct);
                snprintf(buf2, 768, "Your %s finds its mark on $N%c", attack_verb, punct);
                snprintf(buf3, 512, "$n's %s finds its mark on you%c", attack_verb, punct);
            }
            break;
            
        case 5: /* "catches" - simple and clean */
        default:
            if (strlen(atmosphere) > 0) {
                snprintf(buf1, 512, "$n's %s catches $N %s%c", attack_verb, atmosphere, punct);
                snprintf(buf2, 768, "Your %s catches $N %s%c", attack_verb, atmosphere, punct);
                snprintf(buf3, 512, "$n's %s catches you %s%c", attack_verb, atmosphere, punct);
            } else {
                snprintf(buf1, 512, "$n's %s catches $N%c", attack_verb, punct);
                snprintf(buf2, 768, "Your %s catches $N%c", attack_verb, punct);
                snprintf(buf3, 512, "$n's %s catches you%c", attack_verb, punct);
            }
            break;
    }
}

/* Replace generate_varied_miss_message with this: */
void varied_miss_msg(CHAR_DATA *ch, CHAR_DATA *victim,
                    const char *miss_verb, const char *miss_atmosphere,
                    char *buf1, char *buf2, char *buf3, char punct)
{
    int pattern = number_range(1, 3);
    
    switch (pattern) {
        case 1: /* Standard miss */
            if (strlen(miss_atmosphere) > 0) {
                snprintf(buf1, 512, "$n's attack %s $N %s%c", miss_verb, miss_atmosphere, punct);
                snprintf(buf2, 768, "Your attack %s $N %s%c", miss_verb, miss_atmosphere, punct);
                snprintf(buf3, 512, "$n's attack %s you %s%c", miss_verb, miss_atmosphere, punct);
            } else {
                snprintf(buf1, 512, "$n %s $N%c", miss_verb, punct);
                snprintf(buf2, 768, "You %s $N%c", miss_verb, punct);
                snprintf(buf3, 512, "$n %s you%c", miss_verb, punct);
            }
            break;
            
        case 2: /* Target avoids */
            snprintf(buf1, 512, "$N avoids $n's attack%c", punct);
            snprintf(buf2, 768, "$N avoids your attack%c", punct);
            snprintf(buf3, 512, "You avoid $n's attack%c", punct);
            break;
            
        case 3: /* Attack fails */
        default:
            snprintf(buf1, 512, "$n's strike fails to find its mark%c", punct);
            snprintf(buf2, 768, "Your strike fails to find its mark%c", punct);
            snprintf(buf3, 512, "$n's strike fails to find its mark%c", punct);
            break;
    }
}