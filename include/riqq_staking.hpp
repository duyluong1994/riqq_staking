#pragma once

#define CONTRACT_NAME "riqq_staking"
#define ATOMICASSETS "atomicassets"_n

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>

using namespace eosio;
using namespace std;

#include <eosio_token.hpp>
#include <atomichub/atomicdata.hpp>
#include <atomichub/atomicassets-interface.hpp>

#include <tables/settings.hpp>
#include <tables/whitecols.hpp>
#include <tables/stakeconfigs.hpp>
#include <tables/stakedassets.hpp>
#include <tables/users.hpp>

CONTRACT riqq_staking : public contract
{
public:
   using contract::contract;

   // Register collection
   ACTION regcol(name collection_name);
   // Un-Register collection
   ACTION delcol(name collection_name);

   // Config setting
   ACTION cfgsettings(name token_contract,
                      symbol token_symbol);

   // Set stakeconfig
   ACTION setstakecfg(int32_t template_id,
                      name collection_name,
                      asset reward_per_duration,
                      uint64_t earning_duration_days);
   // Remove stakeconfigs
   ACTION rmstakecfgs(vector<int32_t> template_ids);

   // stake nfts
   ACTION stakenfts(name owner, vector<uint64_t> asset_ids);

   // claim
   ACTION claim(name wallet);

   // listener mint
   [[eosio::on_notify("atomicassets::logmint")]] void
   logmint(
       uint64_t asset_id,
       name authorized_minter,
       name collection_name,
       name schema_name,
       int32_t template_id,
       name new_asset_owner,
       ATTRIBUTE_MAP immutable_data,
       ATTRIBUTE_MAP mutable_data,
       vector<asset> backed_tokens,
       ATTRIBUTE_MAP immutable_template_data)
   {
      on_log_mint_m(asset_id, collection_name, template_id, new_asset_owner);
   }

   // listener burn
   [[eosio::on_notify("atomicassets::logburnasset")]] void
   onlogburn(name asset_owner,
             uint64_t asset_id,
             name collection_name,
             name schema_name,
             int32_t template_id,
             vector<asset> backed_tokens,
             ATTRIBUTE_MAP old_immutable_data,
             ATTRIBUTE_MAP old_mutable_data,
             name asset_ram_payer)
   {
      on_log_burn_m(asset_id, template_id);
   }

   // listener transfer
   [[eosio::on_notify("atomicassets::logtransfer")]] void
   onlogtransfer(name collection_name,
                 name from,
                 name to,
                 vector<uint64_t> asset_ids,
                 string memo)
   {
      on_log_transfer_m(collection_name, from, to, asset_ids, memo);
   }

private:
   whitecols_t whitecols = whitecols_t(_self, _self.value);
   setting_t settings = setting_t(_self, _self.value);
   stakeconfigs_t stakeconfigs = stakeconfigs_t(_self, _self.value);
   stakedassets_t stakedassets = stakedassets_t(_self, _self.value);
   users_t users = users_t(_self, _self.value);

   void regcol_m(name collection_name)
   {
      auto itr = whitecols.find(collection_name.value);
      check(itr == whitecols.end(), "The collection has registed.");
      whitecols.emplace(_self, [&](auto &r)
                        { r.collection_name = collection_name; });
   }

   void delcol_m(name collection_name)
   {
      auto itr = whitecols.find(collection_name.value);
      check(itr != whitecols.end(), "The collection has not registed.");
      whitecols.erase(itr);
   }

   setting get_settings()
   {
      check(settings.exists(), "This smart contract do not have setting: " + _self.to_string());
      return settings.get();
   }

   void cfgsettings_m(name token_contract,
                      symbol token_symbol)
   {
      check(is_account(token_contract), "token_contract must be an account.");
      stats statstable(token_contract, token_symbol.code().raw());
      auto existing = statstable.find(token_symbol.code().raw());
      check(existing != statstable.end(), "token with symbol does not exist.");
      check(token_symbol == existing->supply.symbol, "symbol precision mismatch");

      auto _settings = settings.get_or_create(_self, setting_data);
      _settings.token_contract = token_contract;
      _settings.token_symbol = token_symbol;
      settings.set(_settings, _self);
   }

   void setstakecfg_m(int32_t template_id,
                      name collection_name,
                      asset reward_per_duration,
                      uint64_t earning_duration_days)
   {
      auto _setting = get_settings();
      templates_t collection_templates = get_templates(collection_name);
      auto template_itr = collection_templates.find(template_id);
      check(template_itr != collection_templates.end(), "Invalid template_id");

      auto whitecol_itr = whitecols.find(collection_name.value);
      check(whitecol_itr != whitecols.end(), "collection_name is not in whitelist");

      // check asset symbol
      check(_setting.token_symbol == reward_per_duration.symbol, "symbol precision mismatch");

      auto itr = stakeconfigs.find(template_id);
      if (itr != stakeconfigs.end())
      {
         stakeconfigs.modify(itr, _self, [&](auto &r)
                             {
            r.reward_per_duration = reward_per_duration;
            r.earning_duration_days = earning_duration_days; });
      }
      else
      {
         stakeconfigs.emplace(_self, [&](auto &r)
                              {
            r.template_id = template_id;
            r.collection_name = collection_name;
            r.reward_per_duration = reward_per_duration;
            r.earning_duration_days = earning_duration_days; });
      }
   }

   void rmstakecfgs_m(vector<int32_t> template_ids)
   {
      for (int32_t template_id : template_ids)
      {
         auto itr = stakeconfigs.find(template_id);
         check(itr != stakeconfigs.end(), "Invalid template_id: " + to_string(template_id));
         stakeconfigs.erase(itr);
      }
   }

   asset calculate_unclaimed_m(users_t::const_iterator user_itr, time_point current_time)
   {
      uint64_t unclaimed_micro = current_time.time_since_epoch().count() - user_itr->last_claim.time_since_epoch().count();
      asset new_unclaimed = asset((uint64_t)(0), user_itr->unclaimed.symbol);
      for (uint64_t i = 0; i < user_itr->data.size(); i++)
      {
         auto config_itr = stakeconfigs.find(user_itr->data[i].first);
         check(config_itr != stakeconfigs.end(), _self.to_string() + ": Could not find stakeconfig with template_id: " + to_string(user_itr->data[i].first));
         new_unclaimed = asset((uint64_t)(new_unclaimed.amount + (float)(user_itr->data[i].second * config_itr->reward_per_duration.amount * unclaimed_micro / (config_itr->earning_duration_days * days(1).count()))),
                               user_itr->unclaimed.symbol);
      }
      new_unclaimed += user_itr->unclaimed;
      return new_unclaimed;
   }

   void stakenfts_m(name owner, vector<uint64_t> asset_ids)
   {
      auto _setting = get_settings();
      time_point current_time = current_time_point();

      auto user_itr = users.find(owner.value);
      asset new_unclaimed = asset((uint64_t)(0), _setting.token_symbol);
      vector<pair<int32_t, uint64_t>> new_data = vector<pair<int32_t, uint64_t>>();

      if (user_itr != users.end())
      {
         // calculate unclamied
         new_unclaimed = calculate_unclaimed_m(user_itr, current_time);
         new_data = user_itr->data;
      }

      check(asset_ids.size() <= 100, "You can only stake 100 nfts at a time");
      for (uint64_t asset_id : asset_ids)
      {
         // check stakedassets
         auto stakedasset_itr = stakedassets.find(asset_id);
         check(stakedasset_itr == stakedassets.end(), "This asset_id is already staked: " + to_string(asset_id));

         assets_t assets = assets_t(ATOMICASSETS, owner.value);
         auto asset_itr = assets.find(asset_id);
         check(asset_itr != assets.end(), "Invalid asset_id: " + to_string(asset_id));
         // check stakeconfigs
         auto stakeconfig_itr = stakeconfigs.find(asset_itr->template_id);
         check(stakeconfig_itr != stakeconfigs.end(), "There is no config for this template_id: " + to_string(asset_itr->template_id) + " - asset_id: " + to_string(asset_id));
         // check whitecols
         auto whitecol_itr = whitecols.find(asset_itr->collection_name.value);
         check(whitecol_itr != whitecols.end(), "This asset_id is not in whitelist collections: " + to_string(asset_id));
         // update stakedassets
         stakedassets.emplace(_self, [&](auto &r)
                              {
            r.asset_id = asset_id;
            r.owner = owner; });

         // update users data
         auto data_itr = std::find_if(new_data.begin(), new_data.end(), [&](const auto &pair)
                                      { return pair.first == asset_itr->template_id; });
         if (data_itr != new_data.end())
         {
            auto index = std::distance(new_data.begin(), data_itr);
            new_data[index].second += 1;
         }
         else
         {
            pair<int32_t, uint64_t> template_data(asset_itr->template_id, 1);
            new_data.emplace_back(template_data);
         }
      }

      if (user_itr != users.end())
      {
         users.modify(user_itr, _self, [&](auto &r)
                      {
            r.last_claim = current_time;
            r.unclaimed = new_unclaimed;
            r.data = new_data; });
      }
      else
      {
         users.emplace(_self, [&](auto &r)
                       {
            r.wallet = owner;
            r.last_claim = current_time;
            r.unclaimed = new_unclaimed;
            r.data = new_data; });
      }
   }

   void claim_m(name wallet)
   {
      auto _setting = get_settings();
      auto user_itr = users.find(wallet.value);
      check(user_itr != users.end(), "You don't have any staked assets");
      time_point current_time = current_time_point();
      asset new_unclaimed = calculate_unclaimed_m(user_itr, current_time);

      action(
          permission_level{_self, "active"_n},
          _setting.token_contract,
          "transfer"_n,
          std::make_tuple(_self, wallet, new_unclaimed, string("Claim from staked assets")))
          .send();

      users.modify(user_itr, _self, [&](auto &r)
                   {
         r.last_claim = current_time;
         r.unclaimed = asset((uint64_t)(0), _setting.token_symbol); });
   }

   void on_log_transfer_m(name collection_name,
                          name from,
                          name to,
                          vector<uint64_t> asset_ids,
                          string memo)
   {
      auto whitecol_itr = whitecols.find(collection_name.value);
      if (whitecol_itr != whitecols.end())
      {
         auto _setting = get_settings();
         time_point current_time = current_time_point();

         auto from_itr = users.find(from.value);
         asset new_from_unclaimed = asset((uint64_t)(0), _setting.token_symbol);
         vector<pair<int32_t, uint64_t>> new_from_data = vector<pair<int32_t, uint64_t>>();

         if (from_itr != users.end())
         {
            // calculate unclamied
            new_from_unclaimed = calculate_unclaimed_m(from_itr, current_time);
            new_from_data = from_itr->data;
         }

         auto to_itr = users.find(to.value);
         asset new_to_unclaimed = asset((uint64_t)(0), _setting.token_symbol);
         vector<pair<int32_t, uint64_t>> new_to_data = vector<pair<int32_t, uint64_t>>();

         if (to_itr != users.end())
         {
            // calculate unclamied
            new_to_unclaimed = calculate_unclaimed_m(to_itr, current_time);
            new_to_data = to_itr->data;
         }

         assets_t assets = assets_t(ATOMICASSETS, to.value);

         for (uint64_t asset_id : asset_ids)
         {
            auto stakedasset_itr = stakedassets.find(asset_id);
            if (stakedasset_itr != stakedassets.end())
            {
               stakedassets.modify(stakedasset_itr, _self, [&](auto &r)
                                   { r.owner = to; });
            }
            else
            {
               stakedassets.emplace(_self, [&](auto &r)
                                    {
                  r.asset_id = asset_id;
                  r.owner = to; });
            }

            auto asset_itr = assets.find(asset_id);

            // update users data
            auto data_from_itr = std::find_if(new_from_data.begin(), new_from_data.end(), [&](const auto &pair)
                                              { return pair.first == asset_itr->template_id; });
            if (data_from_itr != new_from_data.end())
            {
               auto index = std::distance(new_from_data.begin(), data_from_itr);
               new_from_data[index].second -= 1;
            }

            auto data_to_itr = std::find_if(new_to_data.begin(), new_to_data.end(), [&](const auto &pair)
                                            { return pair.first == asset_itr->template_id; });
            if (data_to_itr != new_to_data.end())
            {
               auto index = std::distance(new_to_data.begin(), data_to_itr);
               new_to_data[index].second += 1;
            }
            else
            {
               pair<int32_t, uint64_t> template_data(asset_itr->template_id, 1);
               new_to_data.emplace_back(template_data);
            }
         }

         if (from_itr != users.end())
         {
            users.modify(from_itr, _self, [&](auto &r)
                         {
               r.last_claim = current_time;
               r.unclaimed = new_from_unclaimed;
               r.data = new_from_data; });
         }
         else
         {
            users.emplace(_self, [&](auto &r)
                          {
               r.wallet = from;
               r.last_claim = current_time;
               r.unclaimed = new_from_unclaimed;
               r.data = new_from_data; });
         }

         if (to_itr != users.end())
         {
            users.modify(to_itr, _self, [&](auto &r)
                         {
               r.last_claim = current_time;
               r.unclaimed = new_to_unclaimed;
               r.data = new_to_data; });
         }
         else
         {
            users.emplace(_self, [&](auto &r)
                          {
               r.wallet = to;
               r.last_claim = current_time;
               r.unclaimed = new_to_unclaimed;
               r.data = new_to_data; });
         }
      }
   }

   void on_log_burn_m(uint64_t asset_id, int32_t template_id)
   {
      auto stakedasset_itr = stakedassets.find(asset_id);
      if (stakedasset_itr != stakedassets.end())
      {
         auto user_itr = users.find(stakedasset_itr->owner.value);
         if (user_itr != users.end())
         {
            auto _setting = get_settings();
            time_point current_time = current_time_point();

            asset new_unclaimed = calculate_unclaimed_m(user_itr, current_time);
            vector<pair<int32_t, uint64_t>> new_data = user_itr->data;

            // update users data
            auto data_itr = std::find_if(new_data.begin(), new_data.end(), [&](const auto &pair)
                                         { return pair.first == template_id; });
            if (data_itr != new_data.end())
            {
               auto index = std::distance(new_data.begin(), data_itr);
               new_data[index].second -= 1;
            }

            users.modify(user_itr, _self, [&](auto &r)
                         {
               r.last_claim = current_time;
               r.unclaimed = new_unclaimed;
               r.data = new_data; });
         }
         stakedassets.erase(stakedasset_itr);
      }
   }

   void on_log_mint_m(uint64_t asset_id, name collection_name, int32_t template_id, name owner)
   {
      auto whitecol_itr = whitecols.find(collection_name.value);
      if (whitecol_itr != whitecols.end())
      {
         stakedassets.emplace(_self, [&](auto &r)
                              {
            r.asset_id = asset_id;
            r.owner = owner; });

         auto _setting = get_settings();
         time_point current_time = current_time_point();
         auto user_itr = users.find(owner.value);
         if (user_itr != users.end())
         {
            asset new_unclaimed = calculate_unclaimed_m(user_itr, current_time);
            vector<pair<int32_t, uint64_t>> new_data = user_itr->data;
            // update users data
            auto data_itr = std::find_if(new_data.begin(), new_data.end(), [&](const auto &pair)
                                         { return pair.first == template_id; });
            if (data_itr != new_data.end())
            {
               auto index = std::distance(new_data.begin(), data_itr);
               new_data[index].second += 1;
            }
            else
            {
               pair<int32_t, uint64_t> template_data(template_id, 1);
               new_data.emplace_back(template_data);
            }

            users.modify(user_itr, _self, [&](auto &r)
                         {
               r.last_claim = current_time;
               r.unclaimed = new_unclaimed;
               r.data = new_data; });
         }
         else
         {
            asset new_unclaimed = asset((uint64_t)(0), _setting.token_symbol);
            pair<int32_t, uint64_t> data(template_id, 1);
            vector<pair<int32_t, uint64_t>> new_data{data};
            users.emplace(_self, [&](auto &r)
                          {
               r.wallet = owner;
               r.last_claim = current_time;
               r.unclaimed = new_unclaimed;
               r.data = new_data; });
         }
      }
   }
};