#!/usr/bin/env bash

BUILD_PATH="${PWD}/build/riqq_staking"

# CLEOS="cleos -u https://wax.greymass.com"
# CLEOS="cleos -u https://waxtestnet.greymass.com"
# CLEOS="cleos -u https://testnet.waxsweden.org"
CLEOS="cleos -u https://testnet.wax.pink.gg"
$CLEOS set contract riqqstaking1 $BUILD_PATH riqq_staking.wasm riqq_staking.abi -p riqqstaking1@active
