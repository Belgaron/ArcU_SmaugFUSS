/***************************************************************************
 * Gathering System Module for SmaugFUSS 1.9.x (C++)
 *
 * This file consolidates gathering-related gameplay systems. It currently
 * contains the complete fishing implementation and provides shared
 * infrastructure that future gathering skills (such as mining) can reuse.
 *
 * Features:
 * - Fishing skill and command implementation
 * - Fish database and runtime management command
 * - Shared difficulty tracker for gathering tasks/targets
 * - Tool/environment helpers scoped for gathering skills
 ***************************************************************************/

#include <algorithm>
#include <cctype>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "mud.h"

namespace Gathering {

/***************************************************************************
 * Difficulty Tracking
 ***************************************************************************/
struct DifficultyProfile
{
   int difficulty_modifier; /* Positive = harder, Negative = easier */
   int minimum_skill;       /* Minimum skill percent required to attempt */
};

class DifficultyTracker
{
 private:
   std::unordered_map<std::string, DifficultyProfile> profiles;
   static std::unique_ptr<DifficultyTracker> instance;

   static std::string normalize( const std::string &value )
   {
      std::string normalized;
      normalized.reserve( value.size() );

      bool previous_space = false;
      for( unsigned char ch : value )
      {
         if( std::isspace( ch ) )
         {
            if( !previous_space && !normalized.empty() )
            {
               normalized.push_back( ' ' );
               previous_space = true;
            }
            continue;
         }

         previous_space = false;
         normalized.push_back( static_cast<char>( std::tolower( ch ) ) );
      }

      while( !normalized.empty() && normalized.back() == ' ' )
         normalized.pop_back();
      if( !normalized.empty() && normalized.front() == ' ' )
         normalized.erase( normalized.begin() );

      return normalized;
   }

   DifficultyTracker() = default;

 public:
   static DifficultyTracker &getInstance()
   {
      if( !instance )
         instance = std::unique_ptr<DifficultyTracker>( new DifficultyTracker() );
      return *instance;
   }

   void setDifficulty( const std::string &key, int difficulty, int min_skill )
   {
      std::string normalized = normalize( key );
      if( normalized.empty() )
         return;

      DifficultyProfile profile;
      profile.difficulty_modifier = std::clamp( difficulty, -100, 100 );
      profile.minimum_skill = std::max( 0, min_skill );
      profiles[normalized] = profile;
   }

   std::optional<DifficultyProfile> getDifficulty( const std::string &key ) const
   {
      std::string normalized = normalize( key );
      if( normalized.empty() )
         return std::nullopt;

      auto iter = profiles.find( normalized );
      if( iter == profiles.end() )
         return std::nullopt;

      return iter->second;
   }

   void clearDifficulty( const std::string &key )
   {
      std::string normalized = normalize( key );
      if( normalized.empty() )
         return;
      profiles.erase( normalized );
   }

   void setDifficultyForTask( const std::string &skill, const std::string &task, int difficulty, int min_skill )
   {
      setDifficulty( skill + ":" + task, difficulty, min_skill );
   }

   std::optional<DifficultyProfile> getDifficultyForTask( const std::string &skill, const std::string &task ) const
   {
      return getDifficulty( skill + ":" + task );
   }

   void clearDifficultyForTask( const std::string &skill, const std::string &task )
   {
      clearDifficulty( skill + ":" + task );
   }
};

std::unique_ptr<DifficultyTracker> DifficultyTracker::instance = nullptr;

/***************************************************************************
 * Fishing Implementation
 ***************************************************************************/
namespace Fishing {

namespace {
   constexpr const char *SKILL_NAME = "fishing";
   constexpr const char *DIFFICULTY_SKILL_KEY = "fishing";
   int fishingSkillSn = -1;
}

class FishData
{
 public:
   int vnum;
   std::string name;
   std::string short_desc;
   std::string long_desc;
   int rarity;           /* 1-100, lower = more common */
   int min_skill;        /* Minimum fishing skill required */
   int weight;           /* Fish weight */
   int value;            /* Fish value in gold */
   std::vector<int> valid_sectors;  /* Which water sectors this fish appears in */

   FishData( int v, const std::string &n, const std::string &sd, const std::string &ld,
             int r, int ms, int w, int val, const std::vector<int> &sectors )
      : vnum( v ), name( n ), short_desc( sd ), long_desc( ld ), rarity( r ),
        min_skill( ms ), weight( w ), value( val ), valid_sectors( sectors )
   {
   }
};

class FishDatabase
{
 private:
   std::vector<std::unique_ptr<FishData>> fish_types;
   static std::unique_ptr<FishDatabase> instance;

   void registerFishDifficulty( const FishData &fish )
   {
      DifficultyTracker::getInstance().setDifficultyForTask( DIFFICULTY_SKILL_KEY, fish.name, 0, fish.min_skill );
   }

 public:
   static FishDatabase &getInstance()
   {
      if( !instance )
      {
         instance = std::make_unique<FishDatabase>();
         instance->initialize();
      }
      return *instance;
   }

   void initialize()
   {
      auto addFish = [this]( int vnum, const std::string &name, const std::string &short_desc, const std::string &long_desc,
                             int rarity, int min_skill, int weight, int value, const std::vector<int> &sectors )
      {
         auto fish = std::make_unique<FishData>( vnum, name, short_desc, long_desc, rarity, min_skill, weight, value, sectors );
         registerFishDifficulty( *fish );
         fish_types.emplace_back( std::move( fish ) );
      };

      addFish( 10001, "bass", "a large bass",
               "A large bass lies here, its scales glistening.",
               75, 0, 5, 25,
               std::vector<int>{ SECT_WATER_SWIM, SECT_WATER_NOSWIM } );

      addFish( 10002, "trout", "a speckled trout",
               "A beautiful speckled trout lies here, still fresh from the water.",
               50, 25, 3, 40,
               std::vector<int>{ SECT_WATER_SWIM, SECT_UNDERWATER } );

      addFish( 10003, "boot", "an old leather boot",
               "Someone's old waterlogged boot lies here, smelling terrible.",
               90, 0, 2, 1,
               std::vector<int>{ SECT_WATER_SWIM, SECT_WATER_NOSWIM, SECT_UNDERWATER, SECT_OCEANFLOOR } );

      addFish( 10004, "grouper", "a massive grouper",
               "A massive grouper lies here, its size impressive even out of water.",
               25, 60, 15, 100,
               std::vector<int>{ SECT_UNDERWATER, SECT_OCEANFLOOR } );
   }

   const std::vector<std::unique_ptr<FishData>> &getAllFish() const
   {
      return fish_types;
   }

   void addFish( std::unique_ptr<FishData> fish )
   {
      registerFishDifficulty( *fish );
      fish_types.push_back( std::move( fish ) );
   }
};

std::unique_ptr<FishDatabase> FishDatabase::instance = nullptr;

class FishingEquipment
{
 public:
   enum class PoleQuality
   {
      BASIC = 0,
      QUALITY = 1,
      MASTER = 2
   };

   static bool hasFishingPole( CHAR_DATA *ch )
   {
      return getFishingPole( ch ) != nullptr;
   }

   static OBJ_DATA *getFishingPole( CHAR_DATA *ch )
   {
      if( auto obj = get_eq_char( ch, WEAR_HOLD ) )
      {
         if( isFishingPole( obj ) )
            return obj;
      }

      if( auto obj = get_eq_char( ch, WEAR_WIELD ) )
      {
         if( isFishingPole( obj ) )
            return obj;
      }

      for( auto obj = ch->first_carrying; obj; obj = obj->next_content )
      {
         if( isFishingPole( obj ) )
            return obj;
      }

      return nullptr;
   }

   static PoleQuality getPoleQuality( OBJ_DATA *pole )
   {
      if( !pole || !isFishingPole( pole ) )
         return PoleQuality::BASIC;

      if( xIS_SET( pole->extra_flags, ITEM_MAGIC ) )
         return PoleQuality::MASTER;

      if( pole->cost > 100 )
         return PoleQuality::QUALITY;

      return PoleQuality::BASIC;
   }

 private:
   static bool isFishingPole( OBJ_DATA *obj )
   {
      return obj && obj->item_type == ITEM_WEAPON && obj->value[0] == 99;
   }
};

class EnvironmentChecker
{
 public:
   static bool isWaterRoom( ROOM_INDEX_DATA *room )
   {
      if( !room )
         return false;

      const std::vector<int> water_sectors = { SECT_WATER_SWIM, SECT_WATER_NOSWIM, SECT_UNDERWATER, SECT_OCEANFLOOR };

      return std::find( water_sectors.begin(), water_sectors.end(), room->sector_type ) != water_sectors.end();
   }

   static int getFishingDifficultyRating( CHAR_DATA *ch, ROOM_INDEX_DATA *room )
   {
      int difficulty = 50; /* Base difficulty */

      if( !room )
         return difficulty;

      switch ( room->sector_type )
      {
         case SECT_WATER_SWIM:
            difficulty -= 10;
            break;
         case SECT_WATER_NOSWIM:
            difficulty += 5;
            break;
         case SECT_UNDERWATER:
            difficulty += 15;
            break;
         case SECT_OCEANFLOOR:
            difficulty += 20;
            break;
      }

      if( auto pole = FishingEquipment::getFishingPole( ch ) )
      {
         auto quality = FishingEquipment::getPoleQuality( pole );
         switch ( quality )
         {
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

      if( time_info.hour < 6 || time_info.hour > 18 )
         difficulty -= 5;

      return std::clamp( difficulty, 10, 90 );
   }
};

class FishSelector
{
 public:
   static const FishData *selectCatch( const CHAR_DATA *ch, ROOM_INDEX_DATA *room, int fishing_skill )
   {
      auto &db = FishDatabase::getInstance();
      const auto &all_fish = db.getAllFish();

      std::vector<const FishData *> possible_catches;
      possible_catches.reserve( all_fish.size() );

      for( const auto &fish : all_fish )
      {
         auto profile = DifficultyTracker::getInstance().getDifficultyForTask( DIFFICULTY_SKILL_KEY, fish->name );
         int required_skill = fish->min_skill;
         if( profile && profile->minimum_skill > 0 )
            required_skill = profile->minimum_skill;

         if( fishing_skill < required_skill )
            continue;

         if( std::find( fish->valid_sectors.begin(), fish->valid_sectors.end(), room->sector_type ) == fish->valid_sectors.end() )
            continue;

         possible_catches.push_back( fish.get() );
      }

      if( possible_catches.empty() )
         return all_fish.empty() ? nullptr : all_fish.front().get();

      return weightedRandomSelect( possible_catches );
   }

 private:
   static const FishData *weightedRandomSelect( const std::vector<const FishData *> &fish_list )
   {
      int total_weight = 0;
      for( const auto &fish : fish_list )
         total_weight += std::max( 1, fish->rarity );

      if( total_weight <= 0 )
         return fish_list.front();

      int random_roll = number_range( 1, total_weight );
      int current_weight = 0;

      for( const auto &fish : fish_list )
      {
         current_weight += std::max( 1, fish->rarity );
         if( random_roll <= current_weight )
            return fish;
      }

      return fish_list.front();
   }
};

class ObjectCreator
{
 public:
   static OBJ_DATA *createFishObject( const FishData *fish_data )
   {
      if( !fish_data )
         return nullptr;

      auto pObjIndex = get_obj_index( fish_data->vnum );

      if( !pObjIndex )
      {
         pObjIndex = make_object( fish_data->vnum, 0, fish_data->name.c_str() );
         if( !pObjIndex )
         {
            bug( "%s: Could not create object index for vnum %d", __func__, fish_data->vnum );
            return nullptr;
         }

         setupObjectIndex( pObjIndex, fish_data );
      }

      return create_object( pObjIndex, 0 );
   }

 private:
   static void setupObjectIndex( OBJ_INDEX_DATA *pObjIndex, const FishData *fish_data )
   {
      STRFREE( pObjIndex->short_descr );
      pObjIndex->short_descr = STRALLOC( fish_data->short_desc.c_str() );

      STRFREE( pObjIndex->description );
      pObjIndex->description = STRALLOC( fish_data->long_desc.c_str() );

      pObjIndex->item_type = ( fish_data->name == "boot" ) ? ITEM_TRASH : ITEM_FOOD;
      pObjIndex->weight = fish_data->weight;
      pObjIndex->cost = fish_data->value;

      if( pObjIndex->item_type == ITEM_FOOD )
      {
         pObjIndex->value[0] = 24;
         pObjIndex->value[1] = 24;
         pObjIndex->value[3] = 0;
      }
   }
};

int getFishingSkillSn()
{
   if( fishingSkillSn < 0 )
      fishingSkillSn = skill_lookup( SKILL_NAME );
   return fishingSkillSn;
}

void setFishingSkillSn( int sn )
{
   fishingSkillSn = sn;
}

} // namespace Fishing

} // namespace Gathering

using Gathering::DifficultyTracker;
using Gathering::Fishing::FishDatabase;
using Gathering::Fishing::FishSelector;
using Gathering::Fishing::ObjectCreator;
using Gathering::Fishing::EnvironmentChecker;
using Gathering::Fishing::FishingEquipment;

/***************************************************************************
 * External C Interfaces
 ***************************************************************************/
extern "C" void do_fish( CHAR_DATA *ch, const char *argument )
{
   using namespace Gathering;
   using namespace Gathering::Fishing;

   if( IS_NPC( ch ) )
   {
      send_to_char( "NPCs don't need to fish for food.\r\n", ch );
      return;
   }

   if( !FishingEquipment::hasFishingPole( ch ) )
   {
      send_to_char( "You need a fishing pole to fish.\r\n", ch );
      return;
   }

   if( !EnvironmentChecker::isWaterRoom( ch->in_room ) )
   {
      send_to_char( "You need to be near a body of water to fish.\r\n", ch );
      return;
   }

   if( ch->fighting )
   {
      send_to_char( "You can't fish while fighting!\r\n", ch );
      return;
   }

   if( ch->position != POS_STANDING && ch->position != POS_SITTING )
   {
      send_to_char( "You must be standing or sitting to fish.\r\n", ch );
      return;
   }

   int fishingSn = getFishingSkillSn();
   if( fishingSn < 0 )
   {
      bug( "%s: fishing skill not found in skill table", __func__ );
      send_to_char( "Fishing skill is not configured.\r\n", ch );
      return;
   }

   int fishing_skill = learned_percent( ch, fishingSn );

   act( AT_ACTION, "You cast your line into the water and wait patiently.", ch, nullptr, nullptr, TO_CHAR );
   act( AT_ACTION, "$n casts $s fishing line into the water.", ch, nullptr, nullptr, TO_ROOM );

   const auto *caught_fish = FishSelector::selectCatch( ch, ch->in_room, fishing_skill );
   if( !caught_fish )
   {
      send_to_char( "BUG: No fish selected. Please report this.\r\n", ch );
      return;
   }

   auto profile = DifficultyTracker::getInstance().getDifficultyForTask( "fishing", caught_fish->name );
   if( profile && profile->minimum_skill > 0 && fishing_skill < profile->minimum_skill )
   {
      send_to_char( "You lack the experience to reel that catch in and it slips away.\r\n", ch );
      learn_from_failure( ch, fishingSn );
      return;
   }

   int difficulty_rating = EnvironmentChecker::getFishingDifficultyRating( ch, ch->in_room );
   if( profile )
      difficulty_rating = std::clamp( difficulty_rating + profile->difficulty_modifier, 0, 100 );

   int roll = number_percent();
   int modifier = difficulty_rating - 50;
   if( modifier != 0 )
   {
      roll = std::clamp( roll + modifier, 0, 150 );
   }

   if( !can_use_skill( ch, roll, fishingSn ) )
   {
      act( AT_ACTION, "After waiting for a while, you reel in your line with nothing to show for it.",
           ch, nullptr, nullptr, TO_CHAR );
      act( AT_ACTION, "$n reels in $s fishing line, looking disappointed.",
           ch, nullptr, nullptr, TO_ROOM );
      learn_from_failure( ch, fishingSn );
      return;
   }

   auto fish_obj = ObjectCreator::createFishObject( caught_fish );
   if( !fish_obj )
   {
      send_to_char( "BUG: Could not create fish object. Please report this.\r\n", ch );
      return;
   }

   obj_to_char( fish_obj, ch );

   ch_printf( ch, "You feel a tug on your line! You reel in %s!\r\n", fish_obj->short_descr );
   act( AT_ACTION, "$n reels in $s line with a catch!", ch, nullptr, nullptr, TO_ROOM );

   learn_from_success( ch, fishingSn );
}

extern "C" void init_gathering_system()
{
   using namespace Gathering;
   using namespace Gathering::Fishing;

   DifficultyTracker::getInstance();
   FishDatabase::getInstance();

   int sn = skill_lookup( SKILL_NAME );
   if( sn < 0 )
   {
      bug( "%s: fishing skill not found in skill table", __func__ );
      return;
   }

   auto skill = skill_table[sn];
   if( !skill )
   {
      bug( "%s: fishing skill table entry is NULL", __func__ );
      return;
   }

   setFishingSkillSn( sn );

   skill->skill_fun = do_fish;
   skill->target = TAR_IGNORE;
   skill->minimum_position = POS_SITTING;
   skill->type = SKILL_SKILL;

   if( !skill->noun_damage )
      skill->noun_damage = strdup( "fishing attempt" );
   if( !skill->msg_off )
      skill->msg_off = strdup( "You feel like you could fish again." );
}

extern "C" void init_fishing_skill()
{
   init_gathering_system();
}

extern "C" void do_fishdb( CHAR_DATA *ch, const char *argument )
{
   using namespace Gathering;

   if( IS_NPC( ch ) || get_trust( ch ) < LEVEL_IMMORTAL )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   char arg[MAX_INPUT_LENGTH];
   one_argument( argument, arg );

   if( str_cmp( arg, "list" ) )
   {
      send_to_char( "Usage: fishdb list\r\n", ch );
      return;
   }

   auto &db = FishDatabase::getInstance();
   const auto &fish_list = db.getAllFish();

   ch_printf( ch, "Available Fish Types:\r\n" );
   ch_printf( ch, "%-6s %-15s %-20s %4s %5s %4s %4s %6s %6s\r\n",
              "Vnum", "Name", "Short Desc", "Rare", "Skill", "Wt", "Val", "DMod", "Req" );
   ch_printf( ch, "%s\r\n", std::string( 85, '-' ).c_str() );

   for( const auto &fish : fish_list )
   {
      auto profile = DifficultyTracker::getInstance().getDifficultyForTask( "fishing", fish->name );
      int difficulty_mod = profile ? profile->difficulty_modifier : 0;
      int minimum_skill = profile ? profile->minimum_skill : fish->min_skill;

      ch_printf( ch, "%-6d %-15s %-20s %4d %5d %4d %4d %6d %6d\r\n",
                 fish->vnum, fish->name.c_str(), fish->short_desc.c_str(),
                 fish->rarity, fish->min_skill, fish->weight, fish->value,
                 difficulty_mod, minimum_skill );
   }
}

extern "C" void set_gather_task_difficulty( const char *skill, const char *task, int difficulty, int min_skill )
{
   if( !skill || !task )
      return;

   DifficultyTracker::getInstance().setDifficultyForTask( skill, task, difficulty, min_skill );
}

extern "C" bool get_gather_task_difficulty( const char *skill, const char *task, int *difficulty, int *min_skill )
{
   if( !skill || !task )
      return false;

   auto profile = DifficultyTracker::getInstance().getDifficultyForTask( skill, task );
   if( !profile )
      return false;

   if( difficulty )
      *difficulty = profile->difficulty_modifier;
   if( min_skill )
      *min_skill = profile->minimum_skill;
   return true;
}

extern "C" void clear_gather_task_difficulty( const char *skill, const char *task )
{
   if( !skill || !task )
      return;

   DifficultyTracker::getInstance().clearDifficultyForTask( skill, task );
}
