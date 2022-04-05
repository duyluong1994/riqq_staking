#include <riqq_staking.hpp>

ACTION riqq_staking::regcol(name collection_name)
{
   // Authority check
   check(has_auth(_self), "Insufficient authority.");
   regcol_m(collection_name);
}

ACTION riqq_staking::delcol(name collection_name)
{
   // Authority check
   check(has_auth(_self), "Insufficient authority.");
   delcol_m(collection_name);
}

ACTION riqq_staking::cfgsettings(name token_contract,
                                 symbol token_symbol)
{
   // Authority check
   check(has_auth(_self), "Insufficient authority.");
   cfgsettings_m(token_contract,
                 token_symbol);
}

ACTION riqq_staking::stakenfts(name owner, vector<uint64_t> asset_ids)
{
   // Authority check
   check(has_auth(owner), "Insufficient authority.");
   stakenfts_m(owner, asset_ids);
}

ACTION riqq_staking::setstakecfg(int32_t template_id,
                                 name collection_name,
                                 asset reward_per_duration,
                                 uint64_t earning_duration_hours)
{
   // Authority check
   check(has_auth(_self), "Insufficient authority.");
   setstakecfg_m(template_id,
                 collection_name,
                 reward_per_duration,
                 earning_duration_hours);
}

ACTION riqq_staking::rmstakecfgs(vector<int32_t> template_ids)
{
   // Authority check
   check(has_auth(_self), "Insufficient authority.");
   rmstakecfgs_m(template_ids);
}

ACTION riqq_staking::claim(name wallet)
{
   // Authority check
   check(has_auth(wallet), "Insufficient authority.");
   claim_m(wallet);
}