#pragma once

#include "json/json.h"
#include "board-initializer.h"
#include "game-engine/card-database.h"
#include "game-engine/game-engine.h"
#include "deck-initializer.h"

class BoardJsonParser : public BoardInitializer
{
public:
	BoardJsonParser(Json::Value const& json) : origin_json(json)
	{
	}

	void InitializeBoard(int rand_seed, GameEngine::Board & board) const
	{
		board.SetRandomSeed(rand_seed);
		this->Parse(board);
	}

	BoardInitializer * Clone() const
	{
		BoardJsonParser * ret = new BoardJsonParser(this->origin_json);
		return ret;
	}

private:
	void Parse(GameEngine::Board & board) const
	{
		// TODO: determine hand/deck type
		DeckInitializer player_deck, opponent_deck;
		//player_deck.SetDeck_BasicPracticeMage();
		//opponent_deck.SetDeck_BasicPracticeMage();
		//player_deck.SetDeck_BasicPracticeWarlock();
		//opponent_deck.SetDeck_BasicPracticeWarlock();
		//player_deck.SetDeck_BasicPracticeDruid();
		//opponent_deck.SetDeck_BasicPracticeDruid();
		player_deck.SetDeck_BasicPracticeShaman();
		opponent_deck.SetDeck_BasicPracticeShaman();

		this->ParsePlayer(this->origin_json["player"], board.player);
		this->ParsePlayer(this->origin_json["opponent"], board.opponent);

		// parse stage
		bool player_goes_first = this->origin_json["player"]["first_player"].asBool();
		int turn = this->origin_json["turn"].asInt();

		if (player_goes_first && (turn % 2 == 1)) {
			board.SetStateToPlayerChooseBoardMove();
		}
		else if (!player_goes_first && (turn % 2 == 0)) {
			board.SetStateToPlayerChooseBoardMove();
		}
		else {
			board.SetStateToOpponentChooseBoardMove();
		}

		

		this->InitializeHand(
			this->origin_json["player"]["deck"]["played_cards"], this->origin_json["player"]["hand"]["cards"], false,
			player_deck, board.player.hand);
		this->InitializeHand(
			this->origin_json["opponent"]["deck"]["played_cards"], this->origin_json["opponent"]["hand"]["cards"], player_goes_first,
			opponent_deck, board.opponent.hand);
	}

	void InitializeHand(Json::Value const& played_cards, Json::Value const& hand_cards, bool add_coin_if_not_played, DeckInitializer const& deck, GameEngine::Hand & hand) const
	{
		bool hand_has_coin = false;

		if (add_coin_if_not_played) {
			bool played_the_coin = false;

			constexpr int the_coin_card_id = CARD_ID_GAME_005;
			for (auto const& played_card : played_cards)
			{
				int card_id = GameEngine::CardDatabase::GetInstance().GetCardIdFromOriginalId(played_card.asString());
				if (card_id < 0) {
					continue;
				}
				if (card_id == the_coin_card_id) {
					played_the_coin = true;
					break;
				}
			}

			if (!played_the_coin) hand_has_coin = true;
		}

		deck.InitializeHand(played_cards, hand_cards, hand, hand_has_coin);
	}

	void ParsePlayer(Json::Value const& json, GameEngine::Player & player) const
	{
		this->ParseCrystal(json["crystal"], player.stat.crystal);
		player.stat.fatigue_damage = json["fatigue"].asInt();

		this->ParseHero(json["hero"], player.hero);
		this->ParseMinions(json["minions"], player.minions);

		this->ParseWeapon(json["weapon"], player);

		// TODO: spell damage
		//      CURRENT_SPELLPOWER

		// TODO: secrets

		// TODO: add known enchantments (e.g., one-turn effects)

		if (player.hero.GetWeaponAttack() != json["weapon"]["attack"].asInt()) {
			throw std::runtime_error("weapon is enchanted? we're not supported it yet.");
		}

		// add an enchantment to make the stat correct
		int attack_boost = json["hero"]["attack"].asInt() - player.hero.GetAttack();
		int spell_damage_boost = 0; // TODO: parse the player's spell damage boost
		if (attack_boost > 0 || spell_damage_boost > 0) {
			player.enchantments.Add(std::make_unique<GameEngine::Enchantment_BuffPlayer>(attack_boost, spell_damage_boost, false));
		}
	}

	void ParseCrystal(Json::Value const& json, GameEngine::PlayerStat::Crystal & crystal) const
	{
		int total = json["total"].asInt();
		int used = json["used"].asInt();
		int this_turn = json["this_turn"].asInt();
		int overload = json["overload"].asInt();
		int overload_next_turn = json["overload_next_turn"].asInt();

		crystal.Set(total + this_turn - used, total, overload, overload_next_turn);
	}

	void ParseHero(Json::Value const& json, GameEngine::Hero & hero) const
	{
		GameEngine::HeroData hero_data;

		// card_id not used
		hero_data.max_hp = json["max_hp"].asInt();
		hero_data.hp = hero_data.max_hp - json["damage"].asInt();
		hero_data.attacked_times = json["attacks_this_turn"].asInt();
		hero_data.armor = json["armor"].asInt();
		hero_data.freezed = json["status"]["frozen"].asInt() != 0;
		hero_data.attack = 0;

		hero_data.hero_power.card_id = GameEngine::CardDatabase::GetInstance().GetCardIdFromOriginalId(json["hero_power"]["card_id"].asString());
		GameEngine::Card card_hero_power = GameEngine::CardDatabase::GetInstance().GetCard(hero_data.hero_power.card_id);
		hero_data.hero_power.cost = card_hero_power.cost;
		hero_data.hero_power.used_this_turn = json["hero_power"]["used"].asBool();

		hero_data.weapon.Clear();

		hero.SetHero(hero_data);
	}

	void ParseWeapon(Json::Value const& json, GameEngine::Player & player) const
	{
		if (json["equipped"].asBool() == false) return;

		// TODO: test if this parse is correct
		int weapon_card_id = GameEngine::CardDatabase::GetInstance().GetCardIdFromOriginalId(json["card_id"].asString());
		GameEngine::Card weapon_card = GameEngine::CardDatabase::GetInstance().GetCard(weapon_card_id);

		auto & weapon_data = player.hero.GetHeroDataForBoardInitialization().weapon;
		weapon_data.card_id = weapon_card_id;
		weapon_data.cost = weapon_card.cost;
		weapon_data.attack = weapon_card.data.weapon.attack;
		weapon_data.durability = json["durability"].asInt() - json["damage"].asInt();
		weapon_data.forgetful = weapon_card.data.weapon.forgetful;
		weapon_data.freeze_attack = weapon_card.data.weapon.freeze;
		weapon_data.windfury = weapon_card.data.weapon.windfury;
	}

	void ParseMinions(Json::Value const& json, GameEngine::Minions & minions) const
	{
		std::vector<GameEngine::MinionIterator> minion_it_map;
		minion_it_map.reserve(json.size());

		for (unsigned int i = 0; i < json.size(); ++i)
		{
			auto const& json_minion = json[i];
			int card_id = GameEngine::CardDatabase::GetInstance().GetCardIdFromOriginalId(json_minion["card_id"].asString());

			bool silenced = json_minion["silenced"].asBool();

			GameEngine::MinionData minion_data = this->ParseMinion(json_minion);

			auto minion_it = minions.InsertBefore(minions.GetEndIterator(), std::move(minion_data));

			if (silenced) {
				// clear deathrattles
				minion_data.triggers_on_death.clear();

				// auras are not attached since it's slienced
			}
			else {
				// original deathrattles have been added in 'ParseMinion'

				GameEngine::Cards::CardCallbackManager::AttachAura(card_id, minion_it->auras);
			}

			if (minion_it_map.size() != i) throw std::runtime_error("logic error");
			minion_it_map.push_back(minion_it);
		}

		for (unsigned int i = 0; i < json.size(); ++i)
		{
			auto const& json_minion = json[i];
			auto minion_it = minion_it_map[i];

			// TODO: add enchantments attached with aura
			// TODO: add one turn enchantments

			// set status
			this->SetFinalMinionStatus(json_minion, minion_it);
		}
	}

	GameEngine::MinionData ParseMinion(Json::Value const& json) const
	{
		int card_id = GameEngine::CardDatabase::GetInstance().GetCardIdFromOriginalId(json["card_id"].asString());
		GameEngine::Card card = GameEngine::CardDatabase::GetInstance().GetCard(card_id);
		auto minion_data = GameEngine::MinionData::FromCard(card);

		minion_data.summoned_this_turn = json["summoned_this_turn"].asBool();
		minion_data.attacked_times = json["attacks_this_turn"].asInt();

		return std::move(minion_data);
	}

	void SetFinalMinionStatus(Json::Value const& json, GameEngine::MinionIterator it) const
	{
		auto & minion_data = it->GetMinionForBoardInitialization();

		// add an enchant to adjust the attack and max_hp
		int attack_boost = json["attack"].asInt() - it->GetAttack();
		int max_hp_boost = json["max_hp"].asInt() - it->GetMaxHP();
		int spell_damage_boost = json["spellpower"].asInt() - it->GetMinion().stat.GetSpellDamage();
		if (attack_boost != 0 || max_hp_boost != 0 || spell_damage_boost != 0) {
			auto enchant = new GameEngine::Enchantment_BuffMinion(attack_boost, max_hp_boost, spell_damage_boost, 0, false);
			it->enchantments.Add(std::unique_ptr<GameEngine::Enchantment_BuffMinion>(enchant));
		}

		if (json["attack"].asInt() != it->GetAttack()) throw std::runtime_error("attack not match");
		if (json["max_hp"].asInt() != it->GetMaxHP()) throw std::runtime_error("max_hp not match");
		if (json["spellpower"].asInt() != it->GetMinion().stat.GetSpellDamage()) throw std::runtime_error("spell damage not match");

		// set current hp
		minion_data.stat.SetHP(json["max_hp"].asInt() - json["damage"].asInt());

		Json::Value const& json_status = json["status"];
		if (json_status["charge"].asInt() > 0) minion_data.stat.SetCharge();
		if (json_status["taunt"].asInt() > 0) minion_data.stat.SetTaunt();
		if (json_status["divine_shield"].asInt() > 0) minion_data.stat.SetShield();
		if (json_status["stealth"].asInt() > 0) minion_data.stat.SetStealth();
		if (json_status["forgetful"].asInt() > 0) minion_data.stat.SetForgetful();
		if (json_status["freeze"].asInt() > 0) minion_data.stat.SetFreezeAttacker();
		if (json_status["frozen"].asInt() > 0) minion_data.stat.SetFreezed();
		if (json_status["poisonous"].asInt() > 0) minion_data.stat.SetPoisonous();
		if (json_status["windfury"].asInt() > 0) minion_data.stat.SetWindFury();
	}

private:
	Json::Value const origin_json;
};