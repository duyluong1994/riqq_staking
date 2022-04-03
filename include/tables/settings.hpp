#pragma once

struct [[eosio::table, eosio::contract(CONTRACT_NAME)]] setting
{
    name token_contract;
    symbol token_symbol;
}
setting_data;

typedef eosio::singleton<"setting"_n, setting> setting_t;