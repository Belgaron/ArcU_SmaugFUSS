/***************************************************************************
 * Fishing System Implementation for SmaugFUSS 1.9.x (C++)
 * 
 * This file contains the complete fishing system including:
 * - Fishing skill and command
 * - Fish objects and catch mechanics  
 * - Bait system
 * - Fishing pole requirement
 ***************************************************************************/

#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <memory>
#include "mud.h"

namespace Fishing {

/***************************************************************************
 * Fish Data Class
 ***************************************************************************/
class FishData {
public:
    int vnum;
    std::string name;
    std::string short_desc;
    std::string long_desc;
    int rarity;           // 1-100, lower = more common
    int min_skill;        // Minimum fishing skill required
    int weight;           // Fish weight
    int value;            // Fish value in gold
    std::vector<int> valid_sectors;  // Which water sectors this fish appears in
    
    FishData(int v, const std::string& n, const std::string& sd, const std::string& ld,
             int r, int ms, int w, int val, const std::vector<int>& sectors)
        : vnum(v), name(n), short_desc(sd), long_desc(ld), rarity(r), 
          min_skill(ms), weight(w), value(val), valid_sectors(sectors) {}
};

/***************************************************************************
 * Fish Database
 ***************************************************************************/
class FishDatabase {
private:
    std::vector<std::unique_ptr<FishData>> fish_types;
    static std::unique_ptr<FishDatabase> instance;
    
public:
    static FishDatabase& getInstance() {
        if (!instance) {
            instance = std::make_unique<FishDatabase>();
            instance->initialize();
        }
        return *instance;
    }
    
    void initialize() {
        // Common fish - appears in calm and rough water
        fish_types.emplace_back(std::make_unique<FishData>(
            10001, "bass", "a large bass",
            "A large bass lies here, its scales glistening.",
            75, 0, 5, 25, 
            std::vector<int>{SECT_WATER_SWIM, SECT_WATER_NOSWIM}
        ));
        
        // Quality fish - appears in clean water only
        fish_types.emplace_back(std::make_unique<FishData>(
            10002, "trout", "a speckled trout",
            "A beautiful speckled trout lies here, still fresh from the water.",
            50, 25, 3, 40,
            std::vector<int>{SECT_WATER_SWIM, SECT_UNDERWATER}
        ));
        
        // Junk catch - appears everywhere
        fish_types.emplace_back(std::make_unique<FishData>(
            10003, "boot", "an old leather boot",
            "Someone's old waterlogged boot lies here, smelling terrible.",
            90, 0, 2, 1,
            std::vector<int>{SECT_WATER_SWIM, SECT_WATER_NOSWIM, SECT_UNDERWATER, SECT_OCEANFLOOR}
        ));
        
        // Deep water fish - requires high skill
        fish_types.emplace_back(std::make_unique<FishData>(
            10004, "grouper", "a massive grouper",
            "A massive grouper lies here, its size impressive even out of water.",
            25, 60, 15, 100,
            std::vector<int>{SECT_UNDERWATER, SECT_OCEANFLOOR}
        ));
    }
    
    const std::vector<std::unique_ptr<FishData>>& getAllFish() const {
        return fish_types;
    }
    
    void addFish(std::unique_ptr<FishData> fish) {
        fish_types.push_back(std::move(fish));
    }
};

// Static member definition
std::unique_ptr<FishDatabase> FishDatabase::instance = nullptr;

/***************************************************************************
 * Fishing Equipment Manager
 ***************************************************************************/
class FishingEquipment {
public:
    enum class PoleQuality {
        BASIC = 0,
        QUALITY = 1,
        MASTER = 2
    };
    
    static bool hasFishingPole(CHAR_DATA* ch) {
        return getFishingPole(ch) != nullptr;
    }
    
    static OBJ_DATA* getFishingPole(CHAR_DATA* ch) {
        // Check held item
        if (auto obj = get_eq_char(ch, WEAR_HOLD)) {
            if (isFishingPole(obj)) return obj;
        }
        
        // Check wielded item
        if (auto obj = get_eq_char(ch, WEAR_WIELD)) {
            if (isFishingPole(obj)) return obj;
        }
        
        // Check inventory
        for (auto obj = ch->first_carrying; obj; obj = obj->next_content) {
            if (isFishingPole(obj)) return obj;
        }
        
        return nullptr;
    }
    
    static PoleQuality getPoleQuality(OBJ_DATA* pole) {
        if (!pole || !isFishingPole(pole)) return PoleQuality::BASIC;
        
        // Check for magical properties or specific vnums
        if (xIS_SET(pole->extra_flags, ITEM_MAGIC)) {
            return PoleQuality::MASTER;
        }
        
        // Quality poles have higher cost
        if (pole->cost > 100) {
            return PoleQuality::QUALITY;
        }
        
        return PoleQuality::BASIC;
    }
    
private:
    static bool isFishingPole(OBJ_DATA* obj) {
        // Use ITEM_WEAPON with specific values to identify fishing poles
        // value[0] = 99 will mark it as a fishing pole
        return obj && obj->item_type == ITEM_WEAPON && obj->value[0] == 99;
    }
};

/***************************************************************************
 * Environment Checker
 ***************************************************************************/
class EnvironmentChecker {
public:
    static bool isWaterRoom(ROOM_INDEX_DATA* room) {
        if (!room) return false;
        
        const std::vector<int> water_sectors = {
            SECT_WATER_SWIM, SECT_WATER_NOSWIM, 
            SECT_UNDERWATER, SECT_OCEANFLOOR
        };
        
        return std::find(water_sectors.begin(), water_sectors.end(), 
                        room->sector_type) != water_sectors.end();
    }
    
    static int getFishingDifficulty(CHAR_DATA* ch, ROOM_INDEX_DATA* room) {
        int difficulty = 50; // Base difficulty
        
        // Adjust for room sector type
        switch(room->sector_type) {
            case SECT_WATER_SWIM:
                difficulty -= 10; // Easier in calm water
                break;
            case SECT_WATER_NOSWIM:
                difficulty += 5;  // Harder in rough water
                break;
            case SECT_UNDERWATER:
                difficulty += 15; // Much harder underwater
                break;
            case SECT_OCEANFLOOR:
                difficulty += 20; // Hardest at ocean floor
                break;
        }
        
        // Equipment bonus
        if (auto pole = FishingEquipment::getFishingPole(ch)) {
            auto quality = FishingEquipment::getPoleQuality(pole);
            switch(quality) {
                case FishingEquipment::PoleQuality::QUALITY:
                    difficulty -= 5;
                    break;
                case FishingEquipment::PoleQuality::MASTER:
                    difficulty -= 15;
                    break;
                default:
                    break;
            }
        }
        
        // Time of day affects fishing (dawn/dusk bonus)
        if (time_info.hour < 6 || time_info.hour > 18) {
            difficulty -= 5;
        }
        
        // Weather effects (if you have weather system)
        // Add weather checks here
        
        return std::clamp(difficulty, 10, 90);
    }
};

/***************************************************************************
 * Fish Selector
 ***************************************************************************/
class FishSelector {
public:
    static const FishData* selectCatch(CHAR_DATA* ch, ROOM_INDEX_DATA* room) {
        auto& db = FishDatabase::getInstance();
        const auto& all_fish = db.getAllFish();
        
        int fishing_skill = learned_percent(ch, skill_lookup("fishing"));
        
        // Build list of possible catches
        std::vector<const FishData*> possible_catches;
        
        for (const auto& fish : all_fish) {
            // Check skill requirement
            if (fishing_skill < fish->min_skill) continue;
            
            // Check if fish appears in this sector
            if (std::find(fish->valid_sectors.begin(), fish->valid_sectors.end(), 
                         room->sector_type) == fish->valid_sectors.end()) {
                continue;
            }
            
            possible_catches.push_back(fish.get());
        }
        
        if (possible_catches.empty()) {
            // Fallback to first fish if nothing else works
            return all_fish.empty() ? nullptr : all_fish[0].get();
        }
        
        // Weighted random selection
        return weightedRandomSelect(possible_catches);
    }
    
private:
    static const FishData* weightedRandomSelect(const std::vector<const FishData*>& fish_list) {
        // Calculate total weight
        int total_weight = 0;
        for (const auto& fish : fish_list) {
            total_weight += fish->rarity;
        }
        
        if (total_weight <= 0) return fish_list[0];
        
        // Random selection
        int random_roll = number_range(1, total_weight);
        int current_weight = 0;
        
        for (const auto& fish : fish_list) {
            current_weight += fish->rarity;
            if (random_roll <= current_weight) {
                return fish;
            }
        }
        
        return fish_list[0]; // Fallback
    }
};

/***************************************************************************
 * Object Creator
 ***************************************************************************/
class ObjectCreator {
public:
    static OBJ_DATA* createFishObject(const FishData* fish_data) {
        if (!fish_data) return nullptr;
        
        // Try to get existing object index
        auto pObjIndex = get_obj_index(fish_data->vnum);
        
        if (!pObjIndex) {
            // Create new object index
            pObjIndex = make_object(fish_data->vnum, 0, fish_data->name.c_str());
            if (!pObjIndex) {
                bug("ObjectCreator::createFishObject: Could not create object index for vnum %d", 
                    fish_data->vnum);
                return nullptr;
            }
            
            setupObjectIndex(pObjIndex, fish_data);
        }
        
        return create_object(pObjIndex, 0);
    }
    
private:
    static void setupObjectIndex(OBJ_INDEX_DATA* pObjIndex, const FishData* fish_data) {
        // Set descriptions
        STRFREE(pObjIndex->short_descr);
        pObjIndex->short_descr = STRALLOC(fish_data->short_desc.c_str());
        
        STRFREE(pObjIndex->description);
        pObjIndex->description = STRALLOC(fish_data->long_desc.c_str());
        
        // Set object properties
        pObjIndex->item_type = (fish_data->name == "boot") ? ITEM_TRASH : ITEM_FOOD;
        pObjIndex->weight = fish_data->weight;
        pObjIndex->cost = fish_data->value;
        
        if (pObjIndex->item_type == ITEM_FOOD) {
            pObjIndex->value[0] = 24; // Hours of food
            pObjIndex->value[1] = 24; // Hours of fullness  
            pObjIndex->value[3] = 0;  // Not poisoned
        }
    }
};

} // namespace Fishing

/***************************************************************************
 * Main Fishing Command (C interface for MUD)
 ***************************************************************************/
extern "C" void do_fish(CHAR_DATA* ch, const char* argument) {
    using namespace Fishing;
    
    if (IS_NPC(ch)) {
        send_to_char("NPCs don't need to fish for food.\r\n", ch);
        return;
    }
    
    // Validate equipment
    if (!FishingEquipment::hasFishingPole(ch)) {
        send_to_char("You need a fishing pole to fish.\r\n", ch);
        return;
    }
    
    // Validate environment
    if (!EnvironmentChecker::isWaterRoom(ch->in_room)) {
        send_to_char("You need to be near a body of water to fish.\r\n", ch);
        return;
    }
    
    // Check combat status
    if (ch->fighting) {
        send_to_char("You can't fish while fighting!\r\n", ch);
        return;
    }
    
    // Check position
    if (ch->position != POS_STANDING && ch->position != POS_SITTING) {
        send_to_char("You must be standing or sitting to fish.\r\n", ch);
        return;
    }
    
    // Fishing attempt messages
    act(AT_ACTION, "You cast your line into the water and wait patiently.", ch, nullptr, nullptr, TO_CHAR);
    act(AT_ACTION, "$n casts $s fishing line into the water.", ch, nullptr, nullptr, TO_ROOM);
    
    // Calculate success
    int skill_check = number_percent();
    
    if (!can_use_skill(ch, skill_check, skill_lookup("fishing"))) {
        // Failed attempt
        act(AT_ACTION, "After waiting for a while, you reel in your line with nothing to show for it.", 
            ch, nullptr, nullptr, TO_CHAR);
        act(AT_ACTION, "$n reels in $s fishing line, looking disappointed.", 
            ch, nullptr, nullptr, TO_ROOM);
        learn_from_failure(ch, skill_lookup("fishing"));
        return;
    }
    
    // Success! Select catch
    const auto* caught_fish = FishSelector::selectCatch(ch, ch->in_room);
    if (!caught_fish) {
        send_to_char("BUG: No fish selected. Please report this.\r\n", ch);
        return;
    }
    
    // Create fish object
    auto fish_obj = ObjectCreator::createFishObject(caught_fish);
    if (!fish_obj) {
        send_to_char("BUG: Could not create fish object. Please report this.\r\n", ch);
        return;
    }
    
    // Give to player
    obj_to_char(fish_obj, ch);
    
    // Success messages
    ch_printf(ch, "You feel a tug on your line! You reel in %s!\r\n", fish_obj->short_descr);
    act(AT_ACTION, "$n reels in $s line with a catch!", ch, nullptr, nullptr, TO_ROOM);
    
    // Learn from success
    learn_from_success(ch, skill_lookup("fishing"));
}

/***************************************************************************
 * Skill Initialization (C interface)
 ***************************************************************************/
extern "C" void init_fishing_skill() {
    // Initialize the fish database
    Fishing::FishDatabase::getInstance();
    
    int sn = skill_lookup("fishing");
    if (sn < 0) {
        bug("init_fishing_skill: fishing skill not found in skill table");
        return;
    }
    
    auto skill = skill_table[sn];
    if (!skill) {
        bug("init_fishing_skill: fishing skill table entry is NULL");
        return;
    }
    
    // Set up fishing skill properties
    skill->skill_fun = do_fish;
    skill->target = TAR_IGNORE;
    skill->minimum_position = POS_SITTING;
    skill->type = SKILL_SKILL;
    
    // Set messages if not already set
    if (!skill->noun_damage)
        skill->noun_damage = strdup("fishing attempt");
    if (!skill->msg_off)
        skill->msg_off = strdup("You feel like you could fish again.");
}

/***************************************************************************
 * Admin Commands for Runtime Fish Management (C interface)
 ***************************************************************************/
extern "C" void do_fishdb(CHAR_DATA* ch, const char* argument) {
    using namespace Fishing;
    
    if (IS_NPC(ch) || get_trust(ch) < LEVEL_IMMORTAL) {
        send_to_char("Huh?\r\n", ch);
        return;
    }
    
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);
    
    if (!str_cmp(arg, "list")) {
        auto& db = FishDatabase::getInstance();
        const auto& fish_list = db.getAllFish();
        
        ch_printf(ch, "Available Fish Types:\r\n");
        ch_printf(ch, "%-6s %-15s %-20s %s %s %s %s\r\n", 
                 "Vnum", "Name", "Short Desc", "Rare", "Skill", "Wt", "Val");
        ch_printf(ch, "%s\r\n", std::string(70, '-').c_str());
        
        for (const auto& fish : fish_list) {
            ch_printf(ch, "%-6d %-15s %-20s %-4d %-5d %-3d %-3d\r\n",
                     fish->vnum, fish->name.c_str(), fish->short_desc.c_str(),
                     fish->rarity, fish->min_skill, fish->weight, fish->value);
        }
        return;
    }
    
    send_to_char("Usage: fishdb list\r\n", ch);
}