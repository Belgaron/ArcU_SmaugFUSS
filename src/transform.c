/***************************************************************************
 * Transformation System Implementation for SmaugFUSS 1.9.x
 * 
 * This file contains the core transformation system that integrates with
 * the existing skill system to provide race-gated, skill-based transformations.
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include "mud.h"

/* Local function prototypes */
bool can_transform(CHAR_DATA *ch, int sn);
int calculate_transform_cost(CHAR_DATA *ch, SKILLTYPE *skill);
bool check_world_trigger(CHAR_DATA *ch, SKILLTYPE *skill);
void apply_transformation_effects(CHAR_DATA *ch, SKILLTYPE *skill);
void remove_transformation_effects(CHAR_DATA *ch, SKILLTYPE *skill);

/*
 * Attempt to transform into the specified form
 */
bool attempt_transformation(CHAR_DATA *ch, int sn)
{
    SKILLTYPE *skill = skill_table[sn];
    int mana_cost;
    
    /* Validate it's a transformation skill */
    if (!SPELL_FLAG(skill, SF_TRANSFORMATION)) {
        bug("%s: skill %d is not a transformation", __func__, sn);
        return FALSE;
    }
    
    /* Check if character can use this transformation */
    if (!can_transform(ch, sn))
        return FALSE;
    
    /* Check if already transformed */
    if (ch->active_transformation > 0) {
        if (ch->active_transformation == sn) {
            /* Already in this form - revert instead */
            send_to_char("You are already in that form. Reverting instead.\r\n", ch);
            return deactivate_transformation(ch);
        } else {
            send_to_char("You must revert from your current form first.\r\n", ch);
            return FALSE;
        }
    }
    
    /* Check world triggers */
    if (!check_world_trigger(ch, skill)) {
        send_to_char("The conditions aren't right for that transformation.\r\n", ch);
        return FALSE;
    }
    
    /* Calculate and check mana cost */
    mana_cost = calculate_transform_cost(ch, skill);
    if (ch->mana < mana_cost) {
        ch_printf(ch, "You need %d mana to transform. You only have %d.\r\n", 
                 mana_cost, ch->mana);
        return FALSE;
    }
    
    /* Apply transformation */
    return activate_transformation(ch, sn, mana_cost);
}

/*
 * Activate a transformation
 */
bool activate_transformation(CHAR_DATA *ch, int sn, int mana_cost)
{
    SKILLTYPE *skill = skill_table[sn];
    
    /* Pay mana cost */
    ch->mana -= mana_cost;
    
    /* Set transformation state */
    ch->active_transformation = sn;
    ch->transform_duration = skill->base_duration;
    ch->transform_upkeep_debt = 0;
    ch->transform_hitroll_bonus = 0;
    ch->transform_damroll_bonus = 0;
    ch->transform_start_time = current_time;
    
    /* Apply transformation effects */
    apply_transformation_effects(ch, skill);
    
    /* Send messages */
    if (skill->transform_msg_self && skill->transform_msg_self[0] != '\0')
        send_to_char(skill->transform_msg_self, ch);
    if (skill->transform_msg_room && skill->transform_msg_room[0] != '\0')
        act(AT_ACTION, skill->transform_msg_room, ch, NULL, NULL, TO_ROOM);
    
    /* Start upkeep timer if needed */
    if (skill->base_upkeep_cost > 0) {
        add_timer(ch, TIMER_TRANSFORMATION_UPKEEP, PULSE_PER_SECOND, NULL, 0);
    }
    
    /* Learn from successful transformation */
    learn_from_success(ch, sn);
    
    return TRUE;
}

/*
 * Deactivate current transformation
 */
bool deactivate_transformation(CHAR_DATA *ch)
{
    SKILLTYPE *skill;
    
    if (ch->active_transformation <= 0)
        return FALSE;
    
    skill = skill_table[ch->active_transformation];
    
    /* Remove transformation effects */
    remove_transformation_effects(ch, skill);
    
    /* Send messages */
    if (skill->revert_msg_self && skill->revert_msg_self[0] != '\0')
        send_to_char(skill->revert_msg_self, ch);
    if (skill->revert_msg_room && skill->revert_msg_room[0] != '\0')
        act(AT_ACTION, skill->revert_msg_room, ch, NULL, NULL, TO_ROOM);
    
    /* Clear transformation state */
    ch->active_transformation = 0;
    ch->transform_duration = 0;
    ch->transform_upkeep_debt = 0;
    ch->transform_hitroll_bonus = 0;
    ch->transform_damroll_bonus = 0;
    ch->transform_start_time = 0;
    
    /* Remove upkeep timer */
    remove_timer(ch, TIMER_TRANSFORMATION_UPKEEP);
    
    return TRUE;
}

/*
 * Apply all effects of a transformation
 */
void apply_transformation_effects(CHAR_DATA *ch, SKILLTYPE *skill)
{
    /* Apply stat bonuses */
    ch->mod_str += skill->str_bonus;
    ch->mod_dex += skill->dex_bonus;
    ch->mod_con += skill->con_bonus;
    ch->mod_int += skill->int_bonus;
    ch->mod_wis += skill->wis_bonus;
    ch->mod_spr += skill->spr_bonus;
    
    /* Apply combat bonuses using dice formulas */
    if (skill->hitroll_bonus && skill->hitroll_bonus[0] != '\0') {
        int hitroll = dice_parse(ch, ch->level, skill->hitroll_bonus);
        ch->hitroll += hitroll;
        ch->transform_hitroll_bonus = hitroll; /* Store for removal */
    }
    
    if (skill->damroll_bonus && skill->damroll_bonus[0] != '\0') {
        int damroll = dice_parse(ch, ch->level, skill->damroll_bonus);
        ch->damroll += damroll;
        ch->transform_damroll_bonus = damroll; /* Store for removal */
    }
    
    /* Apply AC bonus */
    ch->armor += skill->ac_bonus;
    
    /* Apply affects */
    if (!xIS_EMPTY(skill->affects_add)) {
        xSET_BITS(ch->affected_by, skill->affects_add);
    }
    if (!xIS_EMPTY(skill->affects_remove)) {
        xREMOVE_BITS(ch->affected_by, skill->affects_remove);
    }
}

/*
 * Remove all effects of a transformation
 */
void remove_transformation_effects(CHAR_DATA *ch, SKILLTYPE *skill)
{
    /* Remove stat bonuses */
    ch->mod_str -= skill->str_bonus;
    ch->mod_dex -= skill->dex_bonus;
    ch->mod_con -= skill->con_bonus;
    ch->mod_int -= skill->int_bonus;
    ch->mod_wis -= skill->wis_bonus;
    ch->mod_spr -= skill->spr_bonus;
    
    /* Remove combat bonuses */
    ch->hitroll -= ch->transform_hitroll_bonus;
    ch->damroll -= ch->transform_damroll_bonus;
    ch->armor -= skill->ac_bonus;
    
    /* Remove affects */
    if (!xIS_EMPTY(skill->affects_add)) {
        xREMOVE_BITS(ch->affected_by, skill->affects_add);
    }
    if (!xIS_EMPTY(skill->affects_remove)) {
        xSET_BITS(ch->affected_by, skill->affects_remove);
    }
}

/*
 * Check if character can use a transformation
 */
bool can_transform(CHAR_DATA *ch, int sn)
{
    SKILLTYPE *skill = skill_table[sn];
    
    /* Basic validation */
    if (IS_NPC(ch)) {
        send_to_char("NPCs cannot transform.\r\n", ch);
        return FALSE;
    }
    
    if (!skill || sn < 0 || sn >= num_skills) {
        bug("%s: invalid skill number %d", __func__, sn);
        return FALSE;
    }
    
    /* Check if character knows the skill */
    if (ch->pcdata->learned[sn] <= 0) {
        send_to_char("You haven't learned that transformation.\r\n", ch);
        return FALSE;
    }
    
    /* Check power level requirement */
    if (get_power_level(ch) < skill->min_power_level) {
        send_to_char("You are not powerful enough for that transformation.\r\n", ch);
        return FALSE;
    }
    
    /* Check position */
    if (!check_pos(ch, skill->minimum_position))
        return FALSE;
    
    return TRUE;
}

/*
 * Calculate mana cost for transformation
 */
int calculate_transform_cost(CHAR_DATA *ch, SKILLTYPE *skill)
{
    int base_cost = skill->min_mana;
    
    /* Scale with character level (higher level = slightly cheaper) */
    base_cost = base_cost * (100 + ch->level) / 120;
    
    /* World trigger can modify cost */
    /* Future: implement cost modifiers from triggers */
    
    return UMAX(1, base_cost);
}

/*
 * Check world trigger conditions
 */
bool check_world_trigger(CHAR_DATA *ch, SKILLTYPE *skill)
{
    bool condition_met = TRUE;
    
    switch (skill->trigger_type) {
        case TRIGGER_NONE:
            return TRUE; /* No trigger required */
            
        case TRIGGER_TIME_HOUR:
            condition_met = (time_info.hour == skill->trigger_condition);
            break;
            
        case TRIGGER_WEATHER:
            condition_met = (time_info.sunlight == skill->trigger_condition);
            break;
            
        case TRIGGER_NEAR_DEATH:
            condition_met = (ch->hit * 100 / ch->max_hit <= skill->trigger_condition);
            break;
            
        case TRIGGER_IN_COMBAT:
            condition_met = (ch->fighting != NULL);
            break;
            
        case TRIGGER_MOON_PHASE:
            /* TODO: Implement when moon phase system exists */
            condition_met = TRUE; /* Placeholder */
            break;
            
        default:
            bug("%s: unknown trigger type %d", __func__, skill->trigger_type);
            condition_met = TRUE;
            break;
    }
    
    /* Apply probability if condition is met */
    if (condition_met && skill->trigger_probability > 0 && skill->trigger_probability < 100) {
        condition_met = (number_percent() <= skill->trigger_probability);
        if (!condition_met) {
            send_to_char("You sense the opportunity, but it slips away...\r\n", ch);
        }
    }
    
    return condition_met;
}

/*
 * Update transformation upkeep - called from update.c
 */
void transformation_upkeep_update(CHAR_DATA *ch)
{
    SKILLTYPE *skill;
    int upkeep;
    
    if (ch->active_transformation <= 0)
        return;
    
    skill = skill_table[ch->active_transformation];
    
    /* Check duration */
    if (ch->transform_duration > 0) {
        ch->transform_duration--;
        if (ch->transform_duration <= 0) {
            send_to_char("Your transformation expires.\r\n", ch);
            deactivate_transformation(ch);
            return;
        }
        
        /* Warn when getting close to expiration */
        if (ch->transform_duration == 30) { /* 30 seconds warning */
            send_to_char("You feel your transformation beginning to fade...\r\n", ch);
        }
    }
    
    /* Handle upkeep cost */
    if (skill->base_upkeep_cost > 0) {
        upkeep = skill->base_upkeep_cost;
        
        /* Scale upkeep with character level (higher level = more efficient) */
        upkeep = upkeep * 100 / (100 + ch->level / 4);
        upkeep = UMAX(1, upkeep);
        
        if (ch->mana >= upkeep) {
            ch->mana -= upkeep;
        } else {
            /* Accumulate debt */
            ch->transform_upkeep_debt += upkeep - ch->mana;
            ch->mana = 0;
            
            /* Force revert if debt too high */
            if (ch->transform_upkeep_debt >= ch->max_mana) {
                send_to_char("You can no longer maintain your transformation!\r\n", ch);
                act(AT_ACTION, "$n's transformation fails!", ch, NULL, NULL, TO_ROOM);
                deactivate_transformation(ch);
            } else if (ch->transform_upkeep_debt >= ch->max_mana / 2) {
                send_to_char("Your transformation is becoming difficult to maintain!\r\n", ch);
            }
        }
    }
}

/*
 * Force transformation deactivation (for emergency situations)
 */
void force_detransformation(CHAR_DATA *ch)
{
    if (ch->active_transformation <= 0)
        return;
    
    send_to_char("Your transformation is forcibly ended!\r\n", ch);
    act(AT_ACTION, "$n suddenly reverts to normal form!", ch, NULL, NULL, TO_ROOM);
    deactivate_transformation(ch);
}

/*
 * Check if character is currently transformed
 */
bool is_transformed(CHAR_DATA *ch)
{
    return (ch->active_transformation > 0);
}

/*
 * Get the name of the current transformation
 */
const char *get_transformation_name(CHAR_DATA *ch)
{
    if (ch->active_transformation <= 0)
        return "normal";
    
    return skill_table[ch->active_transformation]->name;
}

/*
 * Get transformed description for character
 */
const char *get_transformed_desc(CHAR_DATA *ch)
{
    SKILLTYPE *skill;
    
    if (ch->active_transformation <= 0)
        return NULL;
    
    skill = skill_table[ch->active_transformation];
    if (skill->transformed_desc && skill->transformed_desc[0] != '\0')
        return skill->transformed_desc;
    
    return NULL;
}

/*
 * Save transformation data to file
 */
void fwrite_transformation_data(CHAR_DATA *ch, FILE *fp)
{
    if (ch->active_transformation > 0) {
        fprintf(fp, "ActiveTransform  %d\n", ch->active_transformation);
        fprintf(fp, "TransformDuration %d\n", ch->transform_duration);
        fprintf(fp, "TransformDebt    %d\n", ch->transform_upkeep_debt);
        fprintf(fp, "TransformHitroll %d\n", ch->transform_hitroll_bonus);
        fprintf(fp, "TransformDamroll %d\n", ch->transform_damroll_bonus);
        fprintf(fp, "TransformStart   %ld\n", ch->transform_start_time);
    }
}

/*
 * Load transformation data from file
 */
void fread_transformation_data(CHAR_DATA *ch, const char *word, FILE *fp)
{
    if (!str_cmp(word, "ActiveTransform")) {
        ch->active_transformation = fread_number(fp);
        return;
    }
    
    if (!str_cmp(word, "TransformDuration")) {
        ch->transform_duration = fread_number(fp);
        return;
    }
    
    if (!str_cmp(word, "TransformDebt")) {
        ch->transform_upkeep_debt = fread_number(fp);
        return;
    }
    
    if (!str_cmp(word, "TransformHitroll")) {
        ch->transform_hitroll_bonus = fread_number(fp);
        return;
    }
    
    if (!str_cmp(word, "TransformDamroll")) {
        ch->transform_damroll_bonus = fread_number(fp);
        return;
    }
    
    if (!str_cmp(word, "TransformStart")) {
        ch->transform_start_time = fread_number(fp);
        return;
    }
}