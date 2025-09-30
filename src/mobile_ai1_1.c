/*
 * FILE:        ai.c
 * Purpose:     To create a basic AI sub-system to make
 *              AI battles more realistic..
 * Credits:     Zachery Crosswhite (Cyhawk/Thri)
 * Contact:     cyhawkx@gmail.com
 * Contact2:    AIM: CalibanL
 *
 * Install      Very basic, I used rom's previous ACT_MAGE
 * and info:    bit for this. Near the top of mobile_update in
 *              update.c, after the check for specs, and after
 *              my forced do_wear(ch, "all") (cough) I stuck..
 *
 *              if (IS_SET(ch->act, ACT_MAGE))
 *                  ai_wizard(ch);
 *
 *              I also changed MOBILE_PULSE to 20, due to
 *              the fact its normaly 8, and 99% of stock spells
 *              are 12 or higher.. so.. This is to prevent
 *              un-human-like casting ability. I would
 *              say make it 12 perhaps..
 *
 *              Idealy, when this is expanded, there will be
 *              a wrapper to determine class/AI, as well as
 *              choices for universal stuff (returning home,
 *              sleeping, eating, etc) before the specifics...
 *              As well as checks for how much 'wait' the mob
 *              has from spells (I have timed casting, but the
 *              idea is the same for Insta-cast/wait stock) This
 *              way, the mob casts as fast, or faster than a 
 *              good player.
 *
 * TODO:        Well, obviously this is just a basic piece of code...
 *              There needs to be ones for each 'class' of mobile
 *              you want to have, the 4 basics included in stock,
 *              as well as other special ones, (Group Healers,
 *              mobile leaders/'boss' mobs, maybe special mobs in
 *              here too, or 'super' mages..
 *
 *              Intelligence/Wisdom Checks, dumb wizards wont always
 *              cast, this also gives credence to giving mobs stat points
 *              too.
 *
 *              Checks for Mana, right now, they will always attempt to cast,
 *              I forsee some major spam-fests whe theyre out of mana.. ugh
 *              Maybe if theyre really hurt, they flee/find a healer..
 *              Ooo, I can see some intersting City AI here.
 *
 *              Debuf/Curse spells seprated. Lets face it, in PK, it goes in
 *              a specific order. Debuf->curse->fight (curse being any stat
 *              reduction or offensive enchantments). Mobiles should do this
 *              too.
 *
 *              Add function wrappers for ease of expandability and improvement
 *              of universal AI functions like going home, eating, etc.
 *
 * Licence:     This code falls under the Rom Licence, with the following
 *              additions:
 *              8) A helpfile accessable via the keyword '_thri' Must be
 *              viewable to all players, regardless of level, with the following
 *              Information:
 *              
 *              AI Sub-system created by Thri of Vandagard MUD.
 *              Email: cyhawkx@gmail.com
 *              AIM: CalibanL
 *
 *              Any previous snippets of mine used must also be included
 *              in this helpfile.
 *
 *              9) If you have a 'credits' command or credits helpfile,
 *              a small mention of my Name and the code snip should also
 *              be there.
 *
 *              10) You are also required to make contact with Thri via
 *              any means listed. (AIM/Email) Telling him you are going
 *              to use his code. (Email is best).
 *
 * Afterward:   Anywho, Enjoy the code, and make sure you expand it!
 *              Also, if you do expand it, send me what ya did, id love
 *              to incorporate this into the main sources, and maybe
 *              release a new stock codebase based around this, ROM 3.0?
 *              well, you know what i mean =)
 *
 *  -Cyhawk/Thri
 */

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"

/**
 * AI_DEBUG: Turn on/off Chatting of what the AI is doing
 */
#define AI_DEBUG 1

/** ai()
 *
 * Primary wrapper to take care of basics (sleep, eat)
 * and to determine individual ai functions via class/race
 *
 *  Sleeping?   -> Hungry?  -> Thirsty?     -> Tired?
 *              -> Enemies? -> Attack?      -> race_ai
 *              -> class_ai
 
 * Main AI Controller - HANDLES NON-COMBAT BEHAVIOR ONLY
 * Combat initiation is handled by aggr_update() to prevent duplicates
 *
 */
void ai(CHAR_DATA * mob)
{
    int _mob_hp = 100 * (mob->hit) / (mob->max_hit + 1);
    bool can_heal_self = TRUE; /* Expand me */
    
    /*
     * ACTION_ Sleeping?
     */
    switch (mob->position)
    {   
        default:
        case POS_STANDING:
            break;
        case POS_SLEEPING:
        case POS_RESTING:
        case POS_SITTING:
            if (IS_AFFECTED(mob, AFF_SLEEP))
                break;
            if (_mob_hp <= 75 && !can_heal_self)
                break;
            break;
    }
    
    /*
     * ACTION_ Wear equipment
     */
    do_wear(mob, "all");
    
    /*
     * ACTION_ Class AI (healing, buffing, combat tactics)
     */
    if (ai_wizard(mob))
        return;
    if (ai_priest(mob))
        return;

    /*
     * ACTION_ COMBAT INITIATION DISABLED
     * This is now handled exclusively by aggr_update() to prevent
     * duplicate attacks. The AI system focuses on:
     * - Self-maintenance (healing, buffing)
     * - Combat tactics (when already fighting)
     * - Non-combat behaviors
     */
    
    return;
}

/* Basic Action outline:
                    -> Cast
        -> Fight    -> Melee
Action              -> Debuf
                    -> Flee?

                    -> Heal Self
        -> Idle     -> Enchant Self
                    -> Idle Chit Chat
*/

/** ai_wizard()
 *
 * Primary AI Control for Wizard mobs, follows
 * a basic procedure, hopefuly this will work.
 *
 * Returns TRUE if it did something.
 */
bool ai_wizard(CHAR_DATA * mob) {
    int _mob_hp = 100 * (mob->hit) / (mob->max_hit + 1);
    int _level  = mob->level;
    int chance  = 0;
    
    /* If not a mage, dont go */
    if (!IS_SET(mob->act, ACT_MAGE))
        return FALSE;
    
    mob->spell_cyan     = 50;
    mob->spell_red      = 50;
    mob->spell_yellow   = 50;
    
    /* Ok, Lets start with idle actions.. */
    if (mob->fighting == NULL)
    {
        /* ACTION_ Heal Start (75% or lower) */
        if (_mob_hp < 75)
        {
#if AI_DEBUG
            do_function (mob, &do_say, "I need to heal myself");
#endif
            /* ACTION_ Heal: Lets see if he can hide first */
            if (!is_affected (mob, skill_lookup("invisibility"))
                && _level >= 8
                && !IS_AFFECTED(mob, AFF_INVISIBLE))
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Woha that was close, gotta hide!");
#endif
                do_function (mob, &do_cast, "'invisibility'");
                return TRUE;
            }
            
            if (_level >= 25)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "I'm over level 25, I'll use Heal");
#endif
                do_function (mob, &do_cast, "heal self");
                return TRUE;
            }
            else
            {
                if (mob->position == POS_RESTING)
                    return TRUE;
#if AI_DEBUG
                do_function (mob, &do_say, "I'm not over level 25, I need to rest");
#endif
                do_function (mob, &do_rest, "rest");
                return TRUE;
            }
        }
        else
        {
            if (mob->position != POS_STANDING)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Much Better!");
#endif
                do_function (mob, &do_stand, "");
            }
        }
                
        /* ACTION_ Heal End */
        
        /* ACTION_ Enchant */
        /* Enchant - Orcish Strength */
        if (!is_affected (mob, skill_lookup("orcish strength"))
           && _level >= 3)
        {
#if AI_DEBUG
            do_function (mob, &do_say, "I don't have orcish strength on.");
#endif
            do_function (mob, &do_cast, "'orcish strength'");
            return TRUE;
        }
        /* Enchant - Invisibility */
        if (!is_affected (mob, skill_lookup("invisibility"))
            && _level >= 8
            && !IS_AFFECTED(mob, AFF_INVISIBLE)
            && mob->fighting == NULL)
        {
#if AI_DEBUG
            do_function (mob, &do_say, "I need to be invisible!");
#endif
            do_function (mob, &do_cast, "'invisibility'");
            return TRUE;
        }
        /* Enchant - Invisibility */
        if (!is_affected (mob, skill_lookup("stone skin"))
            && _level >= 12)
        {
#if AI_DEBUG
            do_function (mob, &do_say, "I need to protect myself with Stone skin!");
#endif
            do_function (mob, &do_cast, "'stone skin'");
            return TRUE;
        }
        /* Enchant - Adept */
        if (!is_affected (mob, skill_lookup("adept"))
            && _level >= 20)
        {
#if AI_DEBUG
            do_function (mob, &do_say, "What kind of Wizard isnt adept?");
#endif
            do_function (mob, &do_cast, "'adept'");
            return TRUE;
        }
        /* Enchant - Flight */
        if (!is_affected (mob, skill_lookup("flight"))
            && _level >= 23
            && !IS_AFFECTED(mob, AFF_FLYING))
        {
#if AI_DEBUG
            do_function (mob, &do_say, "My legs are tired...");
#endif
            do_function (mob, &do_cast, "'flight'");
            return TRUE;
        }
        /* Enchant - Giant Strength */
        if (!is_affected (mob, skill_lookup("giant strength"))
            && _level >= 26)
        {
#if AI_DEBUG
            do_function (mob, &do_emote, "looks in a mirror and tries to flex.");
            do_function (mob, &do_say, "Hmm, the ladies sure dont like wimps!");
#endif
            do_function (mob, &do_cast, "'giant strength'");
            return TRUE;
        }
        /* Enchant - Haste */
        if (!is_affected (mob, skill_lookup("haste"))
            && _level >= 30)
        {
#if AI_DEBUG
            do_function (mob, &do_say, "I feel sluggish...");
#endif
            do_function (mob, &do_cast, "'haste'");
            return TRUE;
        }
        /* Enchant - Immolate */
        if (!is_affected (mob, skill_lookup("immolate"))
            && _level >= 31)
        {
#if AI_DEBUG
            do_function (mob, &do_say, "I need to feel some POWAAAHH!!");
#endif
            do_function (mob, &do_cast, "'immolate'");
            return TRUE;
        }
        /* Enchant - Anti-magic Shell */
        if (!is_affected (mob, skill_lookup("anti-magic shell"))
            && _level >= 35)
        {
#if AI_DEBUG
            do_function (mob, &do_say, "I feel too vulnerable..");
#endif
            do_function (mob, &do_cast, "'anti-magic shell'");
            return TRUE;
        }
        /* Enchant - Manashield */
        if (!is_affected (mob, skill_lookup("manashield"))
            && _level >= 45)
        {
#if AI_DEBUG
            do_function (mob, &do_say, "Dude? Wheres my Manashield?");
#endif
            do_function (mob, &do_cast, "'manashield'");
            return TRUE;
        }
    }
    
    /* Ok, Idle actions done, lets start the fighting stuff! */
    if (mob->fighting != NULL)
    {
        /* ACTION_ Fighting: Flee under 35% */
        if (_mob_hp <= 35)
        {
            /* 50% chance to flee every update */
            if (number_range(1, 100) > 50)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Holy bejezzus batman, Im out of here!");
#endif
                do_function (mob, &do_flee, "");
                return TRUE;
            }
        }
        /* Idealy, spells will be Highest Level to Lowest
         * So the AI will hit harder as theyre stronger..
         */
        /* Cut levels every 20 for ease of use */
        
        /*
         * Level 21-41
         */
        if ((_level >= 21) && (_level <= 999))
        {
            bool action_taken = FALSE;

            chance = number_range(1, 7);

            if (chance == 1 && _level >= 21)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Acid anyone?");
#endif
                do_function (mob, &do_cast, "'acid blast'");
                action_taken = TRUE;
            }
            else if (chance == 2 && _level >= 28)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Mmm fire..");
#endif
                do_function (mob, &do_cast, "'fire storm'");
                action_taken = TRUE;
            }
            else if (chance == 3 && _level >= 28)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Just die..");
#endif
                do_function (mob, &do_cast, "'death touch'");
                action_taken = TRUE;
            }
            else if (chance == 4 && _level >= 32)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Here kitty kitty..");
#endif
                do_function (mob, &do_cast, "'energy drain'");
                action_taken = TRUE;
            }
            else if (chance == 5 && _level >= 36)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Is it cold in here, or is it just you?");
#endif
                do_function (mob, &do_cast, "'ice bolt'");
                action_taken = TRUE;
            }
            else if (chance == 6 && _level >= 38)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "crush crush CRUSH CRUSH! FIRE!");
#endif
                do_function (mob, &do_cast, "'fire crush'");
                action_taken = TRUE;
            }
            else if (chance == 7 && _level >= 40)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Wanna smell my breath?");
#endif
                do_function (mob, &do_cast, "'acid breath'");
                action_taken = TRUE;
            }

            if (action_taken)
                return TRUE;
        }
        
        /*
         * Level Under 20
         */
        if (_level <= 20)
        {
            bool action_taken = FALSE;

            chance = number_range(1, 10);

            if (chance == 1 && _level >= 1)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Eat Wizard Fire!");
#endif
                do_function (mob, &do_cast, "'wizard fire'");
                action_taken = TRUE;
            }
            else if (chance == 2 && _level >= 1)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Suck on some Wizard Frost!");
#endif
                do_function (mob, &do_cast, "'wizard frost'");
                action_taken = TRUE;
            }
            else if (chance == 3 && _level >= 1)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Feel the Powah of Wizard Spark!");
#endif
                do_function (mob, &do_cast, "'wizard spark'");
                action_taken = TRUE;
            }
            else if (chance == 4 && _level >= 4)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Are my hands hot to you?");
#endif
                do_function (mob, &do_cast, "'burning hands'");
                action_taken = TRUE;
            }
            else if (chance == 5 && _level >= 5)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Do you feel sick?");
#endif
                do_function (mob, &do_cast, "'weaken'");
                action_taken = TRUE;
            }
            else if (chance == 6 && _level >= 6)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Ooo, purtty colours...");
#endif
                do_function (mob, &do_cast, "'colour spray'");
                action_taken = TRUE;
            }
            else if (chance == 7 && _level >= 9)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Ooo, its chilly in here!");
#endif
                do_function (mob, &do_cast, "'chill touch'");
                action_taken = TRUE;
            }
            else if (chance == 8 && _level >= 10)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Wow, your strong, can I have some?");
#endif
                do_function (mob, &do_cast, "'leech strength'");
                action_taken = TRUE;
            }
            else if (chance == 9 && _level >= 16)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "You can't hit what you can't see!");
#endif
                do_function (mob, &do_cast, "'blindness'");
                action_taken = TRUE;
            }
            else if (chance == 10 && _level >= 19)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Hope this hurts!");
#endif
                do_function (mob, &do_cast, "'force spike'");
                action_taken = TRUE;
            }

            if (action_taken)
                return TRUE;
        }
    }   /* End fighting */
    
    return FALSE;
}

/** ai_priest()
 *
 * Primary AI Control for Priest Mobs.
 *
 * Returns TRUE if it did something.
 */
bool ai_priest(CHAR_DATA * mob) {
    int _mob_hp = 100 * (mob->hit) / (mob->max_hit + 1);
    int _level  = mob->level;
    int chance  = 0;
    
    /* If not a mage, dont go */
    if (!IS_SET(mob->act, ACT_CLERIC))
        return FALSE;

    mob->spell_cyan     = 50;
    mob->spell_yellow   = 50;
    mob->spell_red      = 50;
    mob->spell_violet   = 50;
    /*
     * ACTION_ Not fighting
     */
    if (mob->fighting == NULL)
    {
        /*
         * ACTION_ Remove Bad crap
         */
        {
        /* Remove: Poison */
        if (IS_AFFECTED(mob, AFF_POISON)
           && _level >= 14)
        {
#if AI_DEBUG
            do_function (mob, &do_say, "I feel so.. sick...");
#endif
            do_function (mob, &do_cast, "'cure poison'");
            return TRUE;
        }
        /* Remove: Blindness */
        if (IS_AFFECTED(mob, AFF_BLIND)
           && _level >= 11)
        {
#if AI_DEBUG
            do_function (mob, &do_say, "I can't see!!!");
#endif
            do_function (mob, &do_cast, "'cure blindness'");
            return TRUE;
        }
        /* Remove: Curse */
        if (IS_AFFECTED(mob, AFF_CURSE)
           && _level >= 21)
        {
#if AI_DEBUG
            do_function (mob, &do_say, "I feel disconnected..");
#endif
            do_function (mob, &do_cast, "'remove curse' self");
            return TRUE;
        }
        }
    
        /* ACTION_ Heal Start (75% or lower) */
        if (_mob_hp < 75)
        {
            /* Work backwards in terms of power for better heal spells */
            /* HEAL_ Restoration */
            if (_level >= 95)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Restoration here I come!");
#endif
                do_function (mob, &do_cast, "restoration");
                return TRUE;
            }
            /* HEAL_ Heal Spell */
            if (_level >= 60)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "I'll just use HEAL then...");
#endif
                do_function (mob, &do_cast, "heal");
                return TRUE;
            }
            /* HEAL_ Cure Critical */
            if (_level >= 25)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "Being beat on is the suck");
#endif
                do_function (mob, &do_cast, "cure serious");
                return TRUE;
            }
            /* HEAL_ Heal Spell */
            if (_level >= 60)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "I'll just use HEAL then...");
#endif
                do_function (mob, &do_cast, "heal");
                return TRUE;
            }
            if (_level >= 1)
            {
#if AI_DEBUG
                do_function (mob, &do_say, "I'm at least level 1, cure light wounds");
#endif
                do_function (mob, &do_cast, "cure light wounds");
                return TRUE;
            }
        }
        /*
         * ACTION_ Healing: End
         */
        /*
         * ACTION_ Enchant
         */
        {
        /* Enchant - Armor */
        if (!is_affected (mob, skill_lookup("armor"))
           && _level >= 1)
        {
#if AI_DEBUG
            do_function (mob, &do_say, "I don't have armor on...");
#endif
            do_function (mob, &do_cast, "'armor'");
            return TRUE;
        }
        /* Enchant - Bless */
        if (!is_affected (mob, skill_lookup("bless"))
           && _level >= 3)
        {
#if AI_DEBUG
            do_function (mob, &do_say, "I'm not rightous!");
#endif
            do_function (mob, &do_cast, "'bless'");
            return TRUE;
        }
        /* Enchant - Bark Skin */
        if (!is_affected (mob, skill_lookup("bark skin"))
           && _level >= 15)
        {
#if AI_DEBUG
            do_function (mob, &do_say, "My Skin is too soft");
#endif
            do_function (mob, &do_cast, "'bark skin'");
            return TRUE;
        }
        /* Enchant - Holy Sight */
        if (!is_affected (mob, skill_lookup("holy sight"))
           && _level >= 37)
        {
#if AI_DEBUG
            do_function (mob, &do_say, "My My its dark in here");
#endif
            do_function (mob, &do_cast, "'holy sight'");
            return TRUE;
        }
        /* Enchant - Frenzy */
        if (!is_affected (mob, skill_lookup("frenzy"))
           && _level >= 41)
        {
#if AI_DEBUG
            do_function (mob, &do_say, "I'm not mad enough!");
#endif
            do_function (mob, &do_cast, "'frenzy'");
            return TRUE;
        }
        /* Enchant - Sanctuary */
        if (!is_affected (mob, skill_lookup("sanctuary"))
           && !IS_AFFECTED(mob, AFF_SANCTUARY)
           && _level >= 46)
        {
#if AI_DEBUG
            do_function (mob, &do_say, "Wow, its not pretty here");
#endif
            do_function (mob, &do_cast, "'sanctuary'");
            return TRUE;
        }
        }

    }
    /*
     * END Not Fighting
     */
    return FALSE;
}
