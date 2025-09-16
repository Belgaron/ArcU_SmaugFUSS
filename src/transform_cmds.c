/***************************************************************************
 * Transformation Commands for SmaugFUSS 1.9.x
 * 
 * Player commands: transform, untransform, transformations
 * Immortal commands: transstat, transforce, form
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include "mud.h"

/*
 * Transform command - attempt to transform into specified form
 */
void do_transform(CHAR_DATA *ch, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    int sn;
    
    if (IS_NPC(ch)) {
        send_to_char("NPCs cannot transform.\r\n", ch);
        return;
    }
    
    argument = one_argument(argument, arg);
    
    if (arg[0] == '\0') {
        send_to_char("Transform into what?\r\n", ch);
        send_to_char("Use 'transformations' to see available forms.\r\n", ch);
        return;
    }
    
    /* Find the transformation skill */
    sn = skill_lookup(arg);
    if (sn < 0) {
        send_to_char("You don't know that transformation.\r\n", ch);
        return;
    }
    
    /* Check if it's a transformation skill */
    if (!SPELL_FLAG(skill_table[sn], SF_TRANSFORMATION)) {
        send_to_char("That's not a transformation.\r\n", ch);
        return;
    }
    
    /* Attempt the transformation */
    attempt_transformation(ch, sn);
}

/*
 * Untransform command - return to normal form
 */
void do_untransform(CHAR_DATA *ch, const char *argument)
{
    if (IS_NPC(ch)) {
        send_to_char("NPCs cannot untransform.\r\n", ch);
        return;
    }
    
    if (ch->active_transformation <= 0) {
        send_to_char("You are not transformed.\r\n", ch);
        return;
    }
    
    if (deactivate_transformation(ch)) {
        send_to_char("You untransform to your normal form.\r\n", ch);
    } else {
        send_to_char("You failed to untransform.\r\n", ch);
    }
}

/*
 * Transformations command - list available transformation skills
 */
void do_transformations(CHAR_DATA *ch, const char *argument)
{
    int sn, col = 0;
    bool found = FALSE;
    SKILLTYPE *skill;
    
    if (IS_NPC(ch)) {
        send_to_char("NPCs don't have transformation skills.\r\n", ch);
        return;
    }
    
    send_to_char("&CAvailable Transformations:\r\n&w", ch);
    send_to_char("------------------------\r\n", ch);
    
    for (sn = 0; sn < num_skills; sn++) {
        skill = skill_table[sn];
        
        if (!skill || !skill->name)
            continue;
            
        /* Check if it's a transformation skill */
        if (!SPELL_FLAG(skill, SF_TRANSFORMATION))
            continue;
            
        /* Check if character knows this skill */
        if (ch->pcdata->learned[sn] <= 0)
            continue;
            
        found = TRUE;
        
        /* Show skill name with learned percentage */
        ch_printf(ch, "&C%-20s &w(%3d%%) ", 
                  skill->name, ch->pcdata->learned[sn]);
        
        /* Show if currently active */
        if (ch->active_transformation == sn)
            send_to_char("&G[ACTIVE]&w", ch);
        
        if (++col % 2 == 0)
            send_to_char("\r\n", ch);
    }
    
    if (col % 2 != 0)
        send_to_char("\r\n", ch);
        
    if (!found) {
        send_to_char("&wYou don't know any transformation skills.\r\n", ch);
        send_to_char("&wVisit a trainer to learn some!\r\n", ch);
    } else {
        send_to_char("\r\n&wUse '&Ctransform <form>&w' to transform.\r\n", ch);
        send_to_char("&wUse '&Cuntransform&w' to return to normal form.\r\n", ch);
        
        if (ch->active_transformation > 0) {
            ch_printf(ch, "\r\n&GCurrently transformed: &C%s&w\r\n",
                      get_transformation_name(ch));
        }
    }
}

/*
 * Show detailed information about a transformation skill
 */
void show_transformation_details(CHAR_DATA *ch, SKILLTYPE *skill, int sn)
{
    char duration_buf[50];
    
    if (!skill || !skill->name) {
        send_to_char("Invalid transformation skill.\r\n", ch);
        return;
    }
    
    ch_printf(ch, "&C%s&w\r\n", skill->name);
    ch_printf(ch, "========================\r\n");
    
    /* Basic info */
    ch_printf(ch, "Type: Transformation\r\n");
    ch_printf(ch, "Minimum Power Level: %d\r\n", skill->min_power_level);
    ch_printf(ch, "Mana Cost: %d\r\n", skill->min_mana);
    
    /* Upkeep and duration */
    ch_printf(ch, "Upkeep Cost: %d mana/tick\r\n", skill->base_upkeep_cost);
    
    if (skill->base_duration == -1)
        strcpy(duration_buf, "Until dismissed");
    else if (skill->base_duration == 0)
        strcpy(duration_buf, "Instant");
    else
        snprintf(duration_buf, sizeof(duration_buf), "%d ticks", skill->base_duration);
    
    ch_printf(ch, "Duration: %s\r\n", duration_buf);
    
    /* Stat modifiers */
    if (skill->str_bonus != 0 || skill->dex_bonus != 0 || skill->con_bonus != 0 ||
        skill->int_bonus != 0 || skill->wis_bonus != 0 || skill->spr_bonus != 0) {
        send_to_char("\r\nStat Modifiers:\r\n", ch);
        if (skill->str_bonus != 0)
            ch_printf(ch, "  Strength: %+d\r\n", skill->str_bonus);
        if (skill->dex_bonus != 0)
            ch_printf(ch, "  Dexterity: %+d\r\n", skill->dex_bonus);
        if (skill->con_bonus != 0)
            ch_printf(ch, "  Constitution: %+d\r\n", skill->con_bonus);
        if (skill->int_bonus != 0)
            ch_printf(ch, "  Intelligence: %+d\r\n", skill->int_bonus);
        if (skill->wis_bonus != 0)
            ch_printf(ch, "  Wisdom: %+d\r\n", skill->wis_bonus);
        if (skill->spr_bonus != 0)
            ch_printf(ch, "  Spirit: %+d\r\n", skill->spr_bonus);
    }
    
    /* Combat modifiers */
    send_to_char("\r\nCombat Modifiers:\r\n", ch);
    if (skill->hitroll_bonus && skill->hitroll_bonus[0] != '\0')
        ch_printf(ch, "  Hitroll: %s\r\n", skill->hitroll_bonus);
    if (skill->damroll_bonus && skill->damroll_bonus[0] != '\0')
        ch_printf(ch, "  Damroll: %s\r\n", skill->damroll_bonus);
    if (skill->ac_bonus != 0)
        ch_printf(ch, "  AC Bonus: %+d\r\n", skill->ac_bonus);
    
    /* World trigger */
    if (skill->trigger_type != TRIGGER_NONE) {
        send_to_char("\r\nWorld Trigger:\r\n", ch);
        ch_printf(ch, "  Type: %d (%s)\r\n", skill->trigger_type,
                  skill->trigger_type == TRIGGER_TIME_HOUR ? "Hour" :
                  skill->trigger_type == TRIGGER_WEATHER ? "Weather" :
                  skill->trigger_type == TRIGGER_NEAR_DEATH ? "Near Death" :
                  skill->trigger_type == TRIGGER_IN_COMBAT ? "In Combat" :
                  skill->trigger_type == TRIGGER_MOON_PHASE ? "Moon Phase" : "Unknown");
        ch_printf(ch, "  Condition: %d\r\n", skill->trigger_condition);
        ch_printf(ch, "  Probability: %d%%\r\n", skill->trigger_probability);
    }
    
    /* Messages */
    if (skill->transform_msg_self && skill->transform_msg_self[0] != '\0') {
        send_to_char("\r\nTransformation Message:\r\n", ch);
        ch_printf(ch, "  %s\r\n", skill->transform_msg_self);
    }
}

/*
 * Transstat command - show transformation skill information
 */
void do_transstat(CHAR_DATA *ch, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    SKILLTYPE *skill;
    int sn;
    
    if (IS_NPC(ch)) {
        send_to_char("NPCs don't have transformation skills.\r\n", ch);
        return;
    }
    
    argument = one_argument(argument, arg);
    
    if (arg[0] == '\0') {
        send_to_char("Show statistics for which transformation?\r\n", ch);
        send_to_char("Use 'transformations' to see available forms.\r\n", ch);
        return;
    }
    
    /* Find the transformation skill */
    sn = skill_lookup(arg);
    if (sn < 0) {
        send_to_char("That transformation skill doesn't exist.\r\n", ch);
        return;
    }
    
    skill = skill_table[sn];
    
    /* Check if it's a transformation skill */
    if (!SPELL_FLAG(skill, SF_TRANSFORMATION)) {
        send_to_char("That's not a transformation skill.\r\n", ch);
        return;
    }
    
    /* Show detailed information */
    show_transformation_details(ch, skill, sn);
}

/*
 * Transforce command - force someone to transform or untransform (immortal only)
 */
void do_transforce(CHAR_DATA *ch, const char *argument)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int sn;
    
    if (get_trust(ch) < LEVEL_GREATER) {
        send_to_char("You don't have permission to use this command.\r\n", ch);
        return;
    }
    
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    
    if (arg1[0] == '\0' || arg2[0] == '\0') {
        send_to_char("Syntax: transforce <player> <transformation|untransform>\r\n", ch);
        return;
    }
    
    /* Find the target player */
    victim = get_char_world(ch, arg1);
    if (!victim) {
        send_to_char("They aren't here.\r\n", ch);
        return;
    }
    
    if (IS_NPC(victim)) {
        send_to_char("NPCs cannot be forced to transform.\r\n", ch);
        return;
    }
    
    /* Check if they want to untransform */
    if (!str_cmp(arg2, "untransform")) {
        if (victim->active_transformation <= 0) {
            send_to_char("They are not transformed.\r\n", ch);
            return;
        }
        
        force_detransformation(victim);
        ch_printf(ch, "You force %s to untransform to normal form.\r\n", victim->name);
        ch_printf(victim, "%s forces you to untransform to normal form.\r\n", ch->name);
        return;
    }
    
    /* Find the transformation skill */
    sn = skill_lookup(arg2);
    if (sn < 0) {
        send_to_char("That transformation skill doesn't exist.\r\n", ch);
        return;
    }
    
    /* Check if it's a transformation skill */
    if (!SPELL_FLAG(skill_table[sn], SF_TRANSFORMATION)) {
        send_to_char("That's not a transformation skill.\r\n", ch);
        return;
    }
    
    /* Force the transformation */
    if (victim->active_transformation > 0) {
        send_to_char("Forcing them to untransform first...\r\n", ch);
        force_detransformation(victim);
    }
    
    /* Set their skill to 100% temporarily if they don't know it */
    bool temp_skill = FALSE;
    if (victim->pcdata->learned[sn] <= 0) {
        victim->pcdata->learned[sn] = 100;
        temp_skill = TRUE;
    }
    
    /* Attempt the forced transformation */
    if (attempt_transformation(victim, sn)) {
        ch_printf(ch, "You force %s to transform into %s.\r\n", 
                  victim->name, skill_table[sn]->name);
        ch_printf(victim, "%s forces you to transform into %s.\r\n", 
                  ch->name, skill_table[sn]->name);
    } else {
        send_to_char("The forced transformation failed.\r\n", ch);
        
        /* Remove temporary skill knowledge */
        if (temp_skill)
            victim->pcdata->learned[sn] = 0;
    }
}

/*
 * Form command - show current transformation status
 */
void do_form(CHAR_DATA *ch, const char *argument)
{
    SKILLTYPE *skill;
    
    if (IS_NPC(ch)) {
        send_to_char("NPCs don't transform.\r\n", ch);
        return;
    }
    
    if (ch->active_transformation <= 0) {
        send_to_char("&wYou are in your normal form.\r\n", ch);
        return;
    }
    
    skill = skill_table[ch->active_transformation];
    
    ch_printf(ch, "&GCurrent Form: &C%s&w\r\n", skill->name);
    
    if (ch->transform_duration > 0) {
        ch_printf(ch, "&wDuration remaining: &Y%d&w ticks\r\n", ch->transform_duration);
    } else if (skill->base_duration == -1) {
        send_to_char("&wDuration: &YUntil dismissed&w\r\n", ch);
    }
    
    if (skill->base_upkeep_cost > 0) {
        ch_printf(ch, "&wUpkeep cost: &Y%d&w mana per tick\r\n", skill->base_upkeep_cost);
        if (ch->transform_upkeep_debt > 0) {
            ch_printf(ch, "&RUpkeep debt: &Y%d&w mana\r\n", ch->transform_upkeep_debt);
        }
    }
    
    /* Show active bonuses */
    if (skill->str_bonus != 0 || skill->dex_bonus != 0 || skill->con_bonus != 0 ||
        skill->int_bonus != 0 || skill->wis_bonus != 0 || skill->spr_bonus != 0) {
        send_to_char("&wActive stat bonuses:\r\n", ch);
        if (skill->str_bonus != 0)
            ch_printf(ch, "  &CStrength: &Y%+d&w\r\n", skill->str_bonus);
        if (skill->dex_bonus != 0)
            ch_printf(ch, "  &CDexterity: &Y%+d&w\r\n", skill->dex_bonus);
        if (skill->con_bonus != 0)
            ch_printf(ch, "  &CConstitution: &Y%+d&w\r\n", skill->con_bonus);
        if (skill->int_bonus != 0)
            ch_printf(ch, "  &CIntelligence: &Y%+d&w\r\n", skill->int_bonus);
        if (skill->wis_bonus != 0)
            ch_printf(ch, "  &CWisdom: &Y%+d&w\r\n", skill->wis_bonus);
        if (skill->spr_bonus != 0)
            ch_printf(ch, "  &CSpirit: &Y%+d&w\r\n", skill->spr_bonus);
    }
    
    if (ch->transform_hitroll_bonus != 0 || ch->transform_damroll_bonus != 0 || skill->ac_bonus != 0) {
        send_to_char("&wActive combat bonuses:\r\n", ch);
        if (ch->transform_hitroll_bonus != 0)
            ch_printf(ch, "  &CHitroll: &Y%+d&w\r\n", ch->transform_hitroll_bonus);
        if (ch->transform_damroll_bonus != 0)
            ch_printf(ch, "  &CDamroll: &Y%+d&w\r\n", ch->transform_damroll_bonus);
        if (skill->ac_bonus != 0)
            ch_printf(ch, "  &CArmor Class: &Y%+d&w\r\n", skill->ac_bonus);
    }
}

/*****************************************************************************************************
*																									 *
*           					Transformation Functions											 *
*																									 *
*****************************************************************************************************/

/*
 * Ancient Eldarian transformation skill function
 */
ch_ret spell_ancient(int sn, int level, CHAR_DATA *ch, void *vo)
{
    SKILLTYPE *skill = skill_table[sn];
    
    if (IS_NPC(ch)) {
        send_to_char("NPCs cannot use transformations.\r\n", ch);
        return rSPELL_FAILED;
    }
    
    /* Check if it's a transformation skill */
    if (!SPELL_FLAG(skill, SF_TRANSFORMATION)) {
        bug("%s: skill %d is not a transformation", __func__, sn);
        return rSPELL_FAILED;
    }
    
    /* Check if already transformed into this form */
    if (ch->active_transformation == sn) {
        /* Already in this form - untransform */
        if (deactivate_transformation(ch)) {
            send_to_char("You revert from your ancient eldarian form.\r\n", ch);
            return rNONE;
        } else {
            send_to_char("You failed to revert from your transformation.\r\n", ch);
            return rSPELL_FAILED;
        }
    }
    
    /* Attempt the transformation */
    if (attempt_transformation(ch, sn)) {
        return rNONE;
    } else {
        return rSPELL_FAILED;
    }
}