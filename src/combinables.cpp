// This file takes care of all the skills that make stuff by combining it
// in it's container type.  For example, poison making.


#include <db.h>
#include <fight.h>
#include <room.h>
#include <obj.h>
#include <connect.h>
#include <utility.h>
#include <character.h>
#include <handler.h>
#include <db.h>
#include <player.h>
#include <levels.h>
#include <interp.h>
#include <magic.h>
#include <act.h>
#include <mobile.h>
#include <spells.h>
#include <string.h> // strstr()
#include <returnvals.h>

#include <vector>
#include <algorithm>
using namespace std;

extern CWorld world;
extern struct index_data *obj_index; 

////////////////////////////////////////////////////////////////////////////
// local function declarations
void determine_trade_skill_increase(char_data * ch, int skillnum, int learned, int trivial);
int determine_trade_skill_chance(int learned, int trivial);
int valid_trade_skill_combine(struct obj_data * container, struct trade_data_type * data, char_data * ch);

////////////////////////////////////////////////////////////////////////////
// local definitions

#define TRADE_SKILL_POISON_CONT    698    // mortar & pestle

char * tradeskills[] = 
{
  "poisonmaking",
  "\n"
};

#define MAX_INGREDIANTS   10

struct trade_data_type {
   int  pieces[MAX_INGREDIANTS];
   int  result;  // last item in array must be -1 for this
   int  trivial;
};

// POISON DEFINES
struct trade_data_type poison_vial_data[] = 
{
   { { 600, -1, -1, -1, -1, -1, -1, -1, -1, -1 },   // bee stinger
     697,
     10         // a vial of bee stinger poison
   },

   { { 3175, -1, -1, -1, -1, -1, -1, -1, -1, -1 },  // foraged apple
     697,
     10         // a vial of bee stinger poison
   },

   // This must come last
   { { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
     -1,
     -1
   }
};

struct thief_poison_data {
   char * poison_type;
};

struct thief_poison_data poison_vial_combat_data[] =
{
   {
      "bee stinger poison"
   },

   {
      "invalid poison type"
   }
};


////////////////////////////////////////////////////////////////////////////
// command functions

int do_poisonmaking(struct char_data *ch, char *argument, int cmd)
{
  int learned = has_skill(ch, SKILL_TRADE_POISON);

  if(GET_LEVEL(ch) > IMMORTAL)
    learned = 500;

  if(!learned) {
    send_to_char("You do not know how to make poisons.\r\n", ch);
    return eFAILURE;
  }

  // TODO search for mortar and pestle
  obj_data * container = get_obj_in_list_vis(ch, TRADE_SKILL_POISON_CONT, ch->carrying);

  if(!container) {
    send_to_char("You have nothing to make poisons in.\r\n", ch);
    return eFAILURE;
  }

  // search if the items in mortar and pestle match any poison vial types
  int index = valid_trade_skill_combine(container, poison_vial_data, ch);

  send_to_char("You mix the ingrediants together in an attempt to make a poison..\r\n", ch);

  // Clear the items out of the container
  while(container->contains)
     extract_obj( container->contains );

  int failure = 0;
  int chance = 0;
  int percent;

  // determine success/failure

  if(index == -1)
     failure = 1;
  else {
     percent = number(1, 101);
     chance = determine_trade_skill_chance(learned, poison_vial_data[index].trivial);

     determine_trade_skill_increase(ch, SKILL_TRADE_POISON, learned, poison_vial_data[index].trivial);

     if(percent > chance)
        failure = 1;
  }

  if(failure) {
     send_to_char("Ahh crap, something got screwed up.  You are forced to throw away this attempt.\r\n", ch);
     return eFAILURE;
  }
    
  // give poison to player
  int rewardnum = real_object(poison_vial_data[index].result);
  if(rewardnum < 0) {
     send_to_char("That poison is broken.  Tell a god.\r\n", ch);
     return (eFAILURE|eINTERNAL_ERROR);
  }

  obj_data * reward = clone_object(rewardnum);
  obj_to_obj(reward, container);
  csendf(ch, "You succesfully make a %s!\r\n", reward->short_description);

  return eSUCCESS;
}

int do_poisonweapon(struct char_data *ch, char *argument, int cmd)
{
  if(GET_CLASS(ch) != CLASS_THIEF && GET_LEVEL(ch) <= IMMORTAL) {
    send_to_char("Only thieves are trained enough to poison their weapons.\r\n", ch);
    return eFAILURE;
  }

  char weaponarg[MAX_INPUT_LENGTH];
  char vialarg[MAX_INPUT_LENGTH];

  half_chop(argument, weaponarg, vialarg);

  if(!*weaponarg || !*vialarg) {
    send_to_char("Poison which weapon with what?\r\n", ch);
    return eFAILURE;
  }

  // find weapon

  // find vial and verify it's a valid poison vial

  // poison weapon

  // remove vial

  return eSUCCESS;
}

////////////////////////////////////////////////////////////////////////////
// Utility functions

// Search through the different types of data looking for a match
// Return index of match on successful find
// Return -1 on failure
int valid_trade_skill_combine(obj_data * container, trade_data_type * data, char_data * ch)
{
   if(!(container->contains)) {
     csendf(ch, "Your %s appears to be empty.\r\n", container->short_description);
     return -1;
   }

   vector<int> current;

   // take all the items in our container and put them in an array by vnum
   for(obj_data * j = container->contains; j; j = j->next_content)
      if(j->item_number >= 0)
         current.push_back(obj_index[j->item_number].virt);
      else return -1;  // only valid object ingrediants will match

   sort(current.begin(), current.end());

   vector<int> valid;

   // loop through the valid combinations, and try to find a match
   for(int i = 0; data[i].result != -1; i++)
   {
      valid.clear();
      for(int k = 0; k < MAX_INGREDIANTS && k != -1; k++)
         valid.push_back(data[i].pieces[k]);

      sort(valid.begin(), valid.end());

      if(valid == current)  // if vectors match, then we have a valid combination
         return i;
   }

   return -1;  // didn't match any
}


int determine_trade_skill_chance(int learned, int trivial)
{
   return 50;  // flat 50/50 chance
}

void determine_trade_skill_increase(char_data * ch, int skillnum, int learned, int trivial)
{
   int learn_skill(char_data * ch, int skill, int amount, int maximum);

   // can't learn past item's trivial value
   if(learned >= trivial)
      return;

   // TODO - add other requirements when done debugging

   // learn a point
   learn_skill(ch, skillnum, 1, 500);
}
